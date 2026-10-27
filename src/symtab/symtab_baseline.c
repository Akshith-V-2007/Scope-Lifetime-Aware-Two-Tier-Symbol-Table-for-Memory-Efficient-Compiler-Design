#include "symtab_baseline.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define BASELINE_INITIAL_BUCKETS 64
#define BASELINE_MAX_SCOPES 256

typedef struct baseline_symbol {
    char* name;
    type_id_t type_id;
    symbol_kind_t kind;
    int32_t offset;
    uint32_t scope_depth;
    scope_id_t scope_id;
    struct baseline_symbol* next;
} baseline_symbol_t;

typedef struct baseline_scope {
    scope_id_t scope_id;
    uint32_t depth;
    baseline_symbol_t** buckets;
    size_t bucket_count;
    size_t symbol_count;
} baseline_scope_t;

typedef struct {
    baseline_scope_t* scopes[BASELINE_MAX_SCOPES];
    int scope_depth; /* -1 = empty, 0 = global */
    scope_id_t next_scope_id;
    symtab_stats_t stats;
    symbol_record_t cached_result; /* For returning stable symbol_record_t */
} baseline_impl_t;

static uint32_t baseline_hash(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) {
        h ^= (uint8_t)(*s++);
        h *= 16777619u;
    }
    return h;
}

static baseline_scope_t* scope_create(scope_id_t scope_id, uint32_t depth) {
    baseline_scope_t* s = (baseline_scope_t*)tracked_malloc(sizeof(baseline_scope_t));
    if (!s) return NULL;
    s->scope_id = scope_id;
    s->depth = depth;
    s->bucket_count = BASELINE_INITIAL_BUCKETS;
    s->symbol_count = 0;
    s->buckets = (baseline_symbol_t**)tracked_calloc(s->bucket_count, sizeof(baseline_symbol_t*));
    if (!s->buckets) {
        tracked_free(s);
        return NULL;
    }
    return s;
}

static void scope_destroy(baseline_scope_t* s) {
    if (!s) return;
    for (size_t b = 0; b < s->bucket_count; b++) {
        baseline_symbol_t* curr = s->buckets[b];
        while (curr) {
            baseline_symbol_t* next = curr->next;
            if (curr->name) {
                tracked_free(curr->name);
            }
            tracked_free(curr);
            curr = next;
        }
    }
    tracked_free(s->buckets);
    tracked_free(s);
}

