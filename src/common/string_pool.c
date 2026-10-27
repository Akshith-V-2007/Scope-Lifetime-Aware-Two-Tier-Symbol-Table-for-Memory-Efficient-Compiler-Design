#include "string_pool.h"
#include "mem_tracker.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_STRING_CHUNK_SIZE 512
#define MAX_STRING_CHUNK_SIZE 16384
#define INITIAL_SLOTS 64

typedef struct string_chunk {
    size_t capacity;
    size_t used;
    struct string_chunk* next;
    char data[];
} string_chunk_t;

typedef struct {
    uint32_t hash;
    name_id_t id;
    uint32_t str_offset; /* offset within string array */
} pool_entry_t;

struct string_pool {
    string_chunk_t* chunk_head;
    string_chunk_t* chunk_tail;
    
    /* Lookup table: array of const char* indexed by name_id */
    const char** id_to_str;
    size_t count;
    size_t capacity;

    /* Hash table for deduplication */
    uint16_t* hash_slots; /* maps hash to name_id */
    size_t hash_mask;
};

static uint32_t fnv1a_hash(const char* s) {
    uint32_t h = 2166136261u;
    while (*s) {
        h ^= (uint8_t)(*s++);
        h *= 16777619u;
    }
    return h;
}

string_pool_t* string_pool_create(size_t initial_capacity) {
    if (initial_capacity < 32) initial_capacity = 32;
    
    string_pool_t* pool = (string_pool_t*)tracked_malloc(sizeof(string_pool_t));
    if (!pool) return NULL;
    memset(pool, 0, sizeof(string_pool_t));

    pool->chunk_head = (string_chunk_t*)tracked_malloc(sizeof(string_chunk_t) + INITIAL_STRING_CHUNK_SIZE);
    if (!pool->chunk_head) {
        tracked_free(pool);
        return NULL;
    }
    pool->chunk_head->capacity = INITIAL_STRING_CHUNK_SIZE;
    pool->chunk_head->used = 0;
    pool->chunk_head->next = NULL;
    pool->chunk_tail = pool->chunk_head;

    pool->capacity = initial_capacity;
    pool->count = 1; /* ID 0 is reserved for invalid */
    pool->id_to_str = (const char**)tracked_malloc(sizeof(const char*) * pool->capacity);
    pool->id_to_str[0] = "";

    size_t slot_count = INITIAL_SLOTS;
    pool->hash_mask = slot_count - 1;
    pool->hash_slots = (uint16_t*)tracked_calloc(slot_count, sizeof(uint16_t));

    return pool;
}

void string_pool_destroy(string_pool_t* pool) {
    if (!pool) return;

    string_chunk_t* curr = pool->chunk_head;
    while (curr) {
        string_chunk_t* next = curr->next;
        tracked_free(curr);
        curr = next;
    }

    if (pool->id_to_str) {
        tracked_free(pool->id_to_str);
    }
    if (pool->hash_slots) {
        tracked_free(pool->hash_slots);
    }
    tracked_free(pool);
}

static void string_pool_rehash(string_pool_t* pool) {
    size_t old_slots = pool->hash_mask + 1;
    size_t new_slots = old_slots * 2;
    uint16_t* new_table = (uint16_t*)tracked_calloc(new_slots, sizeof(uint16_t));
    if (!new_table) return;

    size_t new_mask = new_slots - 1;
    for (size_t id = 1; id < pool->count; id++) {
        const char* str = pool->id_to_str[id];
        uint32_t h = fnv1a_hash(str);
        size_t idx = h & new_mask;
        while (new_table[idx] != 0) {
            idx = (idx + 1) & new_mask;
        }
        new_table[idx] = (uint16_t)id;
    }

    tracked_free(pool->hash_slots);
    pool->hash_slots = new_table;
    pool->hash_mask = new_mask;
}

