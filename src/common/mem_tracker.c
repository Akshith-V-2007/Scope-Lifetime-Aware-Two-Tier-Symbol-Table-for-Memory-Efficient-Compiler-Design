#include "mem_tracker.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MEM_CANARY 0xDEADBEEFCAFEBABEU

typedef struct {
    uint64_t canary;
    size_t size;
    uint8_t padding[16 - (sizeof(size_t) % 16)]; /* Align to 16 bytes */
} mem_header_t;

static mem_stats_t g_mem_stats = {0};
static size_t g_budget_limit = 0; /* 0 = unlimited */

void mem_tracker_init(void) {
    memset(&g_mem_stats, 0, sizeof(g_mem_stats));
}

void mem_tracker_reset(void) {
    g_mem_stats.current_bytes = 0;
    g_mem_stats.peak_bytes = 0;
    g_mem_stats.total_allocations = 0;
    g_mem_stats.total_frees = 0;
    g_mem_stats.active_allocations = 0;
    g_mem_stats.max_single_allocation = 0;
}

void* tracked_malloc(size_t size) {
    if (size == 0) return NULL;

    size_t header_size = sizeof(mem_header_t);
    size_t total_size = header_size + size;

    if (g_budget_limit > 0 && g_mem_stats.current_bytes + size > g_budget_limit) {
        return NULL; /* Exceeded budget limit */
    }

    uint8_t* raw = (uint8_t*)malloc(total_size);
    if (!raw) return NULL;

    mem_header_t* header = (mem_header_t*)raw;
    header->canary = MEM_CANARY;
    header->size = size;

    g_mem_stats.current_bytes += size;
    if (g_mem_stats.current_bytes > g_mem_stats.peak_bytes) {
        g_mem_stats.peak_bytes = g_mem_stats.current_bytes;
    }
    g_mem_stats.total_allocations++;
    g_mem_stats.active_allocations++;
    if (size > g_mem_stats.max_single_allocation) {
        g_mem_stats.max_single_allocation = size;
    }

    return raw + header_size;
}

void* tracked_calloc(size_t num, size_t size) {
    size_t total = num * size;
    void* ptr = tracked_malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* tracked_realloc(void* ptr, size_t new_size) {
    if (!ptr) return tracked_malloc(new_size);
    if (new_size == 0) {
        tracked_free(ptr);
        return NULL;
    }

    mem_header_t* header = (mem_header_t*)((uint8_t*)ptr - sizeof(mem_header_t));
    if (header->canary != MEM_CANARY) {
        fprintf(stderr, "FATAL: Corrupted memory header in tracked_realloc\n");
        abort();
    }

    size_t old_size = header->size;
    void* new_ptr = tracked_malloc(new_size);
    if (!new_ptr) return NULL;

    size_t copy_bytes = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_bytes);
    tracked_free(ptr);

    return new_ptr;
}

void tracked_free(void* ptr) {
    if (!ptr) return;

    mem_header_t* header = (mem_header_t*)((uint8_t*)ptr - sizeof(mem_header_t));
    if (header->canary != MEM_CANARY) {
        fprintf(stderr, "FATAL: Invalid memory header or double free in tracked_free\n");
        abort();
    }

    size_t size = header->size;
    header->canary = 0; /* Invalidate */

    if (g_mem_stats.current_bytes >= size) {
        g_mem_stats.current_bytes -= size;
    } else {
        g_mem_stats.current_bytes = 0;
    }

    g_mem_stats.total_frees++;
    if (g_mem_stats.active_allocations > 0) {
        g_mem_stats.active_allocations--;
    }

    free(header);
}

char* tracked_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* copy = (char*)tracked_malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

mem_stats_t mem_tracker_get_stats(void) {
    return g_mem_stats;
}

size_t mem_tracker_get_current(void) {
    return g_mem_stats.current_bytes;
}

size_t mem_tracker_get_peak(void) {
    return g_mem_stats.peak_bytes;
}

void mem_tracker_set_budget_limit(size_t limit) {
    g_budget_limit = limit;
}

size_t mem_tracker_get_budget_limit(void) {
    return g_budget_limit;
}

void mem_tracker_dump_summary(const char* tag) {
    printf("[%s] Peak RAM: %zu bytes | Current: %zu bytes | Allocations: %zu | Frees: %zu | Active Blocks: %zu\n",
           tag ? tag : "MEM",
           g_mem_stats.peak_bytes,
           g_mem_stats.current_bytes,
           g_mem_stats.total_allocations,
           g_mem_stats.total_frees,
           g_mem_stats.active_allocations);
}