static void baseline_enter_scope(symtab_t* self) {
    baseline_impl_t* impl = (baseline_impl_t*)self->impl;
    if (impl->scope_depth + 1 >= BASELINE_MAX_SCOPES) {
        fprintf(stderr, "ERROR: Exceeded max scope depth in baseline\n");
        return;
    }

    impl->scope_depth++;
    scope_id_t id = ++impl->next_scope_id;
    impl->scopes[impl->scope_depth] = scope_create(id, (uint32_t)impl->scope_depth);
    
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

static void baseline_exit_scope(symtab_t* self) {
    baseline_impl_t* impl = (baseline_impl_t*)self->impl;
    if (impl->scope_depth < 0) return;

    baseline_scope_t* s = impl->scopes[impl->scope_depth];
    if (s) {
        if (impl->stats.current_symbols >= s->symbol_count) {
            impl->stats.current_symbols -= s->symbol_count;
        } else {
            impl->stats.current_symbols = 0;
        }
        scope_destroy(s);
    }
    impl->scopes[impl->scope_depth] = NULL;
    impl->scope_depth--;

    impl->stats.total_scopes_destroyed++;
    impl->stats.current_scope_depth = impl->scope_depth;
    impl->stats.current_bytes = mem_tracker_get_current();
}

static const symbol_record_t* baseline_lookup_current_scope(symtab_t* self, const char* name) {
    baseline_impl_t* impl = (baseline_impl_t*)self->impl;
    if (impl->scope_depth < 0 || !name) return NULL;

    baseline_scope_t* s = impl->scopes[impl->scope_depth];
    if (!s) return NULL;
    uint32_t h = baseline_hash(name);
    size_t idx = h % s->bucket_count;

    baseline_symbol_t* curr = s->buckets[idx];
    while (curr) {
        if (strcmp(curr->name, name) == 0) {
            impl->cached_result.name = curr->name;
            impl->cached_result.name_id = 0;
            impl->cached_result.kind = curr->kind;
            impl->cached_result.storage = STORAGE_AUTO;
            impl->cached_result.type_id = curr->type_id;
            impl->cached_result.offset = curr->offset;
            impl->cached_result.scope_depth = curr->scope_depth;
            impl->cached_result.scope_id = curr->scope_id;
            impl->cached_result.next = NULL;
            return &impl->cached_result;
        }
        curr = curr->next;
    }
    return NULL;
}

static bool baseline_insert(symtab_t* self, const char* name, type_id_t type_id, symbol_kind_t kind, int32_t offset) {
    baseline_impl_t* impl = (baseline_impl_t*)self->impl;
    if (impl->scope_depth < 0 || !name) return false;

    /* Check redeclaration in current scope */
    if (baseline_lookup_current_scope(self, name) != NULL) {
        return false; /* Redeclaration error */
    }

    baseline_scope_t* s = impl->scopes[impl->scope_depth];
    if (!s) return false;
    uint32_t h = baseline_hash(name);
    size_t idx = h % s->bucket_count;

    baseline_symbol_t* sym = (baseline_symbol_t*)tracked_malloc(sizeof(baseline_symbol_t));
    if (!sym) return false;

    sym->name = tracked_strdup(name);
    if (!sym->name) {
        tracked_free(sym);
        return false;
    }
    sym->type_id = type_id;
    sym->kind = kind;
    sym->offset = offset;
    sym->scope_depth = (uint32_t)impl->scope_depth;
    sym->scope_id = s->scope_id;

    sym->next = s->buckets[idx];
    s->buckets[idx] = sym;
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

static const symbol_record_t* baseline_lookup(symtab_t* self, const char* name) {
    baseline_impl_t* impl = (baseline_impl_t*)self->impl;
    if (!name) return NULL;
    impl->stats.total_lookups++;

    /* Walk from innermost scope down to scope 0 */
    for (int d = impl->scope_depth; d >= 0; d--) {
        baseline_scope_t* s = impl->scopes[d];
        if (!s) continue;

        uint32_t h = baseline_hash(name);
        size_t idx = h % s->bucket_count;

        baseline_symbol_t* curr = s->buckets[idx];
        while (curr) {
            if (strcmp(curr->name, name) == 0) {
                if (d == impl->scope_depth) {
                    impl->stats.hot_hits++;
                } else {
                    impl->stats.cold_hits++;
                }

                impl->cached_result.name = curr->name;
                impl->cached_result.name_id = 0;
                impl->cached_result.kind = curr->kind;
                impl->cached_result.storage = STORAGE_AUTO;
                impl->cached_result.type_id = curr->type_id;
                impl->cached_result.offset = curr->offset;
                impl->cached_result.scope_depth = curr->scope_depth;
                impl->cached_result.scope_id = curr->scope_id;
                impl->cached_result.next = NULL;
                return &impl->cached_result;
            }
            curr = curr->next;
        }
    }

    impl->stats.misses++;
    return NULL;
}

static int baseline_get_scope_depth(symtab_t* self) {
    baseline_impl_t* impl = (baseline_impl_t*)self->impl;
    return impl->scope_depth;
}

static void baseline_get_stats(symtab_t* self, symtab_stats_t* stats) {
    baseline_impl_t* impl = (baseline_impl_t*)self->impl;
    impl->stats.current_bytes = mem_tracker_get_current();
    if (impl->stats.current_bytes > impl->stats.peak_bytes) {
        impl->stats.peak_bytes = impl->stats.current_bytes;
    }
    *stats = impl->stats;
}

static void baseline_set_param(symtab_t* self, const char* key, int value) {
    (void)self;
    (void)key;
    (void)value;
}

static void baseline_destroy(symtab_t* self) {
    if (!self) return;
    baseline_impl_t* impl = (baseline_impl_t*)self->impl;
    if (impl) {
        while (impl->scope_depth >= 0) {
            baseline_exit_scope(self);
        }
        tracked_free(impl);
    }
    tracked_free(self);
}

symtab_t* symtab_baseline_create(void) {
    symtab_t* tab = (symtab_t*)tracked_malloc(sizeof(symtab_t));
    if (!tab) return NULL;
    memset(tab, 0, sizeof(symtab_t));

    baseline_impl_t* impl = (baseline_impl_t*)tracked_malloc(sizeof(baseline_impl_t));
    if (!impl) {
        tracked_free(tab);
        return NULL;
    }
    memset(impl, 0, sizeof(baseline_impl_t));
    impl->scope_depth = -1;

    tab->kind = SYMTAB_BASELINE;
    tab->name = "Baseline (Malloc-Chained)";
    tab->impl = impl;
    tab->enter_scope = baseline_enter_scope;
    tab->exit_scope = baseline_exit_scope;
    tab->insert = baseline_insert;
    tab->lookup = baseline_lookup;
    tab->lookup_current_scope = baseline_lookup_current_scope;
    tab->get_scope_depth = baseline_get_scope_depth;
    tab->get_stats = baseline_get_stats;
    tab->set_param = baseline_set_param;
    tab->destroy = baseline_destroy;

    return tab;
}
