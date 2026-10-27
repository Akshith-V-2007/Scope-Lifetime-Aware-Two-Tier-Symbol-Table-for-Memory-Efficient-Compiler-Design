#ifndef BLOOM_H
#define BLOOM_H

#include "types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t bloom64_t;

static inline void bloom_init(bloom64_t* bf) {
    *bf = 0;
}

static inline void bloom_add(bloom64_t* bf, name_id_t id) {
    uint32_t x = (uint32_t)id;
    uint32_t h1 = (x * 0x9E3779B9u) >> 26;       /* 6 bits: 0..63 */
    uint32_t h2 = (x * 0x85EBCA6Bu) >> 26;
    uint32_t h3 = (x * 0xC2B2AE35u) >> 26;

    *bf |= ((uint64_t)1 << (h1 & 63));
    *bf |= ((uint64_t)1 << (h2 & 63));
    *bf |= ((uint64_t)1 << (h3 & 63));
}

static inline bool bloom_test(bloom64_t bf, name_id_t id) {
    uint32_t x = (uint32_t)id;
    uint32_t h1 = (x * 0x9E3779B9u) >> 26;
    uint32_t h2 = (x * 0x85EBCA6Bu) >> 26;
    uint32_t h3 = (x * 0xC2B2AE35u) >> 26;

    uint64_t mask = ((uint64_t)1 << (h1 & 63)) |
                    ((uint64_t)1 << (h2 & 63)) |
                    ((uint64_t)1 << (h3 & 63));

    return (bf & mask) == mask;
}

typedef struct {
    uint64_t total_tests;
    uint64_t skips;             /* True negative: bloom_test returned false */
    uint64_t potential_hits;    /* bloom_test returned true */
    uint64_t verified_hits;     /* Actually found in decompressed block */
    uint64_t false_positives;   /* bloom returned true, but symbol was NOT present */
} bloom_stats_t;

#ifdef __cplusplus
}
#endif

#endif /* BLOOM_H */
