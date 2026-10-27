#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t name_id_t;
typedef uint16_t type_id_t;
typedef uint16_t scope_id_t;

#define NAME_ID_INVALID 0
#define TYPE_ID_INVALID 0

typedef enum {
    KIND_VAR = 0,
    KIND_PARAM,
    KIND_FUNC,
    KIND_STRUCT,
    KIND_FIELD,
    KIND_ARRAY,
    KIND_TYPEDEF
} symbol_kind_t;

typedef enum {
    STORAGE_AUTO = 0,
    STORAGE_STATIC,
    STORAGE_EXTERN,
    STORAGE_GLOBAL
} storage_class_t;

typedef enum {
    TYPE_VOID = 0,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_POINTER,
    TYPE_ARRAY_T,
    TYPE_STRUCT_T,
    TYPE_FUNC_T
} type_kind_t;

/*
 * Bit-packed 64-bit Symbol Record:
 * [0..15]   : name_id      (16 bits) -> up to 65,535 distinct interned identifiers
 * [16..19]  : kind         (4 bits)  -> up to 16 symbol kinds
 * [20..23]  : storage      (4 bits)  -> up to 16 storage classes
 * [24..39]  : type_id      (16 bits) -> up to 65,535 canonical types
 * [40..55]  : offset       (16 bits) -> stack frame / struct / global offset
 * [56..63]  : scope_depth  (8 bits)  -> up to 255 nested scopes
 * TOTAL     : 64 bits = 8 bytes
 */
#pragma pack(push, 1)
typedef struct {
    uint16_t name_id;
    uint8_t  kind_and_storage; /* lower 4 bits kind, upper 4 bits storage */
    uint8_t  scope_depth;
    uint16_t type_id;
    int16_t  offset;
} packed_symbol_t;
#pragma pack(pop)

/* High-level unpacked symbol representation used across APIs */
typedef struct symbol_record {
    const char*      name;
    name_id_t        name_id;
    symbol_kind_t    kind;
    storage_class_t  storage;
    type_id_t        type_id;
    int32_t          offset;
    uint32_t         scope_depth;
    scope_id_t       scope_id;
    struct symbol_record* next; /* Baseline chained hash table use */
} symbol_record_t;

static inline packed_symbol_t pack_symbol(const symbol_record_t* rec) {
    packed_symbol_t p;
    p.name_id = rec->name_id;
    p.kind_and_storage = (uint8_t)(((rec->storage & 0x0F) << 4) | (rec->kind & 0x0F));
    p.scope_depth = (uint8_t)(rec->scope_depth & 0xFF);
    p.type_id = rec->type_id;
    p.offset = (int16_t)(rec->offset & 0xFFFF);
    return p;
}

static inline void unpack_symbol(const packed_symbol_t* p, symbol_record_t* rec, const char* name) {
    rec->name = name;
    rec->name_id = p->name_id;
    rec->kind = (symbol_kind_t)(p->kind_and_storage & 0x0F);
    rec->storage = (storage_class_t)((p->kind_and_storage >> 4) & 0x0F);
    rec->scope_depth = p->scope_depth;
    rec->type_id = p->type_id;
    rec->offset = p->offset;
    rec->next = NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* TYPES_H */
