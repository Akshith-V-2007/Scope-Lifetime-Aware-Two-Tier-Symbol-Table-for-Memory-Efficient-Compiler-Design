#include "symtab_ablation.h"
#include "symtab_twotier.h"
#include "../common/arena.h"
#include "../common/string_pool.h"
#include "../common/mem_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_ABLATION_SCOPES 256

/* Linked node used for uncompressed/unpacked variants (Arena-Only and Intern-Only) */
typedef struct arena_node {
    const char* name;
    type_id_t type_id;
    symbol_kind_t kind;
    int32_t offset;
    uint32_t scope_depth;
    struct arena_node* next;
} arena_node_t;

typedef struct {
    arena_t* arena;
    /* For Arena-Only and Intern-Only: chained hash table */
    arena_node_t** buckets;
    size_t bucket_count;
    /* For Packed-Only: contiguous array of 8-byte packed symbols */
    packed_symbol_t* packed_symbols;
    size_t packed_capacity;
    size_t symbol_count;
} arena_scope_t;

typedef struct {
    arena_scope_t* scopes;
    size_t scope_capacity;
    int scope_depth;
    string_pool_t* pool; /* Used if interning or packing is enabled */
    bool use_interning;
    bool use_packing;
    symtab_stats_t stats;
    symbol_record_t cached_result;
} ablation_impl_t;

static uint32_t ablation_hash(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) {
        h ^= (uint8_t)(*s++);
        h *= 16777619u;
    }
    return h;
}

static void ablation_enter_scope(symtab_t* self) {
    ablation_impl_t* impl = (ablation_impl_t*)self->impl;
    impl->scope_depth++;
    int d = impl->scope_depth;
    if ((size_t)d >= impl->scope_capacity) {
        size_t new_cap = impl->scope_capacity * 2;
        arena_scope_t* new_scopes = (arena_scope_t*)tracked_realloc(impl->scopes, sizeof(arena_scope_t) * new_cap);
        if (new_scopes) {
            memset(&new_scopes[impl->scope_capacity], 0, sizeof(arena_scope_t) * (new_cap - impl->scope_capacity));
            impl->scopes = new_scopes;
            impl->scope_capacity = new_cap;
        }
    }
    arena_scope_t* s = &impl->scopes[d];

    size_t chunk_size = impl->use_packing ? 128 : 256;
    s->arena = arena_create(chunk_size);
    s->symbol_count = 0;

    if (impl->use_packing) {
        s->packed_capacity = 16;
        s->packed_symbols = (packed_symbol_t*)arena_alloc(s->arena, sizeof(packed_symbol_t) * s->packed_capacity);
        s->buckets = NULL;
        s->bucket_count = 0;
    } else {
        s->bucket_count = 16;
        s->buckets = (arena_node_t**)arena_alloc(s->arena, sizeof(arena_node_t*) * s->bucket_count);
        memset(s->buckets, 0, sizeof(arena_node_t*) * s->bucket_count);
        s->packed_symbols = NULL;
        s->packed_capacity = 0;
    }

    impl->stats.total_scopes_created++;
    impl->stats.current_scope_depth = d;
    if (d > impl->stats.max_scope_depth) impl->stats.max_scope_depth = d;
    impl->stats.current_bytes = mem_tracker_get_current();
    if (impl->stats.current_bytes > impl->stats.peak_bytes) impl->stats.peak_bytes = impl->stats.current_bytes;
}

static void ablation_exit_scope(symtab_t* self) {
    ablation_impl_t* impl = (ablation_impl_t*)self->impl;
    if (impl->scope_depth < 0) return;
    int d = impl->scope_depth;
    arena_scope_t* s = &impl->scopes[d];
    if (impl->stats.current_symbols >= s->symbol_count) {
        impl->stats.current_symbols -= s->symbol_count;
    } else {
        impl->stats.current_symbols = 0;
    }
    arena_destroy(s->arena);
    s->arena = NULL;
    s->buckets = NULL;
    s->packed_symbols = NULL;
    s->symbol_count = 0;

    impl->scope_depth--;
    impl->stats.total_scopes_destroyed++;
    impl->stats.current_scope_depth = impl->scope_depth;
    impl->stats.current_bytes = mem_tracker_get_current();
}

static const symbol_record_t* packed_lookup_scope(ablation_impl_t* impl, arena_scope_t* s, name_id_t nid, int depth) {
    (void)depth;
    for (size_t i = 0; i < s->symbol_count; i++) {
        if (s->packed_symbols[i].name_id == nid) {
            unpack_symbol(&s->packed_symbols[i], &impl->cached_result, string_pool_resolve(impl->pool, nid));
            return &impl->cached_result;
        }
    }
    return NULL;
}

