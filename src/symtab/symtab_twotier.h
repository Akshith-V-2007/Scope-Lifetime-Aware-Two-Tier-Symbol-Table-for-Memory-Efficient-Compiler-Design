#ifndef SYMTAB_TWOTIER_H
#define SYMTAB_TWOTIER_H

#include "symtab_api.h"
#include "../common/string_pool.h"
#include "../common/type_table.h"
#include "../common/arena.h"
#include "../common/scope_block.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DEMOTE_POLICY_DEPTH_BOUNDED = 0, /* Demote scopes deeper than K (Default) */
    DEMOTE_POLICY_ON_SCOPE_EXIT = 1, /* Keep open scopes hot; demote upon exit */
    DEMOTE_POLICY_SIZE_AWARE    = 2  /* Demote scopes deeper than K only if >= 16 symbols */
} twotier_demote_policy_t;

symtab_t* symtab_twotier_create(int hot_depth, size_t ram_budget);
symtab_t* symtab_twotier_create_with_policy(int hot_depth, size_t ram_budget, twotier_demote_policy_t policy);

typedef struct {
    size_t hot_table_bytes;
    size_t cold_blocks_bytes;
    size_t string_pool_bytes;
    size_t type_table_bytes;
    size_t arena_bytes;
    size_t metadata_bytes;
    size_t total_tracked_bytes;
} twotier_memory_breakdown_t;

void symtab_twotier_get_memory_breakdown(symtab_t* self, twotier_memory_breakdown_t* bd);

typedef struct {
    size_t total_cold_symbols;
    size_t raw_packed_bytes;
    size_t compressed_bytes;
    double compression_ratio;
    double bits_per_symbol;
} twotier_compression_stats_t;

void symtab_twotier_get_compression_stats(symtab_t* self, twotier_compression_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif /* SYMTAB_TWOTIER_H */
