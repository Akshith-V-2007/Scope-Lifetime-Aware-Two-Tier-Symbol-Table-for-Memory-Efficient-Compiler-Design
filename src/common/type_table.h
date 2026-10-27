#ifndef TYPE_TABLE_H
#define TYPE_TABLE_H

#include "types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct struct_field {
    name_id_t name_id;
    type_id_t type_id;
    int32_t   offset;
} struct_field_t;

typedef struct type_info {
    type_id_t   id;
    type_kind_t kind;
    size_t      size_bytes;
    size_t      alignment;
    union {
        type_id_t pointee;              /* For TYPE_POINTER */
        struct {
            type_id_t elem_type;
            size_t    length;
        } array_info;                   /* For TYPE_ARRAY_T */
        struct {
            name_id_t       struct_name_id;
            size_t          field_count;
            struct_field_t* fields;
        } struct_info;                  /* For TYPE_STRUCT_T */
    } as;
} type_info_t;

typedef struct type_table type_table_t;

type_table_t*      type_table_create(void);
void               type_table_destroy(type_table_t* table);

type_id_t          type_get_primitive(type_table_t* table, type_kind_t kind);
type_id_t          type_get_pointer(type_table_t* table, type_id_t pointee);
type_id_t          type_get_array(type_table_t* table, type_id_t elem_type, size_t length);
type_id_t          type_define_struct(type_table_t* table, name_id_t name_id, size_t field_count, const struct_field_t* fields);

const type_info_t* type_table_lookup(const type_table_t* table, type_id_t id);
const struct_field_t* type_lookup_field(const type_table_t* table, type_id_t struct_type_id, name_id_t field_name_id);
const char*        type_to_string(const type_table_t* table, type_id_t id, char* buf, size_t buf_size);
size_t             type_table_get_bytes_used(const type_table_t* table);

#ifdef __cplusplus
}
#endif

#endif /* TYPE_TABLE_H */
