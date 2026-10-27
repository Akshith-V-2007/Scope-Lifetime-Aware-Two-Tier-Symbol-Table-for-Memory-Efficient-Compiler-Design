#include "scope_block.h"
#include "varint.h"
#include "mem_tracker.h"
#include <stdlib.h>
#include <string.h>

static int compare_packed_symbols(const void* a, const void* b) {
    const packed_symbol_t* sa = (const packed_symbol_t*)a;
    const packed_symbol_t* sb = (const packed_symbol_t*)b;
    if (sa->name_id < sb->name_id) return -1;
    if (sa->name_id > sb->name_id) return 1;
    return 0;
}

compressed_scope_block_t* scope_block_create(scope_id_t scope_id, uint8_t depth) {
    compressed_scope_block_t* block = (compressed_scope_block_t*)tracked_malloc(sizeof(compressed_scope_block_t));
    if (!block) return NULL;
    memset(block, 0, sizeof(compressed_scope_block_t));
    block->scope_id = scope_id;
    block->scope_depth = depth;
    bloom_init(&block->bloom);
    return block;
}

void scope_block_destroy(compressed_scope_block_t* block) {
    if (!block) return;
    if (block->compressed_data) {
        tracked_free(block->compressed_data);
    }
    tracked_free(block);
}

bool scope_block_compress(compressed_scope_block_t* block, const packed_symbol_t* symbols, size_t count) {
    if (!block) return false;
    if (count == 0 || !symbols) {
        block->symbol_count = 0;
        block->compressed_bytes = 0;
        block->compressed_data = NULL;
        return true;
    }

    /* Temporary buffer for sorting */
    packed_symbol_t* sorted = (packed_symbol_t*)tracked_malloc(sizeof(packed_symbol_t) * count);
    if (!sorted) return false;
    memcpy(sorted, symbols, sizeof(packed_symbol_t) * count);
    qsort(sorted, count, sizeof(packed_symbol_t), compare_packed_symbols);

    /* Build Bloom filter and compute min/max name_id */
    bloom_init(&block->bloom);
    block->min_name_id = sorted[0].name_id;
    block->max_name_id = sorted[count - 1].name_id;

    for (size_t i = 0; i < count; i++) {
        bloom_add(&block->bloom, sorted[i].name_id);
    }

    /* Allocate scratch compression buffer: at most ~16 bytes per symbol */
    size_t scratch_size = count * 16 + 64;
    uint8_t* scratch = (uint8_t*)tracked_malloc(scratch_size);
    if (!scratch) {
        tracked_free(sorted);
        return false;
    }

    size_t out_pos = 0;
    name_id_t prev_name_id = 0;

    for (size_t i = 0; i < count; i++) {
        /* 1. Delta-encode name_id */
        uint32_t delta_id;
        if (i == 0) {
            delta_id = sorted[0].name_id;
        } else {
            delta_id = (uint32_t)(sorted[i].name_id - prev_name_id);
        }
        prev_name_id = sorted[i].name_id;
        out_pos += varint_encode_u32(scratch + out_pos, delta_id);

        /* 2. Kind and storage class (1 byte) */
        scratch[out_pos++] = sorted[i].kind_and_storage;

        /* 3. Scope depth (1 byte) */
        scratch[out_pos++] = sorted[i].scope_depth;

        /* 4. Type ID as varint */
        out_pos += varint_encode_u32(scratch + out_pos, sorted[i].type_id);

        /* 5. Offset as zigzag varint */
        out_pos += varint_encode_i32(scratch + out_pos, sorted[i].offset);
    }

    /* Store compressed data tightly */
    block->compressed_data = (uint8_t*)tracked_malloc(out_pos);
    if (!block->compressed_data) {
        tracked_free(scratch);
        tracked_free(sorted);
        return false;
    }
    memcpy(block->compressed_data, scratch, out_pos);
    block->compressed_bytes = (uint32_t)out_pos;
    block->symbol_count = (uint16_t)count;

    tracked_free(scratch);
    tracked_free(sorted);
    return true;
}

size_t scope_block_decompress(const compressed_scope_block_t* block, packed_symbol_t* out_symbols, size_t max_count) {
    if (!block || !block->compressed_data || block->symbol_count == 0) {
        return 0;
    }

    size_t in_pos = 0;
    size_t max_in = block->compressed_bytes;
    name_id_t prev_name_id = 0;
    size_t count = 0;

    while (in_pos < max_in && count < max_count) {
        /* 1. Decode delta name_id */
        uint32_t delta_id = 0;
        size_t b = varint_decode_u32(block->compressed_data + in_pos, max_in - in_pos, &delta_id);
        if (b == 0) break;
        in_pos += b;

        name_id_t name_id;
        if (count == 0) {
            name_id = (name_id_t)delta_id;
        } else {
            name_id = (name_id_t)(prev_name_id + delta_id);
        }
        prev_name_id = name_id;
        out_symbols[count].name_id = name_id;

        /* 2. Kind & storage */
        if (in_pos >= max_in) break;
        out_symbols[count].kind_and_storage = block->compressed_data[in_pos++];

        /* 3. Scope depth */
        if (in_pos >= max_in) break;
        out_symbols[count].scope_depth = block->compressed_data[in_pos++];

        /* 4. Type ID */
        uint32_t type_id = 0;
        b = varint_decode_u32(block->compressed_data + in_pos, max_in - in_pos, &type_id);
        if (b == 0) break;
        in_pos += b;
        out_symbols[count].type_id = (uint16_t)type_id;

        /* 5. Offset */
        int32_t offset = 0;
        b = varint_decode_i32(block->compressed_data + in_pos, max_in - in_pos, &offset);
        if (b == 0) break;
        in_pos += b;
        out_symbols[count].offset = (int16_t)offset;

        count++;
    }

    return count;
}

const packed_symbol_t* scope_symbols_find(const packed_symbol_t* symbols, size_t count, name_id_t target_name_id) {
    if (!symbols || count == 0) return NULL;

    size_t low = 0;
    size_t high = count;

    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (symbols[mid].name_id == target_name_id) {
            return &symbols[mid];
        }
        if (symbols[mid].name_id < target_name_id) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }

    return NULL;
}