name_id_t string_pool_intern(string_pool_t* pool, const char* str) {
    if (!pool || !str) return NAME_ID_INVALID;

    uint32_t h = fnv1a_hash(str);
    size_t idx = h & pool->hash_mask;

    while (pool->hash_slots[idx] != 0) {
        name_id_t id = pool->hash_slots[idx];
        const char* candidate = pool->id_to_str[id];
        if (strcmp(candidate, str) == 0) {
            return id;
        }
        idx = (idx + 1) & pool->hash_mask;
    }

    /* Needs new entry */
    if (pool->count >= 65534) {
        fprintf(stderr, "ERROR: String pool exhausted 16-bit IDs (65534 max)\n");
        return NAME_ID_INVALID;
    }

    /* Copy string into chunk */
    size_t len = strlen(str) + 1;
    if (len > MAX_STRING_CHUNK_SIZE) {
        fprintf(stderr, "ERROR: String exceeds max chunk size\n");
        return NAME_ID_INVALID;
    }

    if (pool->chunk_tail->used + len > pool->chunk_tail->capacity) {
        size_t next_cap = pool->chunk_tail->capacity * 2;
        if (next_cap < len) next_cap = len + 256;
        if (next_cap > MAX_STRING_CHUNK_SIZE) next_cap = MAX_STRING_CHUNK_SIZE;
        string_chunk_t* new_chunk = (string_chunk_t*)tracked_malloc(sizeof(string_chunk_t) + next_cap);
        if (!new_chunk) return NAME_ID_INVALID;
        new_chunk->capacity = next_cap;
        new_chunk->used = 0;
        new_chunk->next = NULL;
        pool->chunk_tail->next = new_chunk;
        pool->chunk_tail = new_chunk;
    }

    char* dest = pool->chunk_tail->data + pool->chunk_tail->used;
    memcpy(dest, str, len);
    pool->chunk_tail->used += len;

    /* Expand id_to_str table if necessary */
    if (pool->count >= pool->capacity) {
        size_t new_cap = pool->capacity * 2;
        const char** new_id_to_str = (const char**)tracked_realloc(pool->id_to_str, sizeof(const char*) * new_cap);
        if (!new_id_to_str) return NAME_ID_INVALID;
        pool->id_to_str = new_id_to_str;
        pool->capacity = new_cap;
    }

    name_id_t new_id = (name_id_t)pool->count++;
    pool->id_to_str[new_id] = dest;
    pool->hash_slots[idx] = new_id;

    /* Rehash if load factor > 70% */
    if (pool->count * 10 >= (pool->hash_mask + 1) * 7) {
        string_pool_rehash(pool);
    }

    return new_id;
}

name_id_t string_pool_lookup(const string_pool_t* pool, const char* str) {
    if (!pool || !str) return NAME_ID_INVALID;
    uint32_t h = fnv1a_hash(str);
    size_t idx = h & pool->hash_mask;
    while (pool->hash_slots[idx] != 0) {
        name_id_t id = pool->hash_slots[idx];
        const char* candidate = pool->id_to_str[id];
        if (strcmp(candidate, str) == 0) return id;
        idx = (idx + 1) & pool->hash_mask;
    }
    return NAME_ID_INVALID;
}

const char* string_pool_resolve(const string_pool_t* pool, name_id_t id) {
    if (!pool || id == NAME_ID_INVALID || id >= pool->count) {
        return "";
    }
    return pool->id_to_str[id];
}

size_t string_pool_get_count(const string_pool_t* pool) {
    return pool ? pool->count - 1 : 0;
}

size_t string_pool_get_bytes_used(const string_pool_t* pool) {
    if (!pool) return 0;
    size_t total = sizeof(string_pool_t);
    string_chunk_t* c = pool->chunk_head;
    while (c) {
        total += sizeof(string_chunk_t) + c->capacity;
        c = c->next;
    }
    total += sizeof(const char*) * pool->capacity;
    total += sizeof(uint16_t) * (pool->hash_mask + 1);
    return total;
}
