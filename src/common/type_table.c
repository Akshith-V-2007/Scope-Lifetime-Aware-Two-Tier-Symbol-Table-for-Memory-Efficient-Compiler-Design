#include "type_table.h"
#include "mem_tracker.h"
#include <string.h>
#include <stdio.h>

#define INITIAL_TYPE_CAPACITY 16

struct type_table {
    type_info_t* types;
    size_t count;
    size_t capacity;

    /* Pre-registered primitives */
    type_id_t void_id;
    type_id_t int_id;
    type_id_t float_id;
    type_id_t char_id;
};

type_table_t* type_table_create(void) {
    type_table_t* table = (type_table_t*)tracked_malloc(sizeof(type_table_t));
    if (!table) return NULL;
    memset(table, 0, sizeof(type_table_t));

    table->capacity = INITIAL_TYPE_CAPACITY;
    table->types = (type_info_t*)tracked_malloc(sizeof(type_info_t) * table->capacity);
    table->count = 1; /* 0 is TYPE_ID_INVALID */
    memset(&table->types[0], 0, sizeof(type_info_t));

    /* Register built-ins */
    table->void_id = type_get_primitive(table, TYPE_VOID);
    table->int_id = type_get_primitive(table, TYPE_INT);
    table->float_id = type_get_primitive(table, TYPE_FLOAT);
    table->char_id = type_get_primitive(table, TYPE_CHAR);

    return table;
}

void type_table_destroy(type_table_t* table) {
    if (!table) return;

    for (size_t i = 1; i < table->count; i++) {
        if (table->types[i].kind == TYPE_STRUCT_T && table->types[i].as.struct_info.fields) {
            tracked_free(table->types[i].as.struct_info.fields);
        }
    }
    if (table->types) {
        tracked_free(table->types);
    }
    tracked_free(table);
}

static type_id_t register_type(type_table_t* table, const type_info_t* info) {
    if (table->count >= table->capacity) {
        size_t new_cap = table->capacity * 2;
        table->types = (type_info_t*)tracked_realloc(table->types, sizeof(type_info_t) * new_cap);
        table->capacity = new_cap;
    }
    type_id_t id = (type_id_t)table->count++;
    table->types[id] = *info;
    table->types[id].id = id;
    return id;
}

type_id_t type_get_primitive(type_table_t* table, type_kind_t kind) {
    /* Check if already registered */
    for (size_t i = 1; i < table->count; i++) {
        if (table->types[i].kind == kind) {
            return (type_id_t)i;
        }
    }

    type_info_t info = {0};
    info.kind = kind;
    switch (kind) {
        case TYPE_VOID:  info.size_bytes = 0; info.alignment = 1; break;
        case TYPE_INT:   info.size_bytes = 4; info.alignment = 4; break;
        case TYPE_FLOAT: info.size_bytes = 4; info.alignment = 4; break;
        case TYPE_CHAR:  info.size_bytes = 1; info.alignment = 1; break;
        default:         info.size_bytes = 4; info.alignment = 4; break;
    }
    return register_type(table, &info);
}

type_id_t type_get_pointer(type_table_t* table, type_id_t pointee) {
    for (size_t i = 1; i < table->count; i++) {
        if (table->types[i].kind == TYPE_POINTER && table->types[i].as.pointee == pointee) {
            return (type_id_t)i;
        }
    }

    type_info_t info = {0};
    info.kind = TYPE_POINTER;
    info.size_bytes = 8; /* 64-bit target */
    info.alignment = 8;
    info.as.pointee = pointee;
    return register_type(table, &info);
}

type_id_t type_get_array(type_table_t* table, type_id_t elem_type, size_t length) {
    for (size_t i = 1; i < table->count; i++) {
        if (table->types[i].kind == TYPE_ARRAY_T &&
            table->types[i].as.array_info.elem_type == elem_type &&
            table->types[i].as.array_info.length == length) {
            return (type_id_t)i;
        }
    }

    const type_info_t* elem = type_table_lookup(table, elem_type);
    size_t elem_size = elem ? elem->size_bytes : 4;

    type_info_t info = {0};
    info.kind = TYPE_ARRAY_T;
    info.size_bytes = elem_size * length;
    info.alignment = elem ? elem->alignment : 4;
    info.as.array_info.elem_type = elem_type;
    info.as.array_info.length = length;
    return register_type(table, &info);
}

