#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arena_chunk {
    uint8_t* memory;
    size_t   capacity;
    size_t   used;
    struct arena_chunk* next;
} arena_chunk_t;

typedef struct arena {
    arena_chunk_t* head;
    arena_chunk_t* current;
    size_t default_chunk_size;
    size_t total_allocated;
    size_t total_used;
} arena_t;

arena_t* arena_create(size_t chunk_size);
void     arena_destroy(arena_t* arena);
void*    arena_alloc(arena_t* arena, size_t size);
void     arena_reset(arena_t* arena);
size_t   arena_get_bytes_used(const arena_t* arena);
size_t   arena_get_bytes_allocated(const arena_t* arena);

#ifdef __cplusplus
}
#endif

#endif /* ARENA_H */
