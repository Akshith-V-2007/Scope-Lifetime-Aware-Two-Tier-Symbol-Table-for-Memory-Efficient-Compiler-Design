#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/symtab/symtab_baseline.h"
#include "../src/symtab/symtab_twotier.h"
#include "../src/common/mem_tracker.h"

#define NUM_OPS 2000

static const char* sample_names[] = {
    "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
    "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", "pi",
    "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega",
    "counter", "index", "buffer", "pointer", "temp", "status", "flags",
    "total", "result", "sensor_val", "cache_line", "timer_reg", "baud_rate"
};

#define NUM_NAMES (sizeof(sample_names) / sizeof(sample_names[0]))

void run_differential_test(int seed) {
    srand(seed);
    printf("[DIFFERENTIAL] Running %d random operations (seed: %d)...\n", NUM_OPS, seed);

    symtab_t* base = symtab_baseline_create();
    symtab_t* two = symtab_twotier_create(2, 0);

    /* Open global scope */
    base->enter_scope(base);
    two->enter_scope(two);

    int scope_depth = 0;
    int next_offset = 0;

    for (int op = 0; op < NUM_OPS; op++) {
        int action = rand() % 100;

        if (action < 15) {
            /* Enter scope (up to depth 25) */
            if (scope_depth < 25) {
                base->enter_scope(base);
                two->enter_scope(two);
                scope_depth++;
            }
        } else if (action < 25) {
            /* Exit scope (never exit global scope 0 during stream) */
            if (scope_depth > 0) {
                base->exit_scope(base);
                two->exit_scope(two);
                scope_depth--;
            }
        } else if (action < 60) {
            /* Insert random symbol */
            const char* name = sample_names[rand() % NUM_NAMES];
            type_id_t tid = (type_id_t)(rand() % 5 + 1);
            symbol_kind_t kind = (symbol_kind_t)(rand() % 4);
            int32_t offset = next_offset;
            next_offset += 4;

            bool b_ok = base->insert(base, name, tid, kind, offset);
            bool t_ok = two->insert(two, name, tid, kind, offset);

            /* Both must either succeed or fail (redeclaration) identically! */
            assert(b_ok == t_ok);
        } else {
            /* Lookup random symbol */
            const char* name = sample_names[rand() % NUM_NAMES];
            const symbol_record_t* b_rec = base->lookup(base, name);
            const symbol_record_t* t_rec = two->lookup(two, name);

            if (b_rec == NULL) {
                if (t_rec != NULL) {
                    printf("MISMATCH at op %d: baseline=NULL, twotier found '%s' at depth %u\n",
                           op, t_rec->name, t_rec->scope_depth);
                    assert(t_rec == NULL);
                }
            } else {
                if (t_rec == NULL) {
                    printf("MISMATCH at op %d: lookup '%s' found in baseline at depth %u, but twotier=NULL!\n",
                           op, name, b_rec->scope_depth);
                    fflush(stdout);
                    assert(t_rec != NULL);
                }
                if (b_rec->type_id != t_rec->type_id || b_rec->offset != t_rec->offset || b_rec->scope_depth != t_rec->scope_depth) {
                    printf("MISMATCH at op %d on '%s':\n", op, name);
                    printf("  Baseline: depth=%u, offset=%d, type=%u, kind=%u\n",
                           b_rec->scope_depth, b_rec->offset, b_rec->type_id, b_rec->kind);
                    printf("  TwoTier:  depth=%u, offset=%d, type=%u, kind=%u\n",
                           t_rec->scope_depth, t_rec->offset, t_rec->type_id, t_rec->kind);
                    fflush(stdout);
                }
                assert(strcmp(b_rec->name, t_rec->name) == 0);
                assert(b_rec->type_id == t_rec->type_id);
                assert(b_rec->kind == t_rec->kind);
                assert(b_rec->offset == t_rec->offset);
                assert(b_rec->scope_depth == t_rec->scope_depth);
            }
        }
    }

    /* Tear down remaining scopes */
    while (scope_depth >= 0) {
        base->exit_scope(base);
        two->exit_scope(two);
        scope_depth--;
    }

    symtab_stats_t b_stats, t_stats;
    base->get_stats(base, &b_stats);
    two->get_stats(two, &t_stats);

    printf("               Inserts: %llu | Lookups: %llu\n",
           (unsigned long long)t_stats.total_inserts,
           (unsigned long long)t_stats.total_lookups);
    printf("               Hot Hits: %llu | Cold Hits: %llu | Misses: %llu\n",
           (unsigned long long)t_stats.hot_hits,
           (unsigned long long)t_stats.cold_hits,
           (unsigned long long)t_stats.misses);
    printf("               Bloom Skips: %llu | Decompressions: %llu (Cache Hits: %llu)\n",
           (unsigned long long)t_stats.bloom_skips,
           (unsigned long long)t_stats.decompressions,
           (unsigned long long)t_stats.cache_hits);

    base->destroy(base);
    two->destroy(two);

    printf("[PASS] Differential test passed with 100%% equivalence!\n\n");
}

int main(void) {
    mem_tracker_init();
    run_differential_test(42);
    run_differential_test(1337);
    run_differential_test(2026);
    return 0;
}
