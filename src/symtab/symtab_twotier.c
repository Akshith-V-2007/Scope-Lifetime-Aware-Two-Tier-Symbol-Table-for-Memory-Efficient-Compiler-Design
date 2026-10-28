#include "symtab_twotier.h"
#include "../common/bloom.h"
#include "../common/mem_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define TWOTIER_INITIAL_SCOPE_CAPACITY 8
#define HOT_TABLE_INITIAL_SIZE 32
#define DECOMP_CACHE_SIZE 2

typedef struct {
    name_id_t       name_id;
    scope_id_t      scope_id;
    packed_symbol_t symbol;
} hot_slot_t;

typedef struct {
    scope_id_t  scope_id;
    uint8_t     depth;
    bool        is_hot;
    arena_t*    arena;

    /* Hot state */
    packed_symbol_t* hot_symbols;
    size_t           symbol_count;
    size_t           symbol_capacity;

    /* Cold state */
    compressed_scope_block_t* cold_block;
} twotier_scope_t;

typedef struct {
    scope_id_t       cached_scope_id;
    packed_symbol_t* symbols;
    size_t           count;
    size_t           capacity;
    uint64_t         lru_tick;
} twotier_decomp_cache_t;

typedef struct {
    int hot_depth;
    size_t ram_budget;

    string_pool_t* pool;
    type_table_t*  type_table;

    twotier_scope_t* scopes;
    size_t scope_capacity;
    int scope_depth; /* -1 = empty, 0 = global */
    scope_id_t next_scope_id;

    /* Hot Tier Open-Addressing Hash Table */
    hot_slot_t* hot_slots;
    size_t hot_mask;
    size_t hot_count;

    /* Cold Tier Decompression Cache */
    twotier_decomp_cache_t cache[DECOMP_CACHE_SIZE];
    uint64_t global_tick;

    /* Scratch unpacked result */
    symbol_record_t cached_result;

    /* Flags & Stats */
    bool bloom_enabled;
    bool range_enabled;
    twotier_demote_policy_t demote_policy;
    int min_scope_size;
    symtab_stats_t stats;
} twotier_impl_t;

static void demote_scope_to_cold(twotier_impl_t* impl, int scope_idx);

