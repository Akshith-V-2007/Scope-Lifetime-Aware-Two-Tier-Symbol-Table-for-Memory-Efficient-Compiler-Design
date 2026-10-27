#ifndef SCOPE_BLOCK_H
#define SCOPE_BLOCK_H

#include "types.h"
#include "bloom.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    scope_id_t  scope_id;
    uint8_t     scope_depth;
    uint16_t    symbol_count;
    uint32_t    compressed_bytes;
    bloom64_t   bloom;
    name_id_t   min_name_id;
    name_id_t   max_name_id;
    uint8_t*    compressed_data;
} compressed_scope_block_t;

/* Scratch decompression cache entry */
typedef struct {
    scope_id_t      cached_scope_id;
    size_t          symbol_count;
    packed_symbol_t* symbols;
    size_t          capacity;
    uint64_t        last_access_tick;
} decomp_cache_entry_t;

compressed_scope_block_t* scope_block_create(scope_id_t scope_id, uint8_t depth);
void                      scope_block_destroy(compressed_scope_block_t* block);

bool   scope_block_compress(compressed_scope_block_t* block, const packed_symbol_t* symbols, size_t count);
size_t scope_block_decompress(const compressed_scope_block_t* block, packed_symbol_t* out_symbols, size_t max_count);

/* Binary search inside an uncompressed array of symbols sorted by name_id */
const packed_symbol_t* scope_symbols_find(const packed_symbol_t* symbols, size_t count, name_id_t target_name_id);

#ifdef __cplusplus
}
#endif

#endif /* SCOPE_BLOCK_H */
