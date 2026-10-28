#include "symtab_strong_baseline.h"
#include "../common/arena.h"
#include "../common/string_pool.h"
#include "../common/type_table.h"
#include "../common/mem_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define STRONG_INITIAL_SCOPE_CAPACITY 8
#define STRONG_TABLE_INITIAL_SIZE 32

typedef struct {
    name_id_t       name_id;
    scope_id_t      scope_id;
    packed_symbol_t symbol;
} strong_slot_t;

typedef struct {
    scope_id_t scope_id;
    uint8_t    depth;
    arena_t*   arena;
    size_t     symbol_count;
} strong_scope_t;

typedef struct {
    string_pool_t*  pool;
    type_table_t*   type_table;

    strong_scope_t* scopes;
    size_t          scope_capacity;
    int             scope_depth; /* -1 = empty, 0 = global */
    scope_id_t      next_scope_id;

    /* One-tier open-addressing hash table with linear probing */
    strong_slot_t*  slots;
    size_t          mask;
    size_t          count;

    symbol_record_t cached_result;
    symtab_stats_t  stats;
} strong_baseline_impl_t;

static inline uint32_t hash_strong_key(name_id_t name_id) {
    uint32_t x = (uint32_t)name_id;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

static inline bool is_between_cyclic(size_t a, size_t b, size_t c) {
    if (a <= c) {
        return (a < b && b <= c);
    } else {
        return (a < b || b <= c);
    }
}

static void strong_table_insert(strong_baseline_impl_t* impl, const packed_symbol_t* sym, scope_id_t scope_id) {
    if (!impl->slots) return;

    /* Resize if load factor > 70% */
    if (impl->count * 10 >= (impl->mask + 1) * 7) {
        size_t old_size = impl->mask + 1;
        size_t new_size = old_size * 2;
        strong_slot_t* new_slots = (strong_slot_t*)tracked_calloc(new_size, sizeof(strong_slot_t));
        if (new_slots) {
            size_t new_mask = new_size - 1;
            for (size_t i = 0; i < old_size; i++) {
                if (impl->slots[i].name_id != 0) {
                    uint32_t h = hash_strong_key(impl->slots[i].name_id);
                    size_t idx = h & new_mask;
                    while (new_slots[idx].name_id != 0) {
                        idx = (idx + 1) & new_mask;
                    }
                    new_slots[idx] = impl->slots[i];
                }
            }
            tracked_free(impl->slots);
            impl->slots = new_slots;
            impl->mask = new_mask;
        }
    }

    if (impl->count >= impl->mask + 1) return;

    uint32_t h = hash_strong_key(sym->name_id);
    size_t idx = h & impl->mask;
    size_t start_idx = idx;

    while (impl->slots[idx].name_id != 0) {
        idx = (idx + 1) & impl->mask;
        if (idx == start_idx) return; /* Table full */
    }

    impl->slots[idx].name_id = sym->name_id;
    impl->slots[idx].scope_id = scope_id;
    impl->slots[idx].symbol = *sym;
    impl->count++;
}

static void strong_table_remove_scope(strong_baseline_impl_t* impl, scope_id_t scope_id) {
    size_t cap = impl->mask + 1;
    for (size_t i = 0; i < cap; i++) {
        while (impl->slots[i].name_id != 0 && impl->slots[i].scope_id == scope_id) {
            /* Vacate slot i */
            impl->slots[i].name_id = 0;
            impl->slots[i].scope_id = 0;
            memset(&impl->slots[i].symbol, 0, sizeof(packed_symbol_t));
            impl->count--;

            /* Shift subsequent cluster elements back (Knuth 6.4R) */
            size_t j = i;
            size_t k = (j + 1) & impl->mask;
            while (impl->slots[k].name_id != 0) {
                size_t r = hash_strong_key(impl->slots[k].name_id) & impl->mask;
                if (!is_between_cyclic(j, r, k)) {
                    impl->slots[j] = impl->slots[k];
                    impl->slots[k].name_id = 0;
                    impl->slots[k].scope_id = 0;
                    memset(&impl->slots[k].symbol, 0, sizeof(packed_symbol_t));
                    j = k;
                }
                k = (k + 1) & impl->mask;
            }
        }
    }
}

static void strong_enter_scope(symtab_t* self) {
    strong_baseline_impl_t* impl = (strong_baseline_impl_t*)self->impl;
    impl->scope_depth++;
    int d = impl->scope_depth;

    if ((size_t)d >= impl->scope_capacity) {
        size_t new_cap = impl->scope_capacity * 2;
        strong_scope_t* new_scopes = (strong_scope_t*)tracked_realloc(impl->scopes, sizeof(strong_scope_t) * new_cap);
        if (new_scopes) {
            memset(&new_scopes[impl->scope_capacity], 0, sizeof(strong_scope_t) * (new_cap - impl->scope_capacity));
            impl->scopes = new_scopes;
            impl->scope_capacity = new_cap;
        } else {
            impl->scope_depth--;
            return;
        }
    }

    scope_id_t id = ++impl->next_scope_id;
    impl->scopes[d].scope_id = id;
    impl->scopes[d].depth = (uint8_t)d;
    impl->scopes[d].arena = arena_create(256);
    impl->scopes[d].symbol_count = 0;

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

static void strong_exit_scope(symtab_t* self) {
    strong_baseline_impl_t* impl = (strong_baseline_impl_t*)self->impl;
    if (impl->scope_depth < 0) return;

    int d = impl->scope_depth;
    strong_scope_t* s = &impl->scopes[d];

    if (impl->stats.current_symbols >= s->symbol_count) {
        impl->stats.current_symbols -= s->symbol_count;
    } else {
        impl->stats.current_symbols = 0;
    }

    /* Remove scope symbols from open-addressing table in-place */
    strong_table_remove_scope(impl, s->scope_id);

    /* Bulk free arena for scope in O(1) */
    if (s->arena) {
        arena_destroy(s->arena);
        s->arena = NULL;
    }

    impl->stats.total_scopes_destroyed++;
    impl->scope_depth--;
    impl->stats.current_scope_depth = impl->scope_depth;

    impl->stats.current_bytes = mem_tracker_get_current();
    if (impl->stats.current_bytes > impl->stats.peak_bytes) {
        impl->stats.peak_bytes = impl->stats.current_bytes;
    }
}

static const symbol_record_t* strong_lookup_current_scope(symtab_t* self, const char* name) {
    strong_baseline_impl_t* impl = (strong_baseline_impl_t*)self->impl;
    if (impl->scope_depth < 0 || !name) return NULL;

    name_id_t id = string_pool_lookup(impl->pool, name);
    if (id == NAME_ID_INVALID) return NULL;

    int d = impl->scope_depth;
    scope_id_t cur_scope_id = impl->scopes[d].scope_id;

    uint32_t h = hash_strong_key(id);
    size_t idx = h & impl->mask;
    size_t start_idx = idx;

    while (impl->slots[idx].name_id != 0) {
        if (impl->slots[idx].name_id == id && impl->slots[idx].scope_id == cur_scope_id) {
            unpack_symbol(&impl->slots[idx].symbol, &impl->cached_result, name);
            impl->cached_result.scope_id = cur_scope_id;
            return &impl->cached_result;
        }
        idx = (idx + 1) & impl->mask;
        if (idx == start_idx) break;
    }
    return NULL;
}

static bool strong_insert(symtab_t* self, const char* name, type_id_t type_id, symbol_kind_t kind, int32_t offset) {
    strong_baseline_impl_t* impl = (strong_baseline_impl_t*)self->impl;
    if (impl->scope_depth < 0 || !name) return false;

    if (strong_lookup_current_scope(self, name) != NULL) {
        return false;
    }

    name_id_t nid = string_pool_intern(impl->pool, name);
    if (nid == NAME_ID_INVALID) return false;

    int d = impl->scope_depth;
    strong_scope_t* s = &impl->scopes[d];

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
    strong_table_insert(impl, &ps, s->scope_id);

    s->symbol_count++;
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

static const symbol_record_t* strong_lookup(symtab_t* self, const char* name) {
    strong_baseline_impl_t* impl = (strong_baseline_impl_t*)self->impl;
    if (!name) return NULL;

    impl->stats.total_lookups++;

    name_id_t nid = string_pool_lookup(impl->pool, name);
    if (nid == NAME_ID_INVALID) {
        impl->stats.misses++;
        return NULL;
    }

    const packed_symbol_t* best_sym = NULL;
    uint8_t best_depth = 0;
    scope_id_t best_scope = 0;

    uint32_t h = hash_strong_key(nid);
    size_t idx = h & impl->mask;
    size_t start_idx = idx;

    while (impl->slots[idx].name_id != 0) {
        if (impl->slots[idx].name_id == nid) {
            /* Shadowing: choose highest lexical depth */
            if (!best_sym || impl->slots[idx].symbol.scope_depth >= best_depth) {
                best_sym = &impl->slots[idx].symbol;
                best_depth = impl->slots[idx].symbol.scope_depth;
                best_scope = impl->slots[idx].scope_id;
            }
        }
        idx = (idx + 1) & impl->mask;
        if (idx == start_idx) break;
    }

    if (best_sym) {
        impl->stats.hot_hits++;
        unpack_symbol(best_sym, &impl->cached_result, name);
        impl->cached_result.scope_id = best_scope;
        return &impl->cached_result;
    }

    impl->stats.misses++;
    return NULL;
}

static int strong_get_scope_depth(symtab_t* self) {
    strong_baseline_impl_t* impl = (strong_baseline_impl_t*)self->impl;
    return impl->scope_depth;
}

static void strong_get_stats(symtab_t* self, symtab_stats_t* stats) {
    strong_baseline_impl_t* impl = (strong_baseline_impl_t*)self->impl;
    impl->stats.current_bytes = mem_tracker_get_current();
    if (impl->stats.current_bytes > impl->stats.peak_bytes) {
        impl->stats.peak_bytes = impl->stats.current_bytes;
    }
    *stats = impl->stats;
}

static void strong_set_param(symtab_t* self, const char* key, int value) {
    (void)self;
    (void)key;
    (void)value;
}

static void strong_destroy(symtab_t* self) {
    if (!self) return;
    strong_baseline_impl_t* impl = (strong_baseline_impl_t*)self->impl;
    if (impl) {
        while (impl->scope_depth >= 0) {
            int d = impl->scope_depth;
            strong_scope_t* s = &impl->scopes[d];
            if (s->arena) {
                arena_destroy(s->arena);
                s->arena = NULL;
            }
            impl->scope_depth--;
        }
        if (impl->slots) {
            tracked_free(impl->slots);
        }
        if (impl->scopes) {
            tracked_free(impl->scopes);
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

symtab_t* symtab_strong_baseline_create(void) {
    symtab_t* tab = (symtab_t*)tracked_malloc(sizeof(symtab_t));
    if (!tab) return NULL;
    memset(tab, 0, sizeof(symtab_t));

    strong_baseline_impl_t* impl = (strong_baseline_impl_t*)tracked_malloc(sizeof(strong_baseline_impl_t));
    if (!impl) {
        tracked_free(tab);
        return NULL;
    }
    memset(impl, 0, sizeof(strong_baseline_impl_t));

    impl->scope_capacity = STRONG_INITIAL_SCOPE_CAPACITY;
    impl->scopes = (strong_scope_t*)tracked_calloc(impl->scope_capacity, sizeof(strong_scope_t));
    impl->scope_depth = -1;
    impl->next_scope_id = 0;

    impl->pool = string_pool_create(64);
    impl->type_table = type_table_create();

    size_t initial_slots = STRONG_TABLE_INITIAL_SIZE;
    impl->slots = (strong_slot_t*)tracked_calloc(initial_slots, sizeof(strong_slot_t));
    impl->mask = initial_slots - 1;
    impl->count = 0;

    tab->kind = SYMTAB_STRONG_BASELINE;
    tab->name = "strong_baseline";
    tab->impl = impl;

    tab->enter_scope = strong_enter_scope;
    tab->exit_scope = strong_exit_scope;
    tab->insert = strong_insert;
    tab->lookup = strong_lookup;
    tab->lookup_current_scope = strong_lookup_current_scope;
    tab->get_scope_depth = strong_get_scope_depth;
    tab->get_stats = strong_get_stats;
    tab->set_param = strong_set_param;
    tab->destroy = strong_destroy;

    return tab;
}