static uint32_t hash_hot_key(name_id_t name_id) {
    uint32_t x = (uint32_t)name_id;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

static void hot_table_insert(twotier_impl_t* impl, const packed_symbol_t* sym, scope_id_t scope_id) {
    if (!impl->hot_slots) return;

    /* Resize if load factor > 70% */
    if (impl->hot_count * 10 >= (impl->hot_mask + 1) * 7) {
        size_t old_size = impl->hot_mask + 1;
        size_t new_size = old_size * 2;
        hot_slot_t* new_slots = (hot_slot_t*)tracked_calloc(new_size, sizeof(hot_slot_t));
        if (new_slots) {
            size_t new_mask = new_size - 1;
            for (size_t i = 0; i < old_size; i++) {
                if (impl->hot_slots[i].name_id != 0) {
                    uint32_t h = hash_hot_key(impl->hot_slots[i].name_id);
                    size_t idx = h & new_mask;
                    while (new_slots[idx].name_id != 0) {
                        idx = (idx + 1) & new_mask;
                    }
                    new_slots[idx] = impl->hot_slots[i];
                }
            }
            tracked_free(impl->hot_slots);
            impl->hot_slots = new_slots;
            impl->hot_mask = new_mask;
        }
    }

    if (impl->hot_count >= impl->hot_mask + 1) return; /* Table full and resize failed */

    uint32_t h = hash_hot_key(sym->name_id);
    size_t idx = h & impl->hot_mask;
    size_t start_idx = idx;

    while (impl->hot_slots[idx].name_id != 0) {
        idx = (idx + 1) & impl->hot_mask;
        if (idx == start_idx) return; /* Table completely full */
    }

    impl->hot_slots[idx].name_id = sym->name_id;
    impl->hot_slots[idx].scope_id = scope_id;
    impl->hot_slots[idx].symbol = *sym;
    impl->hot_count++;
}

static inline bool is_between_cyclic(size_t a, size_t b, size_t c) {
    if (a <= c) {
        return (a < b && b <= c);
    } else {
        return (a < b || b <= c);
    }
}

static void hot_table_remove_scope(twotier_impl_t* impl, scope_id_t scope_id) {
    size_t cap = impl->hot_mask + 1;
    for (size_t i = 0; i < cap; i++) {
        while (impl->hot_slots[i].name_id != 0 && impl->hot_slots[i].scope_id == scope_id) {
            /* Vacate slot i */
            impl->hot_slots[i].name_id = 0;
            impl->hot_slots[i].scope_id = 0;
            memset(&impl->hot_slots[i].symbol, 0, sizeof(packed_symbol_t));
            impl->hot_count--;

            /* Shift subsequent cluster elements back (Knuth 6.4R) */
            size_t j = i;
            size_t k = (j + 1) & impl->hot_mask;
            while (impl->hot_slots[k].name_id != 0) {
                size_t r = hash_hot_key(impl->hot_slots[k].name_id) & impl->hot_mask;
                if (!is_between_cyclic(j, r, k)) {
                    impl->hot_slots[j] = impl->hot_slots[k];
                    impl->hot_slots[k].name_id = 0;
                    impl->hot_slots[k].scope_id = 0;
                    memset(&impl->hot_slots[k].symbol, 0, sizeof(packed_symbol_t));
                    j = k;
                }
                k = (k + 1) & impl->hot_mask;
            }
        }
    }
}

static void promote_scope_to_hot(twotier_impl_t* impl, int scope_idx) {
    twotier_scope_t* s = &impl->scopes[scope_idx];
    if (s->is_hot || !s->cold_block) return;

    size_t count = s->cold_block->symbol_count;
    size_t cap = count + 16;
    if (cap < 16) cap = 16;

    arena_t* new_arena = arena_create(256);
    if (!new_arena) return;
    packed_symbol_t* new_hot = (packed_symbol_t*)arena_alloc(new_arena, sizeof(packed_symbol_t) * cap);
    if (!new_hot) {
        arena_destroy(new_arena);
        return;
    }

    /* Invalidate any decompression cache entry */
    for (int c = 0; c < DECOMP_CACHE_SIZE; c++) {
        if (impl->cache[c].cached_scope_id == s->scope_id) {
            impl->cache[c].cached_scope_id = 0;
            impl->cache[c].count = 0;
        }
    }

    if (s->arena) {
        arena_destroy(s->arena);
        s->arena = NULL;
    }
    s->arena = new_arena;
    s->hot_symbols = new_hot;
    s->symbol_capacity = cap;
    s->symbol_count = scope_block_decompress(s->cold_block, s->hot_symbols, cap);

    /* Re-insert decompressed symbols into hot table */
    for (size_t i = 0; i < s->symbol_count; i++) {
        hot_table_insert(impl, &s->hot_symbols[i], s->scope_id);
    }

    scope_block_destroy(s->cold_block);
    s->cold_block = NULL;
    s->is_hot = true;
}

static void twotier_enter_scope(symtab_t* self) {
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    impl->scope_depth++;
    int d = impl->scope_depth;
    if ((size_t)d >= impl->scope_capacity) {
        size_t new_cap = impl->scope_capacity * 2;
        twotier_scope_t* new_scopes = (twotier_scope_t*)tracked_realloc(impl->scopes, sizeof(twotier_scope_t) * new_cap);
        if (new_scopes) {
            memset(&new_scopes[impl->scope_capacity], 0, sizeof(twotier_scope_t) * (new_cap - impl->scope_capacity));
            impl->scopes = new_scopes;
            impl->scope_capacity = new_cap;
        } else {
            /* Out of memory / budget limit hit: rollback scope depth */
            impl->scope_depth--;
            return;
        }
    }
    scope_id_t id = ++impl->next_scope_id;

    impl->scopes[d].scope_id = id;
    impl->scopes[d].depth = (uint8_t)d;
    impl->scopes[d].is_hot = true;
    impl->scopes[d].arena = arena_create(256); /* lightweight per-scope arena */
    impl->scopes[d].symbol_count = 0;
    impl->scopes[d].symbol_capacity = 16;
    impl->scopes[d].hot_symbols = impl->scopes[d].arena ? (packed_symbol_t*)arena_alloc(
        impl->scopes[d].arena, 
        sizeof(packed_symbol_t) * impl->scopes[d].symbol_capacity
    ) : NULL;
    impl->scopes[d].cold_block = NULL;

    /* Check if ancestor scopes should be demoted to Cold Tier */
    /* Under DEPTH_BOUNDED policy: keep top 'hot_depth' scopes hot; demote deeper */
    if (impl->demote_policy == DEMOTE_POLICY_DEPTH_BOUNDED) {
        if (d >= impl->hot_depth) {
            int demote_idx = d - impl->hot_depth;
            if (demote_idx >= 0 && impl->scopes[demote_idx].is_hot) {
                if (impl->min_scope_size <= 0 || impl->scopes[demote_idx].symbol_count >= (size_t)impl->min_scope_size) {
                    demote_scope_to_cold(impl, demote_idx);
                }
            }
        }
    } else if (impl->demote_policy == DEMOTE_POLICY_SIZE_AWARE) {
        if (d >= impl->hot_depth) {
            int demote_idx = d - impl->hot_depth;
            if (demote_idx >= 0 && impl->scopes[demote_idx].is_hot) {
                /* Demote only if scope holds >= threshold symbols (default 16) */
                size_t threshold = (impl->min_scope_size > 0) ? (size_t)impl->min_scope_size : 16;
                if (impl->scopes[demote_idx].symbol_count >= threshold) {
                    demote_scope_to_cold(impl, demote_idx);
                }
            }
        }
    }

    /* Budget check: if exceeding RAM cap, demote more scopes */
    if (impl->ram_budget > 0 && mem_tracker_get_current() > impl->ram_budget) {
        for (int i = 0; i < d; i++) {
            if (impl->scopes[i].is_hot) {
                demote_scope_to_cold(impl, i);
                if (mem_tracker_get_current() <= impl->ram_budget) break;
            }
        }
    }

    impl->stats.total_scopes_created++;
    impl->stats.current_scope_depth = impl->scope_depth;
    if (impl->scope_depth > impl->stats.max_scope_depth) {
        impl->stats.max_scope_depth = impl->scope_depth;
    }
    impl->stats.current_bytes = mem_tracker_get_current();
    if (impl->stats.current_bytes > impl->stats.peak_bytes) {
        impl->stats.peak_bytes = impl->stats.current_bytes;
    }
}

static void demote_scope_to_cold(twotier_impl_t* impl, int scope_idx) {
    twotier_scope_t* s = &impl->scopes[scope_idx];
    if (!s->is_hot) return;

    /* Compress symbols into cold block */
    compressed_scope_block_t* block = scope_block_create(s->scope_id, s->depth);
    if (!block) return;
    if (!scope_block_compress(block, s->hot_symbols, s->symbol_count)) {
        scope_block_destroy(block);
        return;
    }

    /* Invalidate any stale decompression cache entry for this scope */
    for (int c = 0; c < DECOMP_CACHE_SIZE; c++) {
        if (impl->cache[c].cached_scope_id == s->scope_id) {
            impl->cache[c].cached_scope_id = 0;
            impl->cache[c].count = 0;
        }
    }

    /* Remove from hot table */
    hot_table_remove_scope(impl, s->scope_id);

    s->cold_block = block;

    /* Destroy hot arena to save memory while in cold tier */
    if (s->arena) {
        arena_destroy(s->arena);
        s->arena = NULL;
    }
    s->hot_symbols = NULL;
    s->is_hot = false;
}

static void twotier_exit_scope(symtab_t* self) {
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    if (impl->scope_depth < 0) return;

    int d = impl->scope_depth;
    twotier_scope_t* s = &impl->scopes[d];

    /* Update symbol count */
    size_t count = s->is_hot ? s->symbol_count : (s->cold_block ? s->cold_block->symbol_count : 0);
    if (impl->stats.current_symbols >= count) {
        impl->stats.current_symbols -= count;
    }

    /* Invalidate decompression cache if this scope was cached */
    for (int i = 0; i < DECOMP_CACHE_SIZE; i++) {
        if (impl->cache[i].cached_scope_id == s->scope_id) {
            impl->cache[i].cached_scope_id = 0;
            impl->cache[i].count = 0;
        }
    }

    /* Remove from hot table if it was hot */
    if (s->is_hot) {
        hot_table_remove_scope(impl, s->scope_id);
    }

    /* Destroy cold block if cold */
    if (s->cold_block) {
        scope_block_destroy(s->cold_block);
        s->cold_block = NULL;
    }

    /* O(1) Scope Arena Destruction */
    if (s->arena) {
        arena_destroy(s->arena);
        s->arena = NULL;
    }

    impl->scope_depth--;
    if (impl->scope_depth >= 0) {
        int start = impl->scope_depth - impl->hot_depth + 1;
        if (start < 0) start = 0;
        for (int i = start; i <= impl->scope_depth; i++) {
            if (!impl->scopes[i].is_hot && impl->scopes[i].cold_block) {
                promote_scope_to_hot(impl, i);
            }
        }
    }

    impl->stats.total_scopes_destroyed++;
    impl->stats.current_scope_depth = impl->scope_depth;
    impl->stats.current_bytes = mem_tracker_get_current();
}

static const symbol_record_t* twotier_lookup_current_scope(symtab_t* self, const char* name) {
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    if (impl->scope_depth < 0 || !name) return NULL;

    name_id_t id = string_pool_intern(impl->pool, name);
    int d = impl->scope_depth;
    twotier_scope_t* s = &impl->scopes[d];

    if (s->is_hot) {
        for (size_t i = 0; i < s->symbol_count; i++) {
            if (s->hot_symbols[i].name_id == id) {
                unpack_symbol(&s->hot_symbols[i], &impl->cached_result, name);
                impl->cached_result.scope_id = s->scope_id;
                return &impl->cached_result;
            }
        }
    } else if (s->cold_block) {
        /* Rare: current scope was demoted (e.g. by budget mode) */
        if (impl->bloom_enabled && !bloom_test(s->cold_block->bloom, id)) {
            return NULL;
        }
        /* Check cache or decompress */
        packed_symbol_t scratch[256];
        size_t n = scope_block_decompress(s->cold_block, scratch, 256);
        const packed_symbol_t* found = scope_symbols_find(scratch, n, id);
        if (found) {
            unpack_symbol(found, &impl->cached_result, name);
            impl->cached_result.scope_id = s->scope_id;
            return &impl->cached_result;
        }
    }

    return NULL;
}

static bool twotier_insert(symtab_t* self, const char* name, type_id_t type_id, symbol_kind_t kind, int32_t offset) {
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    if (impl->scope_depth < 0 || !name) return false;

    /* Check redeclaration in current scope */
    if (twotier_lookup_current_scope(self, name) != NULL) {
        return false;
    }

    name_id_t nid = string_pool_intern(impl->pool, name);
    if (nid == NAME_ID_INVALID) return false;

    int d = impl->scope_depth;
    twotier_scope_t* s = &impl->scopes[d];

    /* If current scope was demoted (e.g. by budget mode), promote it back to hot */
    if (!s->is_hot) {
        promote_scope_to_hot(impl, d);
        if (!s->is_hot) return false;
    }

    if (!s->hot_symbols) {
        if (!s->arena) s->arena = arena_create(256);
        if (s->arena) {
            s->hot_symbols = (packed_symbol_t*)arena_alloc(s->arena, sizeof(packed_symbol_t) * s->symbol_capacity);
        }
        if (!s->hot_symbols) return false;
    }

    /* Expand hot_symbols array if needed */
    if (s->symbol_count >= s->symbol_capacity) {
        size_t new_cap = s->symbol_capacity * 2;
        packed_symbol_t* new_syms = s->arena ? (packed_symbol_t*)arena_alloc(s->arena, sizeof(packed_symbol_t) * new_cap) : NULL;
        if (!new_syms) return false;
        memcpy(new_syms, s->hot_symbols, sizeof(packed_symbol_t) * s->symbol_count);
        s->hot_symbols = new_syms;
        s->symbol_capacity = new_cap;
    }

    symbol_record_t rec;
    rec.name = name;
    rec.name_id = nid;
    rec.kind = kind;
    rec.storage = STORAGE_AUTO;
    rec.type_id = type_id;
    rec.offset = offset;
    rec.scope_depth = (uint32_t)d;
    rec.scope_id = s->scope_id;

    packed_symbol_t ps = pack_symbol(&rec);
    s->hot_symbols[s->symbol_count++] = ps;

    /* Insert into hot open-addressing table */
    hot_table_insert(impl, &ps, s->scope_id);

    impl->stats.total_inserts++;
    impl->stats.current_symbols++;
    if (impl->stats.current_symbols > impl->stats.peak_symbols) {
        impl->stats.peak_symbols = impl->stats.current_symbols;
    }

    impl->stats.current_bytes = mem_tracker_get_current();
    if (impl->stats.current_bytes > impl->stats.peak_bytes) {
        impl->stats.peak_bytes = impl->stats.current_bytes;
    }

    return true;
}

static const symbol_record_t* twotier_lookup(symtab_t* self, const char* name) {
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    if (!name) return NULL;
    impl->stats.total_lookups++;
    impl->global_tick++;

    name_id_t nid = string_pool_intern(impl->pool, name);

    /* 1. Hot Tier Search: probe hot open-addressing table */
    const packed_symbol_t* best_hot_sym = NULL;
    uint8_t best_hot_depth = 0;
    scope_id_t best_hot_scope = 0;

    uint32_t h = hash_hot_key(nid);
    size_t idx = h & impl->hot_mask;
    size_t start_idx = idx;

    while (impl->hot_slots[idx].name_id != 0) {
        if (impl->hot_slots[idx].name_id == nid) {
            /* If multiple hot scopes have this symbol, pick highest scope depth (shadowing) */
            if (!best_hot_sym || impl->hot_slots[idx].symbol.scope_depth >= best_hot_depth) {
                best_hot_sym = &impl->hot_slots[idx].symbol;
                best_hot_depth = impl->hot_slots[idx].symbol.scope_depth;
                best_hot_scope = impl->hot_slots[idx].scope_id;
            }
        }
        idx = (idx + 1) & impl->hot_mask;
        if (idx == start_idx) break;
    }

    if (best_hot_sym) {
        impl->stats.hot_hits++;
        unpack_symbol(best_hot_sym, &impl->cached_result, name);
        impl->cached_result.scope_id = best_hot_scope;
        return &impl->cached_result;
    }

    /* 2. Cold Tier Search: traverse cold blocks from innermost to outermost (ancestors) */
    for (int d = impl->scope_depth; d >= 0; d--) {
        twotier_scope_t* s = &impl->scopes[d];
        if (s->is_hot || !s->cold_block || s->cold_block->symbol_count == 0) {
            continue;
        }

        compressed_scope_block_t* blk = s->cold_block;

        /* Range check */
        if (impl->range_enabled) {
            if (nid < blk->min_name_id || nid > blk->max_name_id) {
                impl->stats.range_skips++;
                continue;
            }
        }

        /* Bloom Filter check */
        if (impl->bloom_enabled) {
            if (!bloom_test(blk->bloom, nid)) {
                impl->stats.bloom_skips++;
                continue; /* Definitely not in this block! ZERO decompression! */
            }
        }

        /* Bloom hit (or Bloom disabled) -> check Decompression Cache */
        twotier_decomp_cache_t* cache_slot = NULL;
        for (int c = 0; c < DECOMP_CACHE_SIZE; c++) {
            if (impl->cache[c].cached_scope_id == blk->scope_id) {
                cache_slot = &impl->cache[c];
                impl->stats.cache_hits++;
                break;
            }
        }

        if (!cache_slot) {
            /* Decompression Cache Miss -> Decompress into oldest LRU slot */
            impl->stats.cache_misses++;
            impl->stats.decompressions++;

            int lru_idx = 0;
            uint64_t min_tick = impl->cache[0].lru_tick;
            for (int c = 1; c < DECOMP_CACHE_SIZE; c++) {
                if (impl->cache[c].lru_tick < min_tick) {
                    min_tick = impl->cache[c].lru_tick;
                    lru_idx = c;
                }
            }
            cache_slot = &impl->cache[lru_idx];

            if (cache_slot->capacity < blk->symbol_count) {
                size_t new_cap = blk->symbol_count + 32;
                packed_symbol_t* new_syms = (packed_symbol_t*)tracked_realloc(
                    cache_slot->symbols, 
                    sizeof(packed_symbol_t) * new_cap
                );
                if (!new_syms) {
                    /* Out of memory / budget limit hit */
                    impl->stats.misses++;
                    return NULL;
                }
                cache_slot->symbols = new_syms;
                cache_slot->capacity = new_cap;
            }

            if (!cache_slot->symbols) {
                impl->stats.misses++;
                return NULL;
            }

            cache_slot->count = scope_block_decompress(blk, cache_slot->symbols, cache_slot->capacity);
            cache_slot->cached_scope_id = blk->scope_id;
        }
        cache_slot->lru_tick = impl->global_tick;

        /* Binary search in sorted decompressed symbols */
        const packed_symbol_t* found = scope_symbols_find(cache_slot->symbols, cache_slot->count, nid);
        if (found) {
            impl->stats.cold_hits++;
            unpack_symbol(found, &impl->cached_result, name);
            impl->cached_result.scope_id = s->scope_id;
            return &impl->cached_result;
        } else {
            /* False positive of the Bloom filter */
            impl->stats.bloom_false_positives++;
        }
    }

    impl->stats.misses++;
    return NULL;
}

static int twotier_get_scope_depth(symtab_t* self) {
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    return impl->scope_depth;
}

static void twotier_get_stats(symtab_t* self, symtab_stats_t* stats) {
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    impl->stats.current_bytes = mem_tracker_get_current();
    if (impl->stats.current_bytes > impl->stats.peak_bytes) {
        impl->stats.peak_bytes = impl->stats.current_bytes;
    }
    *stats = impl->stats;
}

static void twotier_set_param(symtab_t* self, const char* key, int value) {
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    if (strcmp(key, "hot_depth") == 0) {
        impl->hot_depth = value;
    } else if (strcmp(key, "bloom") == 0) {
        impl->bloom_enabled = (value != 0);
    } else if (strcmp(key, "range") == 0 || strcmp(key, "range_filter") == 0) {
        impl->range_enabled = (value != 0);
    } else if (strcmp(key, "ram_budget") == 0) {
        impl->ram_budget = (size_t)value;
    } else if (strcmp(key, "demote_policy") == 0) {
        impl->demote_policy = (twotier_demote_policy_t)value;
    } else if (strcmp(key, "min_scope_size") == 0) {
        impl->min_scope_size = value;
    }
}

static void twotier_destroy(symtab_t* self) {
    if (!self) return;
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    if (impl) {
        while (impl->scope_depth >= 0) {
            int d = impl->scope_depth;
            twotier_scope_t* s = &impl->scopes[d];
            if (s->is_hot) {
                hot_table_remove_scope(impl, s->scope_id);
            }
            if (s->cold_block) {
                scope_block_destroy(s->cold_block);
                s->cold_block = NULL;
            }
            if (s->arena) {
                arena_destroy(s->arena);
                s->arena = NULL;
            }
            impl->scope_depth--;
        }

        if (impl->hot_slots) {
            tracked_free(impl->hot_slots);
        }

        if (impl->scopes) {
            tracked_free(impl->scopes);
        }

        for (int c = 0; c < DECOMP_CACHE_SIZE; c++) {
            if (impl->cache[c].symbols) {
                tracked_free(impl->cache[c].symbols);
            }
        }

        if (impl->pool) {
            string_pool_destroy(impl->pool);
        }
        if (impl->type_table) {
            type_table_destroy(impl->type_table);
        }

        tracked_free(impl);
    }
    tracked_free(self);
}

symtab_t* symtab_twotier_create_with_policy(int hot_depth, size_t ram_budget, twotier_demote_policy_t policy) {
    symtab_t* tab = (symtab_t*)tracked_malloc(sizeof(symtab_t));
    if (!tab) return NULL;
    memset(tab, 0, sizeof(symtab_t));

    twotier_impl_t* impl = (twotier_impl_t*)tracked_malloc(sizeof(twotier_impl_t));
    if (!impl) {
        tracked_free(tab);
        return NULL;
    }
    memset(impl, 0, sizeof(twotier_impl_t));

    impl->hot_depth = (hot_depth > 0) ? hot_depth : 2;
    impl->ram_budget = ram_budget;
    impl->demote_policy = policy;
    impl->scope_depth = -1;
    impl->bloom_enabled = true;
    impl->range_enabled = true;

    impl->pool = string_pool_create(32);
    impl->type_table = type_table_create();

    impl->scope_capacity = TWOTIER_INITIAL_SCOPE_CAPACITY;
    impl->scopes = (twotier_scope_t*)tracked_calloc(impl->scope_capacity, sizeof(twotier_scope_t));

    size_t hot_size = HOT_TABLE_INITIAL_SIZE;
    impl->hot_mask = hot_size - 1;
    impl->hot_slots = (hot_slot_t*)tracked_calloc(hot_size, sizeof(hot_slot_t));

    tab->kind = SYMTAB_TWOTIER;
    tab->name = "Two-Tier (Scope-Aware Compressed)";
    tab->impl = impl;
    tab->enter_scope = twotier_enter_scope;
    tab->exit_scope = twotier_exit_scope;
    tab->insert = twotier_insert;
    tab->lookup = twotier_lookup;
    tab->lookup_current_scope = twotier_lookup_current_scope;
    tab->get_scope_depth = twotier_get_scope_depth;
    tab->get_stats = twotier_get_stats;
    tab->set_param = twotier_set_param;
    tab->destroy = twotier_destroy;

    return tab;
}

symtab_t* symtab_twotier_create(int hot_depth, size_t ram_budget) {
    return symtab_twotier_create_with_policy(hot_depth, ram_budget, DEMOTE_POLICY_DEPTH_BOUNDED);
}

void symtab_twotier_get_memory_breakdown(symtab_t* self, twotier_memory_breakdown_t* bd) {
    if (!self || !bd) return;
    memset(bd, 0, sizeof(*bd));
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    if (!impl) return;

    /* 1. Hot table */
    bd->hot_table_bytes = (impl->hot_mask + 1) * sizeof(hot_slot_t);

    /* 2. Cold blocks */
    size_t cold_blocks_data = 0;
    size_t cold_blocks_headers = 0;
    for (int d = 0; d <= impl->scope_depth; d++) {
        if (!impl->scopes[d].is_hot && impl->scopes[d].cold_block) {
            cold_blocks_data += impl->scopes[d].cold_block->compressed_bytes;
            cold_blocks_headers += sizeof(compressed_scope_block_t);
        }
    }
    bd->cold_blocks_bytes = cold_blocks_data;

    /* 3. String pool */
    if (impl->pool) {
        bd->string_pool_bytes = string_pool_get_bytes_used(impl->pool);
    }

    /* 4. Type table */
    if (impl->type_table) {
        bd->type_table_bytes = type_table_get_bytes_used(impl->type_table);
    }

    /* 5. Arenas */
    size_t arena_bytes = 0;
    for (int d = 0; d <= impl->scope_depth; d++) {
        if (impl->scopes[d].is_hot && impl->scopes[d].arena) {
            arena_bytes += arena_get_bytes_allocated(impl->scopes[d].arena);
        }
    }
    bd->arena_bytes = arena_bytes;

    /* 6. Metadata */
    size_t meta = sizeof(symtab_t) + sizeof(twotier_impl_t);
    meta += impl->scope_capacity * sizeof(twotier_scope_t);
    meta += cold_blocks_headers;
    for (int c = 0; c < DECOMP_CACHE_SIZE; c++) {
        if (impl->cache[c].symbols) {
            meta += sizeof(packed_symbol_t) * impl->cache[c].capacity;
        }
    }
    bd->metadata_bytes = meta;

    bd->total_tracked_bytes = mem_tracker_get_current();
}

void symtab_twotier_get_compression_stats(symtab_t* self, twotier_compression_stats_t* stats) {
    if (!self || !stats) return;
    memset(stats, 0, sizeof(*stats));
    twotier_impl_t* impl = (twotier_impl_t*)self->impl;
    if (!impl) return;

    size_t total_syms = 0;
    size_t comp_bytes = 0;
    for (int d = 0; d <= impl->scope_depth; d++) {
        if (!impl->scopes[d].is_hot && impl->scopes[d].cold_block) {
            total_syms += impl->scopes[d].cold_block->symbol_count;
            comp_bytes += impl->scopes[d].cold_block->compressed_bytes;
        }
    }
    stats->total_cold_symbols = total_syms;
    stats->raw_packed_bytes = total_syms * sizeof(packed_symbol_t);
    stats->compressed_bytes = comp_bytes;
    if (comp_bytes > 0) {
        stats->compression_ratio = (double)stats->raw_packed_bytes / (double)comp_bytes;
    } else {
        stats->compression_ratio = 1.0;
    }
    if (total_syms > 0) {
        stats->bits_per_symbol = (double)comp_bytes * 8.0 / (double)total_syms;
    } else {
        stats->bits_per_symbol = 64.0;
    }
}
