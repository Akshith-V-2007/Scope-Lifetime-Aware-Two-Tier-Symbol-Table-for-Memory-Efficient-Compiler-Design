#ifndef MEM_TRACKER_H
#define MEM_TRACKER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    size_t current_bytes;
    size_t peak_bytes;
    size_t total_allocations;
    size_t total_frees;
    size_t active_allocations;
    size_t max_single_allocation;
} mem_stats_t;

void  mem_tracker_init(void);
void  mem_tracker_reset(void);
void* tracked_malloc(size_t size);
void* tracked_calloc(size_t num, size_t size);
void* tracked_realloc(void* ptr, size_t new_size);
void  tracked_free(void* ptr);
char* tracked_strdup(const char* s);

mem_stats_t mem_tracker_get_stats(void);
size_t mem_tracker_get_current(void);
size_t mem_tracker_get_peak(void);
void mem_tracker_set_budget_limit(size_t limit);
size_t mem_tracker_get_budget_limit(void);
void mem_tracker_dump_summary(const char* tag);

#ifdef __cplusplus
}
#endif

#endif /* MEM_TRACKER_H */
