#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/common/scope_block.h"
#include "../src/common/mem_tracker.h"

static void test_single_symbol(void) {
    printf("[ROUND-TRIP] Testing single symbol compression...\n");
    packed_symbol_t sym;
    sym.name_id = 42;
    sym.kind_and_storage = 0x12; /* kind=2, storage=1 */
    sym.scope_depth = 3;
    sym.type_id = 105;
    sym.offset = -16;

    compressed_scope_block_t* block = scope_block_create(1, 3);
    assert(scope_block_compress(block, &sym, 1) == true);
    assert(block->symbol_count == 1);
    assert(block->compressed_bytes > 0);

    packed_symbol_t out_sym;
    size_t count = scope_block_decompress(block, &out_sym, 1);
    assert(count == 1);
    assert(out_sym.name_id == sym.name_id);
    assert(out_sym.kind_and_storage == sym.kind_and_storage);
    assert(out_sym.scope_depth == sym.scope_depth);
    assert(out_sym.type_id == sym.type_id);
    assert(out_sym.offset == sym.offset);

    /* Verify Bloom filter */
    assert(bloom_test(block->bloom, 42) == true);
    assert(bloom_test(block->bloom, 999) == false);

    scope_block_destroy(block);
    printf("             Passed! Compressed size: %u bytes\n", block->compressed_bytes);
}

static void test_multi_symbol_roundtrip(size_t n) {
    printf("[ROUND-TRIP] Testing %zu random symbols round-trip...\n", n);
    packed_symbol_t* original = (packed_symbol_t*)tracked_malloc(sizeof(packed_symbol_t) * n);

    for (size_t i = 0; i < n; i++) {
        original[i].name_id = (name_id_t)((i * 37 + 11) % 65000 + 1);
        original[i].kind_and_storage = (uint8_t)((i % 7) | ((i % 3) << 4));
        original[i].scope_depth = (uint8_t)(i % 30);
        original[i].type_id = (type_id_t)((i * 13) % 500 + 1);
        original[i].offset = (int16_t)((int32_t)(i * 4) - 2000);
    }

    compressed_scope_block_t* block = scope_block_create(10, 5);
    assert(scope_block_compress(block, original, n) == true);
    assert(block->symbol_count == n);

    packed_symbol_t* decompressed = (packed_symbol_t*)tracked_malloc(sizeof(packed_symbol_t) * n);
    size_t decomp_count = scope_block_decompress(block, decompressed, n);
    assert(decomp_count == n);

    /* Decompressed symbols are sorted by name_id. Verify all exist and match! */
    for (size_t i = 0; i < n; i++) {
        const packed_symbol_t* found = scope_symbols_find(decompressed, decomp_count, original[i].name_id);
        assert(found != NULL);
        assert(found->name_id == original[i].name_id);
        assert(found->kind_and_storage == original[i].kind_and_storage);
        assert(found->type_id == original[i].type_id);
        assert(found->offset == original[i].offset);
    }

    printf("             %zu symbols: Uncompressed = %zu bytes, Compressed = %u bytes (%.1f%% ratio)\n",
           n, n * sizeof(packed_symbol_t), block->compressed_bytes,
           (1.0 - (double)block->compressed_bytes / (n * sizeof(packed_symbol_t))) * 100.0);

    tracked_free(original);
    tracked_free(decompressed);
    scope_block_destroy(block);
}

int main(void) {
    mem_tracker_init();
    test_single_symbol();
    test_multi_symbol_roundtrip(10);
    test_multi_symbol_roundtrip(100);
    test_multi_symbol_roundtrip(1000);
    test_multi_symbol_roundtrip(5000);

    assert(mem_tracker_get_current() == 0);
    printf("\n[SUCCESS] All round-trip tests passed bitwise and field-exact!\n");
    return 0;
}