static const symbol_record_t* ablation_lookup_current_scope(symtab_t* self, const char* name) {
    ablation_impl_t* impl = (ablation_impl_t*)self->impl;
    if (impl->scope_depth < 0 || !name) return NULL;
    int d = impl->scope_depth;
    arena_scope_t* s = &impl->scopes[d];

    if (impl->use_packing) {
        if (!impl->pool) return NULL;
        name_id_t nid = string_pool_lookup(impl->pool, name);
        if (nid == NAME_ID_INVALID) return NULL;
        return packed_lookup_scope(impl, s, nid, d);
    } else {
        uint32_t h = ablation_hash(name);
        size_t idx = h % s->bucket_count;
        arena_node_t* curr = s->buckets[idx];
        while (curr) {
            if (strcmp(curr->name, name) == 0) {
                impl->cached_result.name = curr->name;
                impl->cached_result.kind = curr->kind;
                impl->cached_result.type_id = curr->type_id;
                impl->cached_result.offset = curr->offset;
                impl->cached_result.scope_depth = curr->scope_depth;
                return &impl->cached_result;
            }
            curr = curr->next;
        }
    }
    return NULL;
}

static bool ablation_insert(symtab_t* self, const char* name, type_id_t type_id, symbol_kind_t kind, int32_t offset) {
    ablation_impl_t* impl = (ablation_impl_t*)self->impl;
    if (impl->scope_depth < 0 || !name) return false;
    if (ablation_lookup_current_scope(self, name) != NULL) return false;

    int d = impl->scope_depth;
    arena_scope_t* s = &impl->scopes[d];

    if (impl->use_packing) {
        if (!impl->pool) return false;
        name_id_t nid = string_pool_intern(impl->pool, name);
        if (s->symbol_count >= s->packed_capacity) {
            size_t new_cap = s->packed_capacity * 2;
            packed_symbol_t* new_syms = (packed_symbol_t*)arena_alloc(s->arena, sizeof(packed_symbol_t) * new_cap);
            if (!new_syms) return false;
            memcpy(new_syms, s->packed_symbols, sizeof(packed_symbol_t) * s->symbol_count);
            s->packed_symbols = new_syms;
            s->packed_capacity = new_cap;
        }
        symbol_record_t rec;
        memset(&rec, 0, sizeof(rec));
        rec.name_id = nid;
        rec.kind = kind;
        rec.storage = STORAGE_AUTO;
        rec.type_id = type_id;
        rec.offset = offset;
        rec.scope_depth = (uint32_t)d;
        s->packed_symbols[s->symbol_count++] = pack_symbol(&rec);
    } else {
        arena_node_t* node = (arena_node_t*)arena_alloc(s->arena, sizeof(arena_node_t));
        if (!node) return false;

        if (impl->use_interning && impl->pool) {
            name_id_t nid = string_pool_intern(impl->pool, name);
            node->name = string_pool_resolve(impl->pool, nid);
        } else {
            /* Store string in scope arena (uninterned) */
            size_t len = strlen(name) + 1;
            char* str_copy = (char*)arena_alloc(s->arena, len);
            if (!str_copy) return false;
            memcpy(str_copy, name, len);
            node->name = str_copy;
        }

        node->type_id = type_id;
        node->kind = kind;
        node->offset = offset;
        node->scope_depth = (uint32_t)d;
        uint32_t h = ablation_hash(name);
        size_t idx = h % s->bucket_count;
        node->next = s->buckets[idx];
        s->buckets[idx] = node;
        s->symbol_count++;
    }

    impl->stats.total_inserts++;
    impl->stats.current_symbols++;
    if (impl->stats.current_symbols > impl->stats.peak_symbols) {
        impl->stats.peak_symbols = impl->stats.current_symbols;
    }
    impl->stats.current_bytes = mem_tracker_get_current();
    if (impl->stats.current_bytes > impl->stats.peak_bytes) impl->stats.peak_bytes = impl->stats.current_bytes;
    return true;
}

