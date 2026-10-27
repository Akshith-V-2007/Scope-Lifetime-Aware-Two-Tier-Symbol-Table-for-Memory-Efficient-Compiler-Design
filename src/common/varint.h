#ifndef VARINT_H
#define VARINT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard unsigned LEB128 varint encode/decode */
size_t varint_encode_u32(uint8_t* out, uint32_t value);
size_t varint_decode_u32(const uint8_t* in, size_t max_len, uint32_t* value);

/* Zigzag encoding maps signed integers to unsigned: 0->0, -1->1, 1->2, -2->3, etc. */
static inline uint32_t zigzag_encode(int32_t n) {
    return (uint32_t)((n << 1) ^ (n >> 31));
}

static inline int32_t zigzag_decode(uint32_t n) {
    return (int32_t)((n >> 1) ^ -(int32_t)(n & 1));
}

size_t varint_encode_i32(uint8_t* out, int32_t value);
size_t varint_decode_i32(const uint8_t* in, size_t max_len, int32_t* value);

#ifdef __cplusplus
}
#endif

#endif /* VARINT_H */
