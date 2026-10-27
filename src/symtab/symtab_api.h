#ifndef SYMTAB_API_H
#define SYMTAB_API_H

#include "../common/types.h"
#include "../common/mem_tracker.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SYMTAB_BASELINE = 0,
    SYMTAB_TWOTIER,
    SYMTAB_ARENA_ONLY,
    SYMTAB_INTERN_ONLY,
    SYMTAB_PACKED_ONLY,
    SYMTAB_COMPRESSED_NO_BLOOM
} symtab_kind_t;

typedef struct {
    uint64_t total_inserts;
    uint64_t total_lookups;
    uint64_t hot_hits;
    uint64_t cold_hits;
    uint64_t misses;
    uint64_t range_skips;
    uint64_t bloom_skips;
    uint64_t bloom_false_positives;
    uint64_t decompressions;
    uint64_t cache_hits;
    uint64_t cache_misses;

    size_t current_symbols;
    size_t peak_symbols;
    size_t current_bytes;
    size_t peak_bytes;
    size_t total_scopes_created;
    size_t total_scopes_destroyed;
    int    current_scope_depth;
    int    max_scope_depth;
} symtab_stats_t;

typedef struct symtab symtab_t;

/* Pluggable Symbol Table Interface */
struct symtab {
    symtab_kind_t kind;
    const char*   name;
    void*         impl; /* Pointer to specific implementation struct */

    void (*enter_scope)(symtab_t* self);
    void (*exit_scope)(symtab_t* self);
    bool (*insert)(symtab_t* self, const char* name, type_id_t type_id, symbol_kind_t kind, int32_t offset);
    const symbol_record_t* (*lookup)(symtab_t* self, const char* name);
    const symbol_record_t* (*lookup_current_scope)(symtab_t* self, const char* name);
    int  (*get_scope_depth)(symtab_t* self);
    void (*get_stats)(symtab_t* self, symtab_stats_t* stats);
    void (*set_param)(symtab_t* self, const char* key, int value);
    void (*destroy)(symtab_t* self);
};

/* Factory Functions */
symtab_t* symtab_baseline_create(void);
symtab_t* symtab_twotier_create(int hot_depth, size_t ram_budget);
symtab_t* symtab_ablation_create(symtab_kind_t variant);

#ifdef __cplusplus
}
#endif

#endif /* SYMTAB_API_H */