static const symbol_record_t* ablation_lookup(symtab_t* self, const char* name) {
    ablation_impl_t* impl = (ablation_impl_t*)self->impl;
    if (!name) return NULL;
    impl->stats.total_lookups++;

    if (impl->use_packing) {
        if (!impl->pool) return NULL;
        name_id_t nid = string_pool_lookup(impl->pool, name);
        if (nid == NAME_ID_INVALID) {
            impl->stats.misses++;
            return NULL;
        }
        for (int d = impl->scope_depth; d >= 0; d--) {
            arena_scope_t* s = &impl->scopes[d];
            const symbol_record_t* rec = packed_lookup_scope(impl, s, nid, d);
            if (rec) {
                if (d == impl->scope_depth) impl->stats.hot_hits++;
                else impl->stats.cold_hits++;
                return rec;
            }
        }
    } else {
        for (int d = impl->scope_depth; d >= 0; d--) {
            arena_scope_t* s = &impl->scopes[d];
            uint32_t h = ablation_hash(name);
            size_t idx = h % s->bucket_count;
            arena_node_t* curr = s->buckets[idx];
            while (curr) {
                if (strcmp(curr->name, name) == 0) {
                    if (d == impl->scope_depth) impl->stats.hot_hits++;
                    else impl->stats.cold_hits++;
                    impl->cached_result.name = curr->name;
                    impl->cached_result.kind = curr->kind;
                    impl->cached_result.type_id = curr->type_id;
                    impl->cached_result.offset = curr->offset;
                    impl->cached_result.scope_depth = curr->scope_depth;
                    return &impl->cached_result;
                }
                curr = curr->next;
            }
        }
    }
    impl->stats.misses++;
    return NULL;
}

static int ablation_get_scope_depth(symtab_t* self) {
    return ((ablation_impl_t*)self->impl)->scope_depth;
}

static void ablation_get_stats(symtab_t* self, symtab_stats_t* stats) {
    ablation_impl_t* impl = (ablation_impl_t*)self->impl;
    impl->stats.current_bytes = mem_tracker_get_current();
    if (impl->stats.current_bytes > impl->stats.peak_bytes) impl->stats.peak_bytes = impl->stats.current_bytes;
    *stats = impl->stats;
}

static void ablation_set_param(symtab_t* self, const char* key, int value) {
    (void)self; (void)key; (void)value;
}

static void ablation_destroy(symtab_t* self) {
    if (!self) return;
    ablation_impl_t* impl = (ablation_impl_t*)self->impl;
    if (impl) {
        while (impl->scope_depth >= 0) {
            ablation_exit_scope(self);
        }
        if (impl->pool) string_pool_destroy(impl->pool);
        if (impl->scopes) tracked_free(impl->scopes);
        tracked_free(impl);
    }
    tracked_free(self);
}

symtab_t* symtab_ablation_create(symtab_kind_t variant) {
    if (variant == SYMTAB_COMPRESSED_NO_BLOOM) {
        symtab_t* tab = symtab_twotier_create(2, 0);
        tab->set_param(tab, "bloom", 0);
        tab->kind = SYMTAB_COMPRESSED_NO_BLOOM;
        tab->name = "Ablation: Compressed (No Bloom)";
        return tab;
    }

    symtab_t* tab = (symtab_t*)tracked_malloc(sizeof(symtab_t));
    if (!tab) return NULL;
    memset(tab, 0, sizeof(symtab_t));

    ablation_impl_t* impl = (ablation_impl_t*)tracked_malloc(sizeof(ablation_impl_t));
    if (!impl) { tracked_free(tab); return NULL; }
    memset(impl, 0, sizeof(ablation_impl_t));
    impl->scope_depth = -1;
    impl->scope_capacity = 8;
    impl->scopes = (arena_scope_t*)tracked_calloc(impl->scope_capacity, sizeof(arena_scope_t));

    if (variant == SYMTAB_ARENA_ONLY) {
        tab->kind = SYMTAB_ARENA_ONLY;
        tab->name = "Ablation: Arena Only";
        impl->use_interning = false;
        impl->use_packing = false;
        impl->pool = NULL;
    } else if (variant == SYMTAB_INTERN_ONLY) {
        tab->kind = SYMTAB_INTERN_ONLY;
        tab->name = "Ablation: Arena + Interning";
        impl->use_interning = true;
        impl->use_packing = false;
        impl->pool = string_pool_create(32);
    } else { /* SYMTAB_PACKED_ONLY */
        tab->kind = SYMTAB_PACKED_ONLY;
        tab->name = "Ablation: Arena + Interning + Packed";
        impl->use_interning = true;
        impl->use_packing = true;
        impl->pool = string_pool_create(32);
    }

    tab->impl = impl;
    tab->enter_scope = ablation_enter_scope;
    tab->exit_scope = ablation_exit_scope;
    tab->insert = ablation_insert;
    tab->lookup = ablation_lookup;
    tab->lookup_current_scope = ablation_lookup_current_scope;
    tab->get_scope_depth = ablation_get_scope_depth;
    tab->get_stats = ablation_get_stats;
    tab->set_param = ablation_set_param;
    tab->destroy = ablation_destroy;

    return tab;
}
