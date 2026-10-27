#include "varint.h"

size_t varint_encode_u32(uint8_t* out, uint32_t value) {
    size_t i = 0;
    while (value >= 0x80) {
        out[i++] = (uint8_t)((value & 0x7F) | 0x80);
        value >>= 7;
    }
    out[i++] = (uint8_t)(value & 0x7F);
    return i;
}

size_t varint_decode_u32(const uint8_t* in, size_t max_len, uint32_t* value) {
    uint32_t result = 0;
    size_t shift = 0;
    size_t i = 0;

    while (i < max_len) {
        uint8_t byte = in[i++];
        result |= ((uint32_t)(byte & 0x7F)) << shift;
        if (!(byte & 0x80)) {
            *value = result;
            return i;
        }
        shift += 7;
        if (shift >= 35) return 0; /* overflow */
    }
    return 0; /* truncated */
}

size_t varint_encode_i32(uint8_t* out, int32_t value) {
    return varint_encode_u32(out, zigzag_encode(value));
}

size_t varint_decode_i32(const uint8_t* in, size_t max_len, int32_t* value) {
    uint32_t uval = 0;
    size_t bytes = varint_decode_u32(in, max_len, &uval);
    if (bytes > 0) {
        *value = zigzag_decode(uval);
    }
    return bytes;
}