type_id_t type_define_struct(type_table_t* table, name_id_t name_id, size_t field_count, const struct_field_t* fields) {
    /* Check if already defined with this name_id */
    for (size_t i = 1; i < table->count; i++) {
        if (table->types[i].kind == TYPE_STRUCT_T && table->types[i].as.struct_info.struct_name_id == name_id) {
            return (type_id_t)i;
        }
    }

    type_info_t info = {0};
    info.kind = TYPE_STRUCT_T;
    info.as.struct_info.struct_name_id = name_id;
    info.as.struct_info.field_count = field_count;

    if (field_count > 0 && fields) {
        info.as.struct_info.fields = (struct_field_t*)tracked_malloc(sizeof(struct_field_t) * field_count);
        memcpy(info.as.struct_info.fields, fields, sizeof(struct_field_t) * field_count);
    } else {
        info.as.struct_info.fields = NULL;
    }

    /* Compute struct size & layout */
    size_t current_offset = 0;
    size_t max_align = 1;
    for (size_t f = 0; f < field_count; f++) {
        const type_info_t* ftype = type_table_lookup(table, info.as.struct_info.fields[f].type_id);
        size_t falign = ftype ? ftype->alignment : 4;
        size_t fsize = ftype ? ftype->size_bytes : 4;
        if (falign > max_align) max_align = falign;

        /* Align offset */
        if (current_offset % falign != 0) {
            current_offset += falign - (current_offset % falign);
        }
        info.as.struct_info.fields[f].offset = (int32_t)current_offset;
        current_offset += fsize;
    }

    /* Pad to multiple of max_align */
    if (max_align > 0 && current_offset % max_align != 0) {
        current_offset += max_align - (current_offset % max_align);
    }

    info.size_bytes = current_offset;
    info.alignment = max_align;

    return register_type(table, &info);
}

const type_info_t* type_table_lookup(const type_table_t* table, type_id_t id) {
    if (!table || id == 0 || id >= table->count) return NULL;
    return &table->types[id];
}

const struct_field_t* type_lookup_field(const type_table_t* table, type_id_t struct_type_id, name_id_t field_name_id) {
    const type_info_t* t = type_table_lookup(table, struct_type_id);
    if (!t || t->kind != TYPE_STRUCT_T) return NULL;

    for (size_t i = 0; i < t->as.struct_info.field_count; i++) {
        if (t->as.struct_info.fields[i].name_id == field_name_id) {
            return &t->as.struct_info.fields[i];
        }
    }
    return NULL;
}

const char* type_to_string(const type_table_t* table, type_id_t id, char* buf, size_t buf_size) {
    if (!table || !buf || buf_size == 0) return "";
    const type_info_t* t = type_table_lookup(table, id);
    if (!t) {
        snprintf(buf, buf_size, "unknown_type(%u)", id);
        return buf;
    }

    switch (t->kind) {
        case TYPE_VOID:  snprintf(buf, buf_size, "void"); break;
        case TYPE_INT:   snprintf(buf, buf_size, "int"); break;
        case TYPE_FLOAT: snprintf(buf, buf_size, "float"); break;
        case TYPE_CHAR:  snprintf(buf, buf_size, "char"); break;
        case TYPE_POINTER: {
            char sub[64];
            type_to_string(table, t->as.pointee, sub, sizeof(sub));
            snprintf(buf, buf_size, "%s*", sub);
            break;
        }
        case TYPE_ARRAY_T: {
            char sub[64];
            type_to_string(table, t->as.array_info.elem_type, sub, sizeof(sub));
            snprintf(buf, buf_size, "%s[%zu]", sub, t->as.array_info.length);
            break;
        }
        case TYPE_STRUCT_T: {
            snprintf(buf, buf_size, "struct#%u", t->as.struct_info.struct_name_id);
            break;
        }
        default:
            snprintf(buf, buf_size, "type#%u", id);
            break;
    }
    return buf;
}

size_t type_table_get_bytes_used(const type_table_t* table) {
    if (!table) return 0;
    size_t bytes = sizeof(type_table_t);
    bytes += table->capacity * sizeof(type_info_t);
    for (size_t i = 1; i < table->count; i++) {
        if (table->types[i].kind == TYPE_STRUCT_T && table->types[i].as.struct_info.fields) {
            bytes += table->types[i].as.struct_info.field_count * sizeof(struct_field_t);
        }
    }
    return bytes;
}
