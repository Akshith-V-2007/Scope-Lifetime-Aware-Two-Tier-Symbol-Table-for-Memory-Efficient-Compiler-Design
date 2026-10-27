#ifndef STRING_POOL_H
#define STRING_POOL_H

#include "types.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct string_pool string_pool_t;

string_pool_t* string_pool_create(size_t initial_capacity);
void           string_pool_destroy(string_pool_t* pool);
name_id_t      string_pool_intern(string_pool_t* pool, const char* str);
name_id_t      string_pool_lookup(const string_pool_t* pool, const char* str);
const char*    string_pool_resolve(const string_pool_t* pool, name_id_t id);
size_t         string_pool_get_count(const string_pool_t* pool);
size_t         string_pool_get_bytes_used(const string_pool_t* pool);

#ifdef __cplusplus
}
#endif

#endif /* STRING_POOL_H */
