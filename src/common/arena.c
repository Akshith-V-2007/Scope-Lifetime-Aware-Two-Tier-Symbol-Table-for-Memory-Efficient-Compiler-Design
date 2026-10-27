#include "arena.h"
#include "mem_tracker.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CHUNK_SIZE 2048

static arena_chunk_t* chunk_create(size_t capacity) {
    arena_chunk_t* chunk = (arena_chunk_t*)tracked_malloc(sizeof(arena_chunk_t));
    if (!chunk) return NULL;
    chunk->memory = (uint8_t*)tracked_malloc(capacity);
    if (!chunk->memory) {
        tracked_free(chunk);
        return NULL;
    }
    chunk->capacity = capacity;
    chunk->used = 0;
    chunk->next = NULL;
    return chunk;
}

arena_t* arena_create(size_t chunk_size) {
    if (chunk_size == 0) chunk_size = DEFAULT_CHUNK_SIZE;
    arena_t* arena = (arena_t*)tracked_malloc(sizeof(arena_t));
    if (!arena) return NULL;

    arena->default_chunk_size = chunk_size;
    arena->head = chunk_create(chunk_size);
    if (!arena->head) {
        tracked_free(arena);
        return NULL;
    }
    arena->current = arena->head;
    arena->total_allocated = chunk_size + sizeof(arena_chunk_t) + sizeof(arena_t);
    arena->total_used = 0;

    return arena;
}

void arena_destroy(arena_t* arena) {
    if (!arena) return;

    arena_chunk_t* curr = arena->head;
    while (curr) {
        arena_chunk_t* next = curr->next;
        if (curr->memory) {
            tracked_free(curr->memory);
        }
        tracked_free(curr);
        curr = next;
    }
    tracked_free(arena);
}

void* arena_alloc(arena_t* arena, size_t size) {
    if (!arena || size == 0) return NULL;

    /* Align to 8 bytes */
    size_t aligned_size = (size + 7) & ~((size_t)7);

    arena_chunk_t* chunk = arena->current;
    if (chunk->used + aligned_size > chunk->capacity) {
        /* Need a new chunk */
        size_t next_cap = arena->default_chunk_size;
        if (aligned_size > next_cap) {
            next_cap = aligned_size;
        }

        /* Check if we already have a chained next chunk from a previous reset */
        if (chunk->next && chunk->next->capacity >= aligned_size) {
            chunk = chunk->next;
            chunk->used = 0;
        } else {
            arena_chunk_t* new_chunk = chunk_create(next_cap);
            if (!new_chunk) return NULL;
            chunk->next = new_chunk;
            chunk = new_chunk;
            arena->total_allocated += next_cap + sizeof(arena_chunk_t);
        }
        arena->current = chunk;
    }

    void* ptr = chunk->memory + chunk->used;
    chunk->used += aligned_size;
    arena->total_used += aligned_size;
    return ptr;
}

void arena_reset(arena_t* arena) {
    if (!arena) return;
    arena_chunk_t* curr = arena->head;
    while (curr) {
        curr->used = 0;
        curr = curr->next;
    }
    arena->current = arena->head;
    arena->total_used = 0;
}

size_t arena_get_bytes_used(const arena_t* arena) {
    return arena ? arena->total_used : 0;
}

size_t arena_get_bytes_allocated(const arena_t* arena) {
    return arena ? arena->total_allocated : 0;
}
