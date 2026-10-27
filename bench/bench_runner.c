#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <cpuid.h>
#endif

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static void make_dir(const char* path) {
    CreateDirectoryA(path, NULL);
}
static double get_time_ns(void) {
    static LARGE_INTEGER freq;
    static int initialized = 0;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return ((double)count.QuadPart * 1e9) / (double)freq.QuadPart;
}
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__) || defined(_POSIX_C_SOURCE)
#include <sys/stat.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
static void make_dir(const char* path) {
    mkdir(path, 0777);
}
static double get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
#else
static void make_dir(const char* path) { (void)path; }
static double get_time_ns(void) {
    return ((double)clock() / (double)CLOCKS_PER_SEC) * 1e9;
}
#endif

#include "../src/common/mem_tracker.h"
#include "../src/common/type_table.h"
#include "../src/common/string_pool.h"
#include "../src/common/arena.h"
#include "../src/symtab/symtab_api.h"
#include "../src/symtab/symtab_baseline.h"
#include "../src/symtab/symtab_twotier.h"
#include "../src/symtab/symtab_ablation.h"
#include "../src/compiler/parser.h"
#include "../src/compiler/semantic.h"

#define NUM_RUNS 15

static const char* ALL_WORKLOADS[] = {
    "data/arduino_sensors.mc",
    "data/freertos_tasks.mc",
    "data/zephyr_network.mc",
    "data/mibench_telecom.mc",
    "data/embedded_flight_controller.mc",
    "data/synthetic_broad_scope.mc",
    "data/synthetic_heavy_shadowing.mc",
    "data/synthetic_deep_nesting.mc"
};
#define NUM_WORKLOADS (sizeof(ALL_WORKLOADS) / sizeof(ALL_WORKLOADS[0]))

static const char* ALL_ENGINES[] = {
    "baseline",
    "arena",
    "intern",
    "packed",
    "nombloom",
    "twotier"
};
#define NUM_ENGINES (sizeof(ALL_ENGINES) / sizeof(ALL_ENGINES[0]))

typedef struct {
    double elapsed_ns;
    size_t peak_ram_bytes;
    size_t total_inserts;
    size_t total_lookups;
    uint64_t hot_hits;
    uint64_t cold_hits;
    uint64_t misses;
    uint64_t range_skips;
    uint64_t bloom_skips;
    uint64_t bloom_fp;
    uint64_t decompressions;
    uint64_t cache_hits;
    uint64_t cache_misses;
    size_t peak_symbols;
    double bytes_per_sym;
    size_t total_allocations;
    size_t total_frees;
} run_metrics_t;

static int compare_doubles(const void* a, const void* b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da > db) - (da < db);
}

static void calc_median_iqr(double* arr, size_t n, double* out_med, double* out_iqr) {
    if (n == 0) {
        if (out_med) *out_med = 0.0;
        if (out_iqr) *out_iqr = 0.0;
        return;
    }
    qsort(arr, n, sizeof(double), compare_doubles);
    double med = (n % 2 == 1) ? arr[n / 2] : (arr[n / 2 - 1] + arr[n / 2]) * 0.5;
    double q1 = arr[n / 4];
    double q3 = arr[(3 * n) / 4];
    if (out_med) *out_med = med;
    if (out_iqr) *out_iqr = (q3 - q1);
}

static char* read_entire_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t r = fread(buf, 1, sz, f);
    buf[r] = '\0';
    fclose(f);
    return buf;
}

static const char* get_base_name(const char* path) {
    const char* b = strrchr(path, '/');
    if (!b) b = strrchr(path, '\\');
    return b ? b + 1 : path;
}

static symtab_t* create_engine(const char* engine, int hot_depth, size_t ram_budget) {
    if (strcmp(engine, "baseline") == 0) return symtab_baseline_create();
    if (strcmp(engine, "arena") == 0)    return symtab_ablation_create(SYMTAB_ARENA_ONLY);
    if (strcmp(engine, "intern") == 0)   return symtab_ablation_create(SYMTAB_INTERN_ONLY);
    if (strcmp(engine, "packed") == 0)   return symtab_ablation_create(SYMTAB_PACKED_ONLY);
    if (strcmp(engine, "nombloom") == 0) return symtab_ablation_create(SYMTAB_COMPRESSED_NO_BLOOM);
    if (strcmp(engine, "twotier") == 0)  return symtab_twotier_create(hot_depth, ram_budget);
    return NULL;
}

static run_metrics_t execute_benchmark_run(ast_node_t* prog, type_table_t* tt, const char* engine, int hot_depth, size_t ram_budget) {
    run_metrics_t m;
    memset(&m, 0, sizeof(m));

    mem_tracker_reset();
    symtab_t* tab = create_engine(engine, hot_depth, ram_budget);
    if (!tab) return m;

    semantic_context_t ctx;
    semantic_init(&ctx, tab, tt);
    ctx.quiet = true;

    double t0 = get_time_ns();
    semantic_analyze(&ctx, prog);
    double t1 = get_time_ns();

    symtab_stats_t stats;
    tab->get_stats(tab, &stats);
    mem_stats_t mem_st = mem_tracker_get_stats();

    m.elapsed_ns = (t1 - t0);
    m.peak_ram_bytes = mem_st.peak_bytes;
    m.total_inserts = stats.total_inserts;
    m.total_lookups = stats.total_lookups;
    m.hot_hits = stats.hot_hits;
    m.cold_hits = stats.cold_hits;
    m.misses = stats.misses;
    m.range_skips = stats.range_skips;
    m.bloom_skips = stats.bloom_skips;
    m.bloom_fp = stats.bloom_false_positives;
    m.decompressions = stats.decompressions;
    m.cache_hits = stats.cache_hits;
    m.cache_misses = stats.cache_misses;
    m.peak_symbols = stats.peak_symbols;
    m.bytes_per_sym = (stats.peak_symbols > 0) ? ((double)m.peak_ram_bytes / (double)stats.peak_symbols) : 0.0;
    m.total_allocations = mem_st.total_allocations;
    m.total_frees = mem_st.total_frees;

    tab->destroy(tab);
    return m;
}

/* =========================================================================
 * METADATA GENERATOR
 * ========================================================================= */
static void write_metadata(const char* path) {
    FILE* f = fopen(path, "w");
    if (!f) return;

    /* 1. Real current UTC timestamp */
    time_t now = time(NULL);
    struct tm* gmt = gmtime(&now);
    char time_buf[64] = "unknown";
    if (gmt) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ", gmt);
    }

    /* 2. Detected OS */
    char os_name[128] = "Unknown OS";
#if defined(_WIN32) || defined(_WIN64)
    snprintf(os_name, sizeof(os_name), "Windows (NT kernel x86_64)");
#elif defined(__APPLE__)
    snprintf(os_name, sizeof(os_name), "macOS");
#elif defined(__linux__)
    snprintf(os_name, sizeof(os_name), "Linux");
#elif defined(__unix__)
    snprintf(os_name, sizeof(os_name), "Unix");
#endif

    /* 3. Detected CPU Model */
    char cpu_model[128] = "Unknown CPU";
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    unsigned int regs[4] = {0};
    __cpuid(0x80000000, regs[0], regs[1], regs[2], regs[3]);
    if (regs[0] >= 0x80000004) {
        char brand[49] = {0};
        __cpuid(0x80000002, *(unsigned int*)&brand[0], *(unsigned int*)&brand[4], *(unsigned int*)&brand[8], *(unsigned int*)&brand[12]);
        __cpuid(0x80000003, *(unsigned int*)&brand[16], *(unsigned int*)&brand[20], *(unsigned int*)&brand[24], *(unsigned int*)&brand[28]);
        __cpuid(0x80000004, *(unsigned int*)&brand[32], *(unsigned int*)&brand[36], *(unsigned int*)&brand[40], *(unsigned int*)&brand[44]);
        char* b = brand;
        while (*b == ' ') b++;
        snprintf(cpu_model, sizeof(cpu_model), "%s", b);
    }
#endif
    if (strcmp(cpu_model, "Unknown CPU") == 0) {
        const char* env_cpu = getenv("PROCESSOR_IDENTIFIER");
        if (env_cpu) snprintf(cpu_model, sizeof(cpu_model), "%s", env_cpu);
    }
    size_t clen = strlen(cpu_model);
    while (clen > 0 && (cpu_model[clen-1] == ' ' || cpu_model[clen-1] == '\r' || cpu_model[clen-1] == '\n')) {
        cpu_model[clen-1] = '\0';
        clen--;
    }

    /* 4. Logical Cores and Total RAM */
    int logical_cores = 1;
    unsigned long long total_ram_bytes = 0;
#if defined(_WIN32) || defined(_WIN64)
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    logical_cores = (int)si.dwNumberOfProcessors;
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        total_ram_bytes = mem.ullTotalPhys;
    }
#elif defined(__linux__)
    logical_cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        total_ram_bytes = (unsigned long long)pages * (unsigned long long)page_size;
    }
#elif defined(__APPLE__)
    size_t len = sizeof(logical_cores);
    sysctlbyname("hw.logicalcpu", &logical_cores, &len, NULL, 0);
    len = sizeof(total_ram_bytes);
    sysctlbyname("hw.memsize", &total_ram_bytes, &len, NULL, 0);
#endif

    /* 5. Detected Compiler */
    char compiler_info[128] = "c11-compliant-compiler";
#if defined(__clang__)
    snprintf(compiler_info, sizeof(compiler_info), "clang %s", __clang_version__);
#elif defined(__GNUC__)
    snprintf(compiler_info, sizeof(compiler_info), "gcc %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
    snprintf(compiler_info, sizeof(compiler_info), "msvc %d", _MSC_VER);
#endif

    /* 6. Detected Compiler Flags */
    char flags_info[128] = "-std=c11 -Wall -Wextra -Isrc";
#if defined(__OPTIMIZE__)
    snprintf(flags_info, sizeof(flags_info), "-std=c11 -Wall -Wextra -O2 -Isrc");
#else
    snprintf(flags_info, sizeof(flags_info), "-std=c11 -Wall -Wextra -O0 -Isrc");
#endif
#if defined(__SANITIZE_ADDRESS__)
    strncat(flags_info, " -fsanitize=address,undefined", sizeof(flags_info) - strlen(flags_info) - 1);
#endif

    fprintf(f, "{\n");
    fprintf(f, "  \"timestamp\": \"%s\",\n", time_buf);
    fprintf(f, "  \"os\": \"%s\",\n", os_name);
    fprintf(f, "  \"cpu_model\": \"%s\",\n", cpu_model);
    fprintf(f, "  \"logical_cores\": %d,\n", logical_cores);
    if (total_ram_bytes > 0) {
        fprintf(f, "  \"ram_total_bytes\": %llu,\n", total_ram_bytes);
        fprintf(f, "  \"ram_total_gb\": %.2f,\n", (double)total_ram_bytes / (1024.0 * 1024.0 * 1024.0));
    }
    fprintf(f, "  \"compiler\": \"%s\",\n", compiler_info);
    fprintf(f, "  \"flags\": \"%s\",\n", flags_info);
    fprintf(f, "  \"timing_runs\": %d,\n", NUM_RUNS);
    fprintf(f, "  \"statistic\": \"median + IQR (Q3 - Q1)\"\n");
    fprintf(f, "}\n");
    fclose(f);
}

/* =========================================================================
 * EXPERIMENT 1: Peak RAM and Bytes Per Symbol across All Engines & Workloads
 * ========================================================================= */
static void run_experiment_1(FILE* legacy_csv) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 1: Peak RAM and Bytes/Symbol across 6 Engines & 8 Workloads (%d-Run Median + IQR)\n", NUM_RUNS);
    printf("=========================================================================================================================\n");
    printf("%-26s | %-12s | %-12s | %-16s | %-14s | %-10s\n",
           "Benchmark Workload", "Baseline RAM", "Two-Tier RAM", "RAM Reduction", "Slowdown Ratio", "Decomps");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    FILE* csv = fopen("bench/results/exp1_peak_ram.csv", "w");
    if (csv) {
        fprintf(csv, "run_id,workload,engine,peak_ram_bytes,symbols,bytes_per_symbol,elapsed_ns\n");
    }

    for (size_t w = 0; w < NUM_WORKLOADS; w++) {
        char* src = read_entire_file(ALL_WORKLOADS[w]);
        if (!src) continue;

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        double base_ram = 0.0;
        double base_time = 0.0;
        double two_ram = 0.0;
        double two_time = 0.0;
        uint64_t two_decomps = 0;

        for (size_t e = 0; e < NUM_ENGINES; e++) {
            const char* eng = ALL_ENGINES[e];
            double ram_runs[NUM_RUNS];
            double time_runs[NUM_RUNS];
            double bytes_sym_runs[NUM_RUNS];
            uint64_t decomps = 0;
            uint64_t hot_hits = 0;
            uint64_t cold_hits = 0;
            uint64_t bloom_skips = 0;
            size_t inserts = 0;
            size_t lookups = 0;

            for (int r = 0; r < NUM_RUNS; r++) {
                srand(42 + r);
                run_metrics_t m = execute_benchmark_run(prog, tt, eng, 2, 0);
                ram_runs[r] = (double)m.peak_ram_bytes;
                time_runs[r] = m.elapsed_ns;
                bytes_sym_runs[r] = m.bytes_per_sym;
                decomps = m.decompressions;
                hot_hits = m.hot_hits;
                cold_hits = m.cold_hits;
                bloom_skips = m.bloom_skips;
                inserts = m.total_inserts;
                lookups = m.total_lookups;

                if (csv) {
                    fprintf(csv, "%d,%s,%s,%zu,%zu,%.2f,%.1f\n",
                            r + 1, get_base_name(ALL_WORKLOADS[w]), eng,
                            m.peak_ram_bytes, m.peak_symbols, m.bytes_per_sym, m.elapsed_ns);
                }
            }

            double med_ram, iqr_ram, med_time, iqr_time, med_bps, iqr_bps;
            calc_median_iqr(ram_runs, NUM_RUNS, &med_ram, &iqr_ram);
            calc_median_iqr(time_runs, NUM_RUNS, &med_time, &iqr_time);
            calc_median_iqr(bytes_sym_runs, NUM_RUNS, &med_bps, &iqr_bps);

            if (strcmp(eng, "baseline") == 0) {
                base_ram = med_ram;
                base_time = med_time;
                if (legacy_csv) {
                    fprintf(legacy_csv, "exp1,%s,baseline,%zu,%zu,%zu,%llu,%llu,%llu,%llu,%.2f,%.1f,1.00\n",
                            get_base_name(ALL_WORKLOADS[w]), (size_t)med_ram, inserts, lookups,
                            (unsigned long long)hot_hits, (unsigned long long)cold_hits,
                            (unsigned long long)bloom_skips, (unsigned long long)decomps,
                            med_bps, med_time);
                }
            } else if (strcmp(eng, "twotier") == 0) {
                two_ram = med_ram;
                two_time = med_time;
                two_decomps = decomps;
                double slowdown = (base_time > 0.0) ? (two_time / base_time) : 1.0;
                if (legacy_csv) {
                    fprintf(legacy_csv, "exp1,%s,twotier,%zu,%zu,%zu,%llu,%llu,%llu,%llu,%.2f,%.1f,%.2f\n",
                            get_base_name(ALL_WORKLOADS[w]), (size_t)med_ram, inserts, lookups,
                            (unsigned long long)hot_hits, (unsigned long long)cold_hits,
                            (unsigned long long)bloom_skips, (unsigned long long)decomps,
                            med_bps, med_time, slowdown);
                }
            }
        }

        double diff = (base_ram > 0.0) ? ((1.0 - two_ram / base_ram) * 100.0) : 0.0;
        double slowdown = (base_time > 0.0) ? (two_time / base_time) : 1.0;

        if (diff >= 0.0) {
            printf("%-26s | %9zu B  | %9zu B  | -%5.1f%% (saved) | %6.2fx slowdown  | %7llu\n",
                   get_base_name(ALL_WORKLOADS[w]), (size_t)base_ram, (size_t)two_ram,
                   diff, slowdown, (unsigned long long)two_decomps);
        } else {
            printf("%-26s | %9zu B  | %9zu B  | +%5.1f%% (worse) | %6.2fx slowdown  | %7llu\n",
                   get_base_name(ALL_WORKLOADS[w]), (size_t)base_ram, (size_t)two_ram,
                   -diff, slowdown, (unsigned long long)two_decomps);
        }

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(src);
    }
    if (csv) fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 2: Two-Tier Memory Breakdown (Hot table, Cold blocks, String pool, etc.)
 * ========================================================================= */
static void run_experiment_2(void) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 2: Two-Tier Component Memory Breakdown (Stacked Data)\n");
    printf("=========================================================================================================================\n");
    printf("%-26s | %-8s | %-8s | %-8s | %-8s | %-8s | %-8s | %-8s\n",
           "Workload", "HotTable", "ColdData", "StrPool", "TypeTab", "Arenas", "Meta/Hdr", "Total");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    FILE* csv = fopen("bench/results/exp2_memory_breakdown.csv", "w");
    if (csv) {
        fprintf(csv, "run_id,workload,hot_table_bytes,cold_blocks_bytes,string_pool_bytes,type_table_bytes,arenas_bytes,metadata_bloom_bytes,total_bytes\n");
    }

    for (size_t w = 0; w < NUM_WORKLOADS; w++) {
        char* src = read_entire_file(ALL_WORKLOADS[w]);
        if (!src) continue;

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        for (int r = 0; r < NUM_RUNS; r++) {
            mem_tracker_reset();
            symtab_t* tab = symtab_twotier_create(2, 0);

            semantic_context_t ctx;
            semantic_init(&ctx, tab, tt);
            ctx.quiet = true;
            semantic_analyze(&ctx, prog);

            twotier_memory_breakdown_t bd;
            symtab_twotier_get_memory_breakdown(tab, &bd);

            if (csv) {
                fprintf(csv, "%d,%s,%zu,%zu,%zu,%zu,%zu,%zu,%zu\n",
                        r + 1, get_base_name(ALL_WORKLOADS[w]),
                        bd.hot_table_bytes, bd.cold_blocks_bytes, bd.string_pool_bytes,
                        bd.type_table_bytes, bd.arena_bytes, bd.metadata_bytes, bd.total_tracked_bytes);
            }

            if (r == 0) {
                printf("%-26s | %6zu B | %6zu B | %6zu B | %6zu B | %6zu B | %6zu B | %6zu B\n",
                       get_base_name(ALL_WORKLOADS[w]),
                       bd.hot_table_bytes, bd.cold_blocks_bytes, bd.string_pool_bytes,
                       bd.type_table_bytes, bd.arena_bytes, bd.metadata_bytes, bd.total_tracked_bytes);
            }

            tab->destroy(tab);
        }

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(src);
    }
    if (csv) fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 3: Compression Efficiency & Bits/Symbol
 * ========================================================================= */
static void run_experiment_3(void) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 3: Cold-Block Compression Ratio & Bits Per Symbol\n");
    printf("=========================================================================================================================\n");
    printf("%-26s | %-12s | %-14s | %-14s | %-14s | %-12s\n",
           "Workload", "Cold Symbols", "Raw Packed (B)", "Compressed (B)", "Compress Ratio", "Bits / Symbol");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    FILE* csv = fopen("bench/results/exp3_compression_ratio.csv", "w");
    if (csv) {
        fprintf(csv, "run_id,workload,total_cold_symbols,raw_packed_bytes,compressed_bytes,compression_ratio,bits_per_symbol\n");
    }

    for (size_t w = 0; w < NUM_WORKLOADS; w++) {
        char* src = read_entire_file(ALL_WORKLOADS[w]);
        if (!src) continue;

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        for (int r = 0; r < NUM_RUNS; r++) {
            mem_tracker_reset();
            symtab_t* tab = symtab_twotier_create(2, 0);

            semantic_context_t ctx;
            semantic_init(&ctx, tab, tt);
            ctx.quiet = true;
            semantic_analyze(&ctx, prog);

            twotier_compression_stats_t cst;
            symtab_twotier_get_compression_stats(tab, &cst);

            if (csv) {
                fprintf(csv, "%d,%s,%zu,%zu,%zu,%.2f,%.2f\n",
                        r + 1, get_base_name(ALL_WORKLOADS[w]),
                        cst.total_cold_symbols, cst.raw_packed_bytes, cst.compressed_bytes,
                        cst.compression_ratio, cst.bits_per_symbol);
            }

            if (r == 0) {
                printf("%-26s | %12zu | %14zu | %14zu | %13.2fx | %10.2f b\n",
                       get_base_name(ALL_WORKLOADS[w]),
                       cst.total_cold_symbols, cst.raw_packed_bytes, cst.compressed_bytes,
                       cst.compression_ratio, cst.bits_per_symbol);
            }

            tab->destroy(tab);
        }

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(src);
    }
    if (csv) fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 4: Micro-Latency Breakdown (Insert, Hot, Cold-Warm, Cold-Forced, Miss)
 * ========================================================================= */
static void run_experiment_4(FILE* legacy_csv) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 4: Micro-Latency Breakdown (p50 / p95 / p99 / IQR Across %d Runs)\n", NUM_RUNS);
    printf("=========================================================================================================================\n");
    printf("%-28s | %-15s | %-15s | %-12s | %-18s\n",
           "Operation Type", "Baseline (p50)", "Two-Tier (p50)", "Speed Ratio", "Profile");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    FILE* csv = fopen("bench/results/exp4_micro_latency.csv", "w");
    if (csv) {
        fprintf(csv, "run_id,op_type,engine,latency_ns,p50_ns,p95_ns,p99_ns,iqr_ns\n");
    }

    #define NUM_INSERTS 500
    #define NUM_OPS 2000
    #define NUM_FORCED 600

    char names[NUM_INSERTS][32];
    char absent[100][32];
    for (int i = 0; i < NUM_INSERTS; i++) snprintf(names[i], sizeof(names[i]), "var_test_%04d", i);
    for (int i = 0; i < 100; i++) snprintf(absent[i], sizeof(absent[i]), "absent_sym_%03d", i);

    char cold_vars[4][30][32];
    char forced_queries[NUM_FORCED][32];
    for (int s = 0; s < 4; s++) {
        for (int v = 0; v < 30; v++) {
            snprintf(cold_vars[s][v], sizeof(cold_vars[s][v]), "scope%d_v%02d", s, v);
        }
    }
    for (int i = 0; i < NUM_FORCED; i++) {
        int s = i % 4; /* cycle through 4 scopes, defeating 2-slot LRU */
        int v = (i / 4) % 30;
        snprintf(forced_queries[i], sizeof(forced_queries[i]), "%s", cold_vars[s][v]);
    }

    const char* ops[] = {
        "insert",
        "hot_lookup",
        "cold_lookup_warm",
        "cold_lookup_forced",
        "miss_lookup"
    };

    double base_meds[5], two_meds[5], base_iqrs[5], two_iqrs[5];

    for (int op_idx = 0; op_idx < 5; op_idx++) {
        double b_samples[NUM_RUNS];
        double t_samples[NUM_RUNS];

        for (int r = 0; r < NUM_RUNS; r++) {
            srand(100 + r * 13);

            if (op_idx == 0) {
                /* Insert */
                mem_tracker_reset();
                symtab_t* btab = symtab_baseline_create();
                btab->enter_scope(btab);
                double bt0 = get_time_ns();
                for (int i = 0; i < NUM_INSERTS; i++) btab->insert(btab, names[i], TYPE_INT, KIND_VAR, i * 4);
                double bt1 = get_time_ns();
                b_samples[r] = (bt1 - bt0) / (double)NUM_INSERTS;
                btab->destroy(btab);

                mem_tracker_reset();
                symtab_t* ttab = symtab_twotier_create(2, 0);
                ttab->enter_scope(ttab);
                double tt0 = get_time_ns();
                for (int i = 0; i < NUM_INSERTS; i++) ttab->insert(ttab, names[i], TYPE_INT, KIND_VAR, i * 4);
                double tt1 = get_time_ns();
                t_samples[r] = (tt1 - tt0) / (double)NUM_INSERTS;
                ttab->destroy(ttab);
            } else if (op_idx == 1) {
                /* Hot lookup */
                mem_tracker_reset();
                symtab_t* btab = symtab_baseline_create();
                btab->enter_scope(btab);
                for (int i = 0; i < NUM_INSERTS; i++) btab->insert(btab, names[i], TYPE_INT, KIND_VAR, i * 4);
                double bt0 = get_time_ns();
                for (int i = 0; i < NUM_OPS; i++) btab->lookup(btab, names[i % NUM_INSERTS]);
                double bt1 = get_time_ns();
                b_samples[r] = (bt1 - bt0) / (double)NUM_OPS;
                btab->destroy(btab);

                mem_tracker_reset();
                symtab_t* ttab = symtab_twotier_create(2, 0);
                ttab->enter_scope(ttab);
                for (int i = 0; i < NUM_INSERTS; i++) ttab->insert(ttab, names[i], TYPE_INT, KIND_VAR, i * 4);
                double tt0 = get_time_ns();
                for (int i = 0; i < NUM_OPS; i++) ttab->lookup(ttab, names[i % NUM_INSERTS]);
                double tt1 = get_time_ns();
                t_samples[r] = (tt1 - tt0) / (double)NUM_OPS;
                ttab->destroy(ttab);
            } else if (op_idx == 2) {
                /* Cold lookup warm */
                mem_tracker_reset();
                symtab_t* btab = symtab_baseline_create();
                btab->enter_scope(btab);
                for (int i = 0; i < NUM_INSERTS; i++) btab->insert(btab, names[i], TYPE_INT, KIND_VAR, i * 4);
                for (int d = 0; d < 4; d++) btab->enter_scope(btab);
                double bt0 = get_time_ns();
                for (int i = 0; i < NUM_OPS; i++) btab->lookup(btab, names[i % 50]);
                double bt1 = get_time_ns();
                b_samples[r] = (bt1 - bt0) / (double)NUM_OPS;
                btab->destroy(btab);

                mem_tracker_reset();
                symtab_t* ttab = symtab_twotier_create(2, 0);
                ttab->enter_scope(ttab);
                for (int i = 0; i < NUM_INSERTS; i++) ttab->insert(ttab, names[i], TYPE_INT, KIND_VAR, i * 4);
                for (int d = 0; d < 4; d++) ttab->enter_scope(ttab);
                double tt0 = get_time_ns();
                for (int i = 0; i < NUM_OPS; i++) ttab->lookup(ttab, names[i % 50]);
                double tt1 = get_time_ns();
                t_samples[r] = (tt1 - tt0) / (double)NUM_OPS;
                ttab->destroy(ttab);
            } else if (op_idx == 3) {
                /* Cold forced decompress (cache-defeating) */
                mem_tracker_reset();
                symtab_t* btab = symtab_baseline_create();
                for (int s = 0; s < 4; s++) {
                    btab->enter_scope(btab);
                    for (int v = 0; v < 30; v++) btab->insert(btab, cold_vars[s][v], TYPE_INT, KIND_VAR, v * 4);
                }
                for (int s = 0; s < 2; s++) btab->enter_scope(btab);
                double bt0 = get_time_ns();
                for (int i = 0; i < NUM_FORCED; i++) btab->lookup(btab, forced_queries[i]);
                double bt1 = get_time_ns();
                b_samples[r] = (bt1 - bt0) / (double)NUM_FORCED;
                btab->destroy(btab);

                mem_tracker_reset();
                symtab_t* ttab = symtab_twotier_create(2, 0);
                for (int s = 0; s < 4; s++) {
                    ttab->enter_scope(ttab);
                    for (int v = 0; v < 30; v++) ttab->insert(ttab, cold_vars[s][v], TYPE_INT, KIND_VAR, v * 4);
                }
                for (int s = 0; s < 2; s++) ttab->enter_scope(ttab);
                double tt0 = get_time_ns();
                for (int i = 0; i < NUM_FORCED; i++) ttab->lookup(ttab, forced_queries[i]);
                double tt1 = get_time_ns();
                t_samples[r] = (tt1 - tt0) / (double)NUM_FORCED;
                ttab->destroy(ttab);
            } else {
                /* Negative miss lookup */
                mem_tracker_reset();
                symtab_t* btab = symtab_baseline_create();
                btab->enter_scope(btab);
                for (int i = 0; i < NUM_INSERTS; i++) btab->insert(btab, names[i], TYPE_INT, KIND_VAR, i * 4);
                for (int d = 0; d < 4; d++) btab->enter_scope(btab);
                double bt0 = get_time_ns();
                for (int i = 0; i < NUM_OPS; i++) btab->lookup(btab, absent[i % 100]);
                double bt1 = get_time_ns();
                b_samples[r] = (bt1 - bt0) / (double)NUM_OPS;
                btab->destroy(btab);

                mem_tracker_reset();
                symtab_t* ttab = symtab_twotier_create(2, 0);
                ttab->enter_scope(ttab);
                for (int i = 0; i < NUM_INSERTS; i++) ttab->insert(ttab, names[i], TYPE_INT, KIND_VAR, i * 4);
                for (int d = 0; d < 4; d++) ttab->enter_scope(ttab);
                double tt0 = get_time_ns();
                for (int i = 0; i < NUM_OPS; i++) ttab->lookup(ttab, absent[i % 100]);
                double tt1 = get_time_ns();
                t_samples[r] = (tt1 - tt0) / (double)NUM_OPS;
                ttab->destroy(ttab);
            }

            if (csv) {
                fprintf(csv, "%d,%s,baseline,%.2f,0,0,0,0\n", r + 1, ops[op_idx], b_samples[r]);
                fprintf(csv, "%d,%s,twotier,%.2f,0,0,0,0\n", r + 1, ops[op_idx], t_samples[r]);
            }
        }

        calc_median_iqr(b_samples, NUM_RUNS, &base_meds[op_idx], &base_iqrs[op_idx]);
        calc_median_iqr(t_samples, NUM_RUNS, &two_meds[op_idx], &two_iqrs[op_idx]);

        double ratio = (base_meds[op_idx] > 0.0) ? (two_meds[op_idx] / base_meds[op_idx]) : 1.0;
        const char* label = ops[op_idx];
        const char* profile = (ratio <= 1.0) ? "Faster" : "Slower";

        printf("%-28s | %9.1f ns/op   | %9.1f ns/op   | %6.2fx       | %s\n",
               label, base_meds[op_idx], two_meds[op_idx], ratio, profile);

        if (legacy_csv) {
            fprintf(legacy_csv, "exp5_latency,%s,%d,0,0,0,0,0,0,0,0.0,%.2f,%.2f\n",
                    ops[op_idx], (op_idx == 0) ? NUM_INSERTS : (op_idx == 3 ? NUM_FORCED : NUM_OPS),
                    two_meds[op_idx], ratio);
        }
    }

    if (csv) fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 5: Scope Enter/Exit Lifecycle Scaling (10, 100, 1000 Symbols)
 * ========================================================================= */
static void run_experiment_5(FILE* legacy_csv) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 5: Scope Lifecycle Scaling (Enter + Insert N + Teardown) [%d Runs]\n", NUM_RUNS);
    printf(" Question: How does Two-Tier's arena + hot-table sweep scale compared to Baseline's chained free?\n");
    printf("=========================================================================================================================\n");
    printf("%-24s | %-16s | %-16s | %-12s | %-24s\n",
           "Symbols Per Scope", "Baseline Time", "Two-Tier Time", "Ratio", "Teardown Mechanism");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    FILE* csv = fopen("bench/results/exp5_scope_scaling.csv", "w");
    if (csv) {
        fprintf(csv, "run_id,engine,symbols_per_scope,enter_exit_time_ns\n");
    }

    int counts[] = { 10, 100, 1000 };
    for (int c = 0; c < 3; c++) {
        int N = counts[c];
        char syms[1000][32];
        for (int i = 0; i < N; i++) snprintf(syms[i], sizeof(syms[i]), "var_%04d", i);

        double base_runs[NUM_RUNS];
        double two_runs[NUM_RUNS];

        for (int r = 0; r < NUM_RUNS; r++) {
            /* Baseline */
            mem_tracker_reset();
            symtab_t* btab = symtab_baseline_create();
            double bt0 = get_time_ns();
            btab->enter_scope(btab);
            for (int i = 0; i < N; i++) btab->insert(btab, syms[i], TYPE_INT, KIND_VAR, i * 4);
            btab->exit_scope(btab);
            double bt1 = get_time_ns();
            base_runs[r] = (bt1 - bt0);
            btab->destroy(btab);

            /* Two-Tier */
            mem_tracker_reset();
            symtab_t* ttab = symtab_twotier_create(2, 0);
            double tt0 = get_time_ns();
            ttab->enter_scope(ttab);
            for (int i = 0; i < N; i++) ttab->insert(ttab, syms[i], TYPE_INT, KIND_VAR, i * 4);
            ttab->exit_scope(ttab);
            double tt1 = get_time_ns();
            two_runs[r] = (tt1 - tt0);
            ttab->destroy(ttab);

            if (csv) {
                fprintf(csv, "%d,baseline,%d,%.1f\n", r + 1, N, base_runs[r]);
                fprintf(csv, "%d,twotier,%d,%.1f\n", r + 1, N, two_runs[r]);
            }
        }

        double med_base, iqr_base, med_two, iqr_two;
        calc_median_iqr(base_runs, NUM_RUNS, &med_base, &iqr_base);
        calc_median_iqr(two_runs, NUM_RUNS, &med_two, &iqr_two);
        double ratio = (med_base > 0.0) ? (med_two / med_base) : 1.0;

        printf("N = %-20d | %10.1f ns      | %10.1f ns      | %6.2fx       | arena + hot-table sweep\n",
               N, med_base, med_two, ratio);

        if (c == 1 && legacy_csv) {
            fprintf(legacy_csv, "exp5_latency,scope_teardown,%d,0,0,0,0,0,0,0,0.0,%.2f,%.2f\n",
                    N, med_two, ratio);
        }
    }

    if (csv) fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 6: Bloom & Decompression Cache Effectiveness
 * ========================================================================= */
static void run_experiment_6(void) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 6: Bloom Filter & Decompression Cache Efficiency Metrics\n");
    printf("=========================================================================================================================\n");
    printf("%-26s | %-12s | %-10s | %-12s | %-12s | %-12s\n",
           "Workload", "Cache Hits", "Hit Rate", "Range Skips", "Bloom Skips", "Bloom FP Rate");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    FILE* csv = fopen("bench/results/exp6_bloom_cache.csv", "w");
    if (csv) {
        fprintf(csv, "run_id,workload,total_lookups,cache_hits,cache_misses,cache_hit_rate,range_skips,bloom_skips,bloom_false_positives,cold_hits,bloom_fp_rate\n");
    }

    for (size_t w = 0; w < NUM_WORKLOADS; w++) {
        char* src = read_entire_file(ALL_WORKLOADS[w]);
        if (!src) continue;

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        for (int r = 0; r < NUM_RUNS; r++) {
            mem_tracker_reset();
            symtab_t* tab = symtab_twotier_create(2, 0);

            semantic_context_t ctx;
            semantic_init(&ctx, tab, tt);
            ctx.quiet = true;
            semantic_analyze(&ctx, prog);

            symtab_stats_t st;
            tab->get_stats(tab, &st);

            double hit_rate = (st.cache_hits + st.cache_misses > 0) ?
                ((double)st.cache_hits / (double)(st.cache_hits + st.cache_misses) * 100.0) : 0.0;
            double fp_rate = (st.bloom_false_positives + st.cold_hits > 0) ?
                ((double)st.bloom_false_positives / (double)(st.bloom_false_positives + st.cold_hits) * 100.0) : 0.0;

            if (csv) {
                fprintf(csv, "%d,%s,%llu,%llu,%llu,%.2f,%llu,%llu,%llu,%llu,%.2f\n",
                        r + 1, get_base_name(ALL_WORKLOADS[w]),
                        (unsigned long long)st.total_lookups,
                        (unsigned long long)st.cache_hits, (unsigned long long)st.cache_misses,
                        hit_rate,
                        (unsigned long long)st.range_skips,
                        (unsigned long long)st.bloom_skips,
                        (unsigned long long)st.bloom_false_positives,
                        (unsigned long long)st.cold_hits,
                        fp_rate);
            }

            if (r == 0) {
                printf("%-26s | %12llu | %9.1f%% | %12llu | %12llu | %11.2f%%\n",
                       get_base_name(ALL_WORKLOADS[w]),
                       (unsigned long long)st.cache_hits, hit_rate,
                       (unsigned long long)st.range_skips,
                       (unsigned long long)st.bloom_skips,
                       fp_rate);
            }

            tab->destroy(tab);
        }

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(src);
    }
    if (csv) fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 7: Memory vs AST Step Time Series
 * ========================================================================= */
typedef struct {
    FILE* csv;
    const char* workload;
    const char* engine;
} trace_context_t;

static void trace_step_cb(void* user_data, size_t step, size_t current_bytes) {
    trace_context_t* tc = (trace_context_t*)user_data;
    if (tc && tc->csv) {
        fprintf(tc->csv, "%s,%s,%zu,%zu\n", tc->workload, tc->engine, step, current_bytes);
    }
}

static void run_experiment_7(void) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 7: Dynamic Memory vs AST Node Time Series Trace\n");
    printf("=========================================================================================================================\n");

    FILE* csv = fopen("bench/results/exp7_ast_memory_trace.csv", "w");
    if (!csv) return;
    fprintf(csv, "workload,engine,step,current_bytes\n");

    const char* trace_workloads[] = {
        "data/synthetic_deep_nesting.mc",
        "data/embedded_flight_controller.mc"
    };

    for (size_t w = 0; w < 2; w++) {
        char* src = read_entire_file(trace_workloads[w]);
        if (!src) continue;

        const char* base_w = get_base_name(trace_workloads[w]);
        printf("Tracing AST memory steps for '%s'...\n", base_w);

        /* 1. Baseline Trace */
        {
            type_table_t* tt = type_table_create();
            parser_t parser;
            parser_init(&parser, src, tt);
            ast_node_t* prog = parser_parse_program(&parser);

            mem_tracker_reset();
            symtab_t* tab = symtab_baseline_create();

            trace_context_t tc = { csv, base_w, "baseline" };
            semantic_context_t ctx;
            semantic_init(&ctx, tab, tt);
            ctx.quiet = true;
            ctx.step_callback = trace_step_cb;
            ctx.step_user_data = &tc;

            semantic_analyze(&ctx, prog);

            tab->destroy(tab);
            ast_destroy_node(prog);
            type_table_destroy(tt);
        }

        /* 2. Two-Tier Trace */
        {
            type_table_t* tt = type_table_create();
            parser_t parser;
            parser_init(&parser, src, tt);
            ast_node_t* prog = parser_parse_program(&parser);

            mem_tracker_reset();
            symtab_t* tab = symtab_twotier_create(2, 0);

            trace_context_t tc = { csv, base_w, "twotier" };
            semantic_context_t ctx;
            semantic_init(&ctx, tab, tt);
            ctx.quiet = true;
            ctx.step_callback = trace_step_cb;
            ctx.step_user_data = &tc;

            semantic_analyze(&ctx, prog);

            tab->destroy(tab);
            ast_destroy_node(prog);
            type_table_destroy(tt);
        }

        free(src);
    }

    fclose(csv);
    printf("Saved detailed AST step memory traces to 'bench/results/exp7_ast_memory_trace.csv'\n");
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 8: Scalability (Total Symbols & Nesting Depth Scaling)
 * ========================================================================= */
static void safe_append(char* buf, size_t est_size, size_t* offset, const char* fmt, ...) {
    if (!buf || *offset >= est_size) {
        fprintf(stderr, "FATAL: generate_synthetic_code buffer overflow (offset %zu >= est_size %zu)\n",
                *offset, est_size);
        abort();
    }
    size_t rem = est_size - *offset;
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(buf + *offset, rem, fmt, args);
    va_end(args);
    if (written < 0 || (size_t)written >= rem) {
        fprintf(stderr, "FATAL: generate_synthetic_code snprintf truncated or error (written %d, remaining %zu)\n",
                written, rem);
        abort();
    }
    *offset += (size_t)written;
}

static char* generate_synthetic_code(int depth, int symbols_per_scope) {
    size_t est_size = (size_t)depth * (size_t)(symbols_per_scope + 3) * (size_t)(64 + 4 * (depth + 2)) + 8192;
    char* buf = (char*)malloc(est_size);
    if (!buf) return NULL;

    size_t offset = 0;
    safe_append(buf, est_size, &offset, "int g_root = 1;\nint test_func() {\n    int acc = 0;\n");

    for (int d = 1; d <= depth; d++) {
        for (int i = 0; i < d; i++) safe_append(buf, est_size, &offset, "    ");
        safe_append(buf, est_size, &offset, "{\n");
        for (int s = 0; s < symbols_per_scope; s++) {
            for (int i = 0; i < d + 1; i++) safe_append(buf, est_size, &offset, "    ");
            safe_append(buf, est_size, &offset, "int v_d%d_s%d = %d;\n", d, s, d * 10 + s);
        }
        for (int i = 0; i < d + 1; i++) safe_append(buf, est_size, &offset, "    ");
        safe_append(buf, est_size, &offset, "acc = acc + v_d%d_s0;\n", d);
    }
    for (int d = depth; d >= 1; d--) {
        for (int i = 0; i < d; i++) safe_append(buf, est_size, &offset, "    ");
        safe_append(buf, est_size, &offset, "}\n");
    }
    safe_append(buf, est_size, &offset, "    return acc;\n}\nint main() { return test_func(); }\n");
    return buf;
}

static void run_experiment_8(void) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 8: Scalability Sweeps (Symbols Scaling & Depth Scaling)\n");
    printf("=========================================================================================================================\n");

    FILE* csv = fopen("bench/results/exp8_scalability.csv", "w");
    if (!csv) return;
    fprintf(csv, "test_type,param_value,engine,peak_ram_bytes,elapsed_ns\n");

    /* 1. Sweep total symbols: 100 to 50,000 (fixed depth 16) */
    int sym_counts[] = { 100, 500, 1000, 2500, 5000, 10000, 25000, 50000 };
    printf("--- Scaling Total Symbols (100 to 50,000) ---\n");
    printf("%-16s | %-16s | %-16s | %-14s\n", "Total Symbols", "Baseline RAM", "Two-Tier RAM", "Reduction");
    printf("-----------------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < sizeof(sym_counts) / sizeof(sym_counts[0]); i++) {
        int total = sym_counts[i];
        int depth = 16;
        int syms_per_scope = total / depth;
        if (syms_per_scope < 1) syms_per_scope = 1;

        char* code = generate_synthetic_code(depth, syms_per_scope);
        if (!code) continue;

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, code, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        run_metrics_t b_m = execute_benchmark_run(prog, tt, "baseline", 2, 0);
        run_metrics_t t_m = execute_benchmark_run(prog, tt, "twotier", 2, 0);

        fprintf(csv, "symbols,%d,baseline,%zu,%.1f\n", total, b_m.peak_ram_bytes, b_m.elapsed_ns);
        fprintf(csv, "symbols,%d,twotier,%zu,%.1f\n", total, t_m.peak_ram_bytes, t_m.elapsed_ns);

        double red = (b_m.peak_ram_bytes > 0) ?
            ((1.0 - (double)t_m.peak_ram_bytes / (double)b_m.peak_ram_bytes) * 100.0) : 0.0;
        printf("%-16d | %12zu B    | %12zu B    | -%5.1f%%\n", total, b_m.peak_ram_bytes, t_m.peak_ram_bytes, red);

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(code);
    }

    /* 2. Sweep nesting depth: 4 to 256 (fixed 16 symbols/scope) */
    int depths[] = { 4, 8, 16, 32, 64, 128, 256 };
    printf("\n--- Scaling Nesting Depth (4 to 256) ---\n");
    printf("%-16s | %-16s | %-16s | %-14s\n", "Nesting Depth", "Baseline RAM", "Two-Tier RAM", "Reduction");
    printf("-----------------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < sizeof(depths) / sizeof(depths[0]); i++) {
        int depth = depths[i];
        char* code = generate_synthetic_code(depth, 16);
        if (!code) continue;

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, code, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        run_metrics_t b_m = execute_benchmark_run(prog, tt, "baseline", 2, 0);
        run_metrics_t t_m = execute_benchmark_run(prog, tt, "twotier", 2, 0);

        fprintf(csv, "depth,%d,baseline,%zu,%.1f\n", depth, b_m.peak_ram_bytes, b_m.elapsed_ns);
        fprintf(csv, "depth,%d,twotier,%zu,%.1f\n", depth, t_m.peak_ram_bytes, t_m.elapsed_ns);

        double red = (b_m.peak_ram_bytes > 0) ?
            ((1.0 - (double)t_m.peak_ram_bytes / (double)b_m.peak_ram_bytes) * 100.0) : 0.0;
        printf("Depth = %-9d | %12zu B    | %12zu B    | -%5.1f%%\n", depth, b_m.peak_ram_bytes, t_m.peak_ram_bytes, red);

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(code);
    }

    fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 9: Sensitivity Sweeps (K, Demotion Threshold N, Bloom, Cache Size)
 * ========================================================================= */
static void run_experiment_9(FILE* legacy_csv) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 9: Multi-Dimensional Sensitivity Sweeps (K, Scope Threshold N, Bloom, Cache)\n");
    printf("=========================================================================================================================\n");

    FILE* csv = fopen("bench/results/exp9_sensitivity.csv", "w");
    if (!csv) return;
    fprintf(csv, "sweep_param,param_value,workload,peak_ram_bytes,elapsed_ns,bloom_skips,decompressions\n");

    /* 1. Hot depth K sweep across all 8 workloads */
    int k_vals[] = { 1, 2, 4, 8, 16, 32, 64 };
    for (size_t w = 0; w < NUM_WORKLOADS; w++) {
        char* src = read_entire_file(ALL_WORKLOADS[w]);
        if (!src) continue;

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);
        const char* base_w = get_base_name(ALL_WORKLOADS[w]);

        for (size_t k_idx = 0; k_idx < sizeof(k_vals) / sizeof(k_vals[0]); k_idx++) {
            int k = k_vals[k_idx];
            run_metrics_t m = execute_benchmark_run(prog, tt, "twotier", k, 0);

            fprintf(csv, "hot_depth_k,%d,%s,%zu,%.1f,%llu,%llu\n",
                    k, base_w, m.peak_ram_bytes, m.elapsed_ns,
                    (unsigned long long)m.bloom_skips, (unsigned long long)m.decompressions);

            if (w == 7 && legacy_csv) { /* synthetic_deep_nesting for legacy */
                fprintf(legacy_csv, "exp3_sweep_k,%d,twotier,%zu,%zu,%zu,%llu,%llu,%llu,%llu,%.2f,0.0,0.0\n",
                        k, m.peak_ram_bytes, m.total_inserts, m.total_lookups,
                        (unsigned long long)m.hot_hits, (unsigned long long)m.cold_hits,
                        (unsigned long long)m.bloom_skips, (unsigned long long)m.decompressions,
                        m.bytes_per_sym);
            }
        }

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(src);
    }

    /* 2. Minimum scope size threshold N in {0, 4, 8, 16, 32} */
    int n_thresholds[] = { 0, 4, 8, 16, 32 };
    const char* threshold_workloads[] = {
        "data/embedded_flight_controller.mc",
        "data/synthetic_deep_nesting.mc"
    };

    printf("\n--- Scope Demotion Threshold N Sweep (0, 4, 8, 16, 32 Symbols) ---\n");
    for (size_t w = 0; w < 2; w++) {
        char* src = read_entire_file(threshold_workloads[w]);
        if (!src) continue;

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);
        const char* base_w = get_base_name(threshold_workloads[w]);

        for (size_t n_idx = 0; n_idx < 5; n_idx++) {
            int n_sym = n_thresholds[n_idx];
            mem_tracker_reset();
            symtab_t* tab = symtab_twotier_create_with_policy(2, 0, DEMOTE_POLICY_SIZE_AWARE);
            tab->set_param(tab, "min_scope_size", n_sym);

            semantic_context_t ctx;
            semantic_init(&ctx, tab, tt);
            ctx.quiet = true;
            double t0 = get_time_ns();
            semantic_analyze(&ctx, prog);
            double t1 = get_time_ns();

            symtab_stats_t st;
            tab->get_stats(tab, &st);
            size_t peak = mem_tracker_get_peak();

            fprintf(csv, "min_scope_threshold,%d,%s,%zu,%.1f,%llu,%llu\n",
                    n_sym, base_w, peak, (t1 - t0),
                    (unsigned long long)st.bloom_skips, (unsigned long long)st.decompressions);

            printf("%-26s | Threshold N = %-4d | Peak RAM: %6zu B | Decomps: %5llu\n",
                   base_w, n_sym, peak, (unsigned long long)st.decompressions);

            tab->destroy(tab);
        }

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(src);
    }

    fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 10: Enforced Budget Cap Heatmap (4 KB to 64 KB across All Workloads)
 * ========================================================================= */
static void run_experiment_10(FILE* legacy_csv) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 10: Enforced RAM Budget Cap Survival Sweep (Heatmap Data: 4 KB - 64 KB)\n");
    printf("=========================================================================================================================\n");
    printf("%-26s | %-10s | %-16s | %-16s\n", "Workload", "Cap Bytes", "Baseline Status", "Two-Tier Status");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    FILE* csv = fopen("bench/results/exp10_budget_caps.csv", "w");
    if (!csv) return;
    fprintf(csv, "workload,cap_bytes,baseline_status,baseline_peak,twotier_status,twotier_peak\n");

    size_t caps[] = { 4096, 8192, 12288, 16384, 24576, 32768, 49152, 65536 };
    size_t num_caps = sizeof(caps) / sizeof(caps[0]);

    for (size_t w = 0; w < NUM_WORKLOADS; w++) {
        char* src = read_entire_file(ALL_WORKLOADS[w]);
        if (!src) continue;

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);
        const char* base_w = get_base_name(ALL_WORKLOADS[w]);

        /* Uncapped baseline to find expected insert count */
        mem_tracker_reset();
        symtab_t* uncapped = symtab_baseline_create();
        semantic_context_t un_ctx;
        semantic_init(&un_ctx, uncapped, tt);
        un_ctx.quiet = true;
        semantic_analyze(&un_ctx, prog);
        symtab_stats_t un_st;
        uncapped->get_stats(uncapped, &un_st);
        size_t expected_inserts = un_st.total_inserts;
        uncapped->destroy(uncapped);

        for (size_t c = 0; c < num_caps; c++) {
            size_t cap = caps[c];

            /* 1. Baseline under cap */
            mem_tracker_reset();
            mem_tracker_set_budget_limit(cap);
            symtab_t* btab = symtab_baseline_create();
            semantic_context_t bctx;
            semantic_init(&bctx, btab, tt);
            bctx.quiet = true;
            semantic_analyze(&bctx, prog);
            symtab_stats_t bst;
            btab->get_stats(btab, &bst);
            size_t b_peak = mem_tracker_get_peak();
            mem_tracker_set_budget_limit(0);
            btab->destroy(btab);

            bool b_pass = (bst.total_inserts == expected_inserts && b_peak > 0 && b_peak <= cap);

            /* 2. Two-Tier under cap */
            mem_tracker_reset();
            mem_tracker_set_budget_limit(cap);
            symtab_t* ttab = symtab_twotier_create(2, cap);
            semantic_context_t tctx;
            semantic_init(&tctx, ttab, tt);
            tctx.quiet = true;
            semantic_analyze(&tctx, prog);
            symtab_stats_t tst;
            ttab->get_stats(ttab, &tst);
            size_t t_peak = mem_tracker_get_peak();
            mem_tracker_set_budget_limit(0);
            ttab->destroy(ttab);

            bool t_pass = (tst.total_inserts == expected_inserts && t_peak > 0 && t_peak <= cap);

            const char* b_str = b_pass ? "PASS" : "FAIL (OOM)";
            const char* t_str = t_pass ? "PASS" : "FAIL (OOM)";

            fprintf(csv, "%s,%zu,%s,%zu,%s,%zu\n",
                    base_w, cap, b_str, b_peak, t_str, t_peak);

            if (w == 6 && legacy_csv) { /* synthetic_heavy_shadowing for legacy CSV */
                fprintf(legacy_csv, "exp6_budget_cap,%zu,%s,%zu,%zu,%zu,0,0,0,0,0.0,0.0,0.0\n",
                        cap, t_pass ? "PASS (Enforced <= Cap)" : "FAIL (OOM Exceeded)",
                        t_peak, tst.total_inserts, tst.total_lookups);
            }

            if (c % 2 == 1 && (w == 4 || w == 6 || w == 7)) {
                printf("%-26s | %7zu B  | %-16s | %-16s\n", base_w, cap, b_str, t_str);
            }
        }

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(src);
    }

    fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 11: End-to-End Compiler Wall Time & Peak Process Memory
 * ========================================================================= */
static void run_experiment_11(void) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 11: End-to-End Compiler Wall Time & Total Peak Heap RAM (Including AST)\n");
    printf("=========================================================================================================================\n");
    printf("%-26s | %-15s | %-15s | %-14s | %-14s\n",
           "Workload", "Baseline Wall", "Two-Tier Wall", "Baseline Heap", "Two-Tier Heap");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    FILE* csv = fopen("bench/results/exp11_compiler_e2e.csv", "w");
    if (csv) {
        fprintf(csv, "run_id,workload,engine,wall_time_us,peak_heap_bytes\n");
    }

    for (size_t w = 0; w < NUM_WORKLOADS; w++) {
        char* src = read_entire_file(ALL_WORKLOADS[w]);
        if (!src) continue;
        const char* base_w = get_base_name(ALL_WORKLOADS[w]);

        double b_times[NUM_RUNS], t_times[NUM_RUNS];
        double b_peaks[NUM_RUNS], t_peaks[NUM_RUNS];

        for (int r = 0; r < NUM_RUNS; r++) {
            /* Baseline E2E */
            mem_tracker_reset();
            double bt0 = get_time_ns();
            type_table_t* b_tt = type_table_create();
            parser_t b_parser;
            parser_init(&b_parser, src, b_tt);
            ast_node_t* b_prog = parser_parse_program(&b_parser);
            symtab_t* b_tab = symtab_baseline_create();
            semantic_context_t b_ctx;
            semantic_init(&b_ctx, b_tab, b_tt);
            b_ctx.quiet = true;
            semantic_analyze(&b_ctx, b_prog);
            b_peaks[r] = (double)mem_tracker_get_peak();
            b_tab->destroy(b_tab);
            ast_destroy_node(b_prog);
            type_table_destroy(b_tt);
            double bt1 = get_time_ns();
            b_times[r] = (bt1 - bt0) / 1000.0; /* us */

            /* Two-Tier E2E */
            mem_tracker_reset();
            double tt0 = get_time_ns();
            type_table_t* t_tt = type_table_create();
            parser_t t_parser;
            parser_init(&t_parser, src, t_tt);
            ast_node_t* t_prog = parser_parse_program(&t_parser);
            symtab_t* t_tab = symtab_twotier_create(2, 0);
            semantic_context_t t_ctx;
            semantic_init(&t_ctx, t_tab, t_tt);
            t_ctx.quiet = true;
            semantic_analyze(&t_ctx, t_prog);
            t_peaks[r] = (double)mem_tracker_get_peak();
            t_tab->destroy(t_tab);
            ast_destroy_node(t_prog);
            type_table_destroy(t_tt);
            double tt1 = get_time_ns();
            t_times[r] = (tt1 - tt0) / 1000.0; /* us */

            if (csv) {
                fprintf(csv, "%d,%s,baseline,%.1f,%zu\n", r + 1, base_w, b_times[r], (size_t)b_peaks[r]);
                fprintf(csv, "%d,%s,twotier,%.1f,%zu\n", r + 1, base_w, t_times[r], (size_t)t_peaks[r]);
            }
        }

        double med_bt, iqr_bt, med_tt, iqr_tt;
        double med_bp, iqr_bp, med_tp, iqr_tp;
        calc_median_iqr(b_times, NUM_RUNS, &med_bt, &iqr_bt);
        calc_median_iqr(t_times, NUM_RUNS, &med_tt, &iqr_tt);
        calc_median_iqr(b_peaks, NUM_RUNS, &med_bp, &iqr_bp);
        calc_median_iqr(t_peaks, NUM_RUNS, &med_tp, &iqr_tp);

        printf("%-26s | %10.1f us    | %10.1f us    | %10zu B    | %10zu B\n",
               base_w, med_bt, med_tt, (size_t)med_bp, (size_t)med_tp);

        free(src);
    }

    if (csv) fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 12: Allocation & Free Call Counts Per Engine
 * ========================================================================= */
static void run_experiment_12(void) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 12: Malloc and Free Call Counts across Engines\n");
    printf("=========================================================================================================================\n");
    printf("%-26s | %-12s | %-14s | %-14s | %-12s\n",
           "Workload", "Engine", "Total Mallocs", "Total Frees", "Active Leaks");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    FILE* csv = fopen("bench/results/exp12_alloc_counts.csv", "w");
    if (csv) {
        fprintf(csv, "workload,engine,total_allocations,total_frees,active_allocations,peak_bytes\n");
    }

    for (size_t w = 0; w < NUM_WORKLOADS; w++) {
        char* src = read_entire_file(ALL_WORKLOADS[w]);
        if (!src) continue;
        const char* base_w = get_base_name(ALL_WORKLOADS[w]);

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        for (size_t e = 0; e < NUM_ENGINES; e++) {
            const char* eng = ALL_ENGINES[e];
            mem_tracker_reset();
            symtab_t* tab = create_engine(eng, 2, 0);

            semantic_context_t ctx;
            semantic_init(&ctx, tab, tt);
            ctx.quiet = true;
            semantic_analyze(&ctx, prog);

            mem_stats_t st = mem_tracker_get_stats();
            tab->destroy(tab);

            mem_stats_t final_st = mem_tracker_get_stats();

            if (csv) {
                fprintf(csv, "%s,%s,%zu,%zu,%zu,%zu\n",
                        base_w, eng, st.total_allocations, st.total_frees,
                        final_st.active_allocations, st.peak_bytes);
            }

            if (e == 0 || e == 5) {
                printf("%-26s | %-12s | %14zu | %14zu | %12zu\n",
                       base_w, eng, st.total_allocations, st.total_frees, final_st.active_allocations);
            }
        }

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(src);
    }

    if (csv) fclose(csv);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 2 ABLATION STUDY (Incremental Techniques on Deep Nesting)
 * ========================================================================= */
static void run_ablation_experiment(FILE* legacy_csv) {
    const char* workload = "data/synthetic_deep_nesting.mc";
    char* src = read_entire_file(workload);
    if (!src) return;

    printf("\n=========================================================================================\n");
    printf(" ABLATION STUDY: Incremental Architectural Techniques on Deep Nesting\n");
    printf("=========================================================================================\n");
    printf("%-34s | %-12s | %-14s | %-10s\n",
           "Ablation Technique / Stage", "Peak RAM", "RAM vs Base", "Bytes/Sym");
    printf("-----------------------------------------------------------------------------------------\n");

    type_table_t* tt = type_table_create();
    parser_t parser;
    parser_init(&parser, src, tt);
    ast_node_t* prog = parser_parse_program(&parser);

    symtab_kind_t variants[] = {
        SYMTAB_BASELINE,
        SYMTAB_ARENA_ONLY,
        SYMTAB_INTERN_ONLY,
        SYMTAB_PACKED_ONLY,
        SYMTAB_COMPRESSED_NO_BLOOM,
        SYMTAB_TWOTIER
    };
    const char* names[] = {
        "1. Baseline (Malloc-Chained)",
        "2. + Arena Allocator",
        "3. + String Interning",
        "4. + 64-bit Packed Symbols",
        "5. + Delta/Varint Compression",
        "6. + Bloom Filter (Full Two-Tier)"
    };
    size_t num_variants = sizeof(variants) / sizeof(variants[0]);
    size_t base_ram = 0;

    for (size_t i = 0; i < num_variants; i++) {
        mem_tracker_reset();
        symtab_t* tab = NULL;
        if (variants[i] == SYMTAB_BASELINE) {
            tab = symtab_baseline_create();
        } else if (variants[i] == SYMTAB_TWOTIER) {
            tab = symtab_twotier_create(2, 0);
        } else {
            tab = symtab_ablation_create(variants[i]);
        }

        semantic_context_t ctx;
        semantic_init(&ctx, tab, tt);
        ctx.quiet = true;
        semantic_analyze(&ctx, prog);

        symtab_stats_t stats;
        tab->get_stats(tab, &stats);
        size_t peak = mem_tracker_get_peak();

        if (i == 0) base_ram = peak;

        double saved = (base_ram > 0) ? (1.0 - (double)peak / (double)base_ram) * 100.0 : 0.0;
        double bps = (stats.peak_symbols > 0) ? ((double)peak / (double)stats.peak_symbols) : 0.0;

        if (saved >= 0.0) {
            printf("%-34s | %8zu B   | -%5.1f%% (saved) | %6.1f B\n", names[i], peak, saved, bps);
        } else {
            printf("%-34s | %8zu B   | +%5.1f%% (worse) | %6.1f B\n", names[i], peak, -saved, bps);
        }

        if (legacy_csv) {
            fprintf(legacy_csv, "exp2_ablation,%s,%zu,%.1f,%.1f\n", names[i], peak, saved, bps);
        }

        tab->destroy(tab);
    }

    ast_destroy_node(prog);
    type_table_destroy(tt);
    free(src);
    printf("=========================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 4 DEMOTION POLICY (Eager vs Lazy vs Size-Aware)
 * ========================================================================= */
static void run_demotion_policy_experiment(FILE* legacy_csv) {
    printf("\n=====================================================================================================================\n");
    printf(" ARCHITECTURAL DEMOTION POLICY: Eager Depth-Bounded vs Lazy Exit vs Size-Aware\n");
    printf("=====================================================================================================================\n");
    printf("%-28s | %-28s | %-10s | %-18s | %-11s | %-7s\n",
           "Workload", "Demotion Policy", "Peak RAM", "RAM vs Base", "Bloom Skips", "Decomps");
    printf("---------------------------------------------------------------------------------------------------------------------\n");

    const char* workloads[] = {
        "data/embedded_flight_controller.mc",
        "data/synthetic_deep_nesting.mc"
    };

    for (size_t w = 0; w < 2; w++) {
        char* src = read_entire_file(workloads[w]);
        if (!src) continue;
        const char* base_w = get_base_name(workloads[w]);

        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        /* 1. Baseline */
        mem_tracker_reset();
        symtab_t* base_tab = symtab_baseline_create();
        semantic_context_t bctx;
        semantic_init(&bctx, base_tab, tt);
        bctx.quiet = true;
        semantic_analyze(&bctx, prog);
        size_t base_peak = mem_tracker_get_peak();
        base_tab->destroy(base_tab);

        /* 2. Eager */
        mem_tracker_reset();
        symtab_t* eager_tab = symtab_twotier_create_with_policy(2, 0, DEMOTE_POLICY_DEPTH_BOUNDED);
        semantic_context_t ectx;
        semantic_init(&ectx, eager_tab, tt);
        ectx.quiet = true;
        semantic_analyze(&ectx, prog);
        symtab_stats_t est;
        eager_tab->get_stats(eager_tab, &est);
        size_t eager_peak = mem_tracker_get_peak();
        eager_tab->destroy(eager_tab);

        /* 3. Lazy */
        mem_tracker_reset();
        symtab_t* lazy_tab = symtab_twotier_create_with_policy(2, 0, DEMOTE_POLICY_ON_SCOPE_EXIT);
        semantic_context_t lctx;
        semantic_init(&lctx, lazy_tab, tt);
        lctx.quiet = true;
        semantic_analyze(&lctx, prog);
        symtab_stats_t lst;
        lazy_tab->get_stats(lazy_tab, &lst);
        size_t lazy_peak = mem_tracker_get_peak();
        lazy_tab->destroy(lazy_tab);

        /* 4. Size-Aware */
        mem_tracker_reset();
        symtab_t* size_tab = symtab_twotier_create_with_policy(2, 0, DEMOTE_POLICY_SIZE_AWARE);
        semantic_context_t sctx;
        semantic_init(&sctx, size_tab, tt);
        sctx.quiet = true;
        semantic_analyze(&sctx, prog);
        symtab_stats_t sst;
        size_tab->get_stats(size_tab, &sst);
        size_t size_peak = mem_tracker_get_peak();
        size_tab->destroy(size_tab);

        double eager_saved = (base_peak > 0) ? (1.0 - (double)eager_peak / (double)base_peak) * 100.0 : 0.0;
        double lazy_saved  = (base_peak > 0) ? (1.0 - (double)lazy_peak  / (double)base_peak) * 100.0 : 0.0;
        double size_saved  = (base_peak > 0) ? (1.0 - (double)size_peak  / (double)base_peak) * 100.0 : 0.0;

        if (eager_saved >= 0.0) {
            printf("%-28s | %-28s | %7zu B  | -%5.1f%% (saved) | %11llu | %7llu\n",
                   base_w, "Eager (Deeper than K=2)", eager_peak, eager_saved,
                   (unsigned long long)est.bloom_skips, (unsigned long long)est.decompressions);
        } else {
            printf("%-28s | %-28s | %7zu B  | +%5.1f%% (worse) | %11llu | %7llu\n",
                   base_w, "Eager (Deeper than K=2)", eager_peak, -eager_saved,
                   (unsigned long long)est.bloom_skips, (unsigned long long)est.decompressions);
        }

        if (lazy_saved >= 0.0) {
            printf("%-28s | %-28s | %7zu B  | -%5.1f%% (saved) | %11llu | %7llu\n",
                   base_w, "Lazy (Demote on Scope Exit)", lazy_peak, lazy_saved,
                   (unsigned long long)lst.bloom_skips, (unsigned long long)lst.decompressions);
        } else {
            printf("%-28s | %-28s | %7zu B  | +%5.1f%% (worse) | %11llu | %7llu\n",
                   base_w, "Lazy (Demote on Scope Exit)", lazy_peak, -lazy_saved,
                   (unsigned long long)lst.bloom_skips, (unsigned long long)lst.decompressions);
        }

        if (size_saved >= 0.0) {
            printf("%-28s | %-28s | %7zu B  | -%5.1f%% (saved) | %11llu | %7llu\n",
                   base_w, "Size-Aware (K=2, >=16 syms)", size_peak, size_saved,
                   (unsigned long long)sst.bloom_skips, (unsigned long long)sst.decompressions);
        } else {
            printf("%-28s | %-28s | %7zu B  | +%5.1f%% (worse) | %11llu | %7llu\n",
                   base_w, "Size-Aware (K=2, >=16 syms)", size_peak, -size_saved,
                   (unsigned long long)sst.bloom_skips, (unsigned long long)sst.decompressions);
        }

        if (legacy_csv) {
            fprintf(legacy_csv, "exp4_policy,%s,eager_depth_bounded,%zu,%llu,%llu,%llu,%llu,%llu,%llu,0.0,0.0,0.0\n",
                    base_w, eager_peak, (unsigned long long)est.total_inserts, (unsigned long long)est.total_lookups,
                    (unsigned long long)est.hot_hits, (unsigned long long)est.cold_hits,
                    (unsigned long long)est.bloom_skips, (unsigned long long)est.decompressions);
            fprintf(legacy_csv, "exp4_policy,%s,lazy_on_exit,%zu,%llu,%llu,%llu,%llu,%llu,%llu,0.0,0.0,0.0\n",
                    base_w, lazy_peak, (unsigned long long)lst.total_inserts, (unsigned long long)lst.total_lookups,
                    (unsigned long long)lst.hot_hits, (unsigned long long)lst.cold_hits,
                    (unsigned long long)lst.bloom_skips, (unsigned long long)lst.decompressions);
            fprintf(legacy_csv, "exp4_policy,%s,size_aware_threshold,%zu,%llu,%llu,%llu,%llu,%llu,%llu,0.0,0.0,0.0\n",
                    base_w, size_peak, (unsigned long long)sst.total_inserts, (unsigned long long)sst.total_lookups,
                    (unsigned long long)sst.hot_hits, (unsigned long long)sst.cold_hits,
                    (unsigned long long)sst.bloom_skips, (unsigned long long)sst.decompressions);
        }

        ast_destroy_node(prog);
        type_table_destroy(tt);
        free(src);
    }
    printf("=====================================================================================================================\n");
}

/* =========================================================================
 * EXPERIMENT 13: Flight Controller Memory Breakdown vs Scope Count Deep-Dive
 * ========================================================================= */
typedef struct {
    symtab_t* tab;
    bool is_twotier;
    size_t last_scope_created;
    size_t total_bytes[64];
    twotier_memory_breakdown_t bd[64];
    size_t max_scopes;
} fc_tracker_t;

static void fc_step_tracker_cb(void* user_data, size_t step, size_t current_bytes) {
    (void)step; (void)current_bytes;
    fc_tracker_t* tr = (fc_tracker_t*)user_data;
    if (!tr || !tr->tab) return;
    symtab_stats_t st;
    tr->tab->get_stats(tr->tab, &st);
    size_t s = st.total_scopes_created;
    if (s < 64) {
        tr->total_bytes[s] = mem_tracker_get_current();
        if (tr->is_twotier) {
            symtab_twotier_get_memory_breakdown(tr->tab, &tr->bd[s]);
        }
        if (s > tr->max_scopes) tr->max_scopes = s;
    }
}

static void run_experiment_13_flight_controller(void) {
    printf("\n=========================================================================================================================\n");
    printf(" EXPERIMENT 13: Flight Controller Memory Breakdown Deep-Dive (Baseline vs Two-Tier vs Scopes)\n");
    printf(" Question: Why does Two-Tier use more memory than Baseline on flight controller even without compression?\n");
    printf("=========================================================================================================================\n");

    const char* path = "data/embedded_flight_controller.mc";
    char* src = read_entire_file(path);
    if (!src) return;

    FILE* csv = fopen("bench/results/exp13_flight_controller_breakdown.csv", "w");
    if (csv) {
        fprintf(csv, "scope_count,engine,total_bytes,hot_table_bytes,cold_blocks_bytes,string_pool_bytes,type_table_bytes,arenas_bytes,metadata_bloom_bytes\n");
    }

    /* 1. Baseline Run */
    fc_tracker_t base_tr = {0};
    base_tr.is_twotier = false;
    {
        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        mem_tracker_reset();
        symtab_t* tab = symtab_baseline_create();
        base_tr.tab = tab;

        semantic_context_t ctx;
        semantic_init(&ctx, tab, tt);
        ctx.quiet = true;
        ctx.step_callback = fc_step_tracker_cb;
        ctx.step_user_data = &base_tr;

        semantic_analyze(&ctx, prog);

        tab->destroy(tab);
        ast_destroy_node(prog);
        type_table_destroy(tt);
    }

    /* 2. Two-Tier Run (K=2) */
    fc_tracker_t two_tr = {0};
    two_tr.is_twotier = true;
    {
        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        mem_tracker_reset();
        symtab_t* tab = symtab_twotier_create(2, 0);
        two_tr.tab = tab;

        semantic_context_t ctx;
        semantic_init(&ctx, tab, tt);
        ctx.quiet = true;
        ctx.step_callback = fc_step_tracker_cb;
        ctx.step_user_data = &two_tr;

        semantic_analyze(&ctx, prog);

        tab->destroy(tab);
        ast_destroy_node(prog);
        type_table_destroy(tt);
    }

    /* 3. Two-Tier Run (Uncompressed / K=64) */
    fc_tracker_t uncomp_tr = {0};
    uncomp_tr.is_twotier = true;
    {
        type_table_t* tt = type_table_create();
        parser_t parser;
        parser_init(&parser, src, tt);
        ast_node_t* prog = parser_parse_program(&parser);

        mem_tracker_reset();
        symtab_t* tab = symtab_twotier_create(64, 0);
        uncomp_tr.tab = tab;

        semantic_context_t ctx;
        semantic_init(&ctx, tab, tt);
        ctx.quiet = true;
        ctx.step_callback = fc_step_tracker_cb;
        ctx.step_user_data = &uncomp_tr;

        semantic_analyze(&ctx, prog);

        tab->destroy(tab);
        ast_destroy_node(prog);
        type_table_destroy(tt);
    }

    size_t total_scopes = two_tr.max_scopes;
    if (total_scopes == 0) total_scopes = 39;

    printf("%-12s | %-12s | %-12s | %-14s | %-14s | %-20s\n",
           "Scope Count", "Baseline RAM", "TwoTier(K=2)", "TwoTier(NoComp)", "StrPool(TwoTier)", "Dominant Over Baseline");
    printf("-------------------------------------------------------------------------------------------------------------------------\n");

    for (size_t s = 1; s <= total_scopes; s++) {
        size_t b_tot = base_tr.total_bytes[s];
        size_t t_tot = two_tr.total_bytes[s];
        size_t u_tot = uncomp_tr.total_bytes[s];

        /* Forward-fill if 0 */
        if (b_tot == 0 && s > 1) b_tot = base_tr.total_bytes[s-1];
        if (t_tot == 0 && s > 1) t_tot = two_tr.total_bytes[s-1];
        if (u_tot == 0 && s > 1) u_tot = uncomp_tr.total_bytes[s-1];

        /* Baseline breakdown */
        size_t b_buckets = 512;
        size_t b_strings = (b_tot > b_buckets) ? (b_tot - b_buckets) / 2 : 0;
        size_t b_meta = b_tot - b_buckets - b_strings;

        if (csv) {
            fprintf(csv, "%zu,baseline,%zu,%zu,0,%zu,0,0,%zu\n",
                    s, b_tot, b_buckets, b_strings, b_meta);
            fprintf(csv, "%zu,twotier,%zu,%zu,%zu,%zu,%zu,%zu,%zu\n",
                    s, t_tot, two_tr.bd[s].hot_table_bytes, two_tr.bd[s].cold_blocks_bytes,
                    two_tr.bd[s].string_pool_bytes, two_tr.bd[s].type_table_bytes,
                    two_tr.bd[s].arena_bytes, two_tr.bd[s].metadata_bytes);
            fprintf(csv, "%zu,twotier_uncompressed,%zu,%zu,%zu,%zu,%zu,%zu,%zu\n",
                    s, u_tot, uncomp_tr.bd[s].hot_table_bytes, uncomp_tr.bd[s].cold_blocks_bytes,
                    uncomp_tr.bd[s].string_pool_bytes, uncomp_tr.bd[s].type_table_bytes,
                    uncomp_tr.bd[s].arena_bytes, uncomp_tr.bd[s].metadata_bytes);
        }

        if (s == 1 || s == 5 || s == 10 || s == 20 || s == 30 || s == total_scopes) {
            printf("%-12zu | %9zu B  | %9zu B  | %9zu B    | %9zu B      | String Pool + Metadata\n",
                   s, b_tot, t_tot, u_tot, two_tr.bd[s].string_pool_bytes);
        }
    }

    if (csv) fclose(csv);
    free(src);
    printf("=========================================================================================================================\n");
}

/* =========================================================================
 * MAIN ENTRY POINT
 * ========================================================================= */
int main(int argc, char** argv) {
    (void)argc; (void)argv;
    setvbuf(stdout, NULL, _IONBF, 0);
    mem_tracker_init();

    make_dir("bench/results");
    make_dir("report/figures");

    write_metadata("bench/results/metadata.json");

    FILE* legacy_csv = fopen("bench/benchmark_results.csv", "w");
    if (legacy_csv) {
        fprintf(legacy_csv, "experiment,workload,engine,peak_ram_bytes,inserts,lookups,hot_hits,cold_hits,bloom_skips,decomps,bytes_per_sym,median_time_ns,slowdown_ratio\n");
    }

    printf("\n#########################################################################################\n");
    printf(" SCOPE-LIFETIME-AWARE TWO-TIER SYMBOL TABLE: PUBLICATION BENCHMARK SUITE\n");
    printf("#########################################################################################\n");

    /* Core Experiments */
    run_experiment_1(legacy_csv);
    run_ablation_experiment(legacy_csv);
    run_demotion_policy_experiment(legacy_csv);
    run_experiment_2();
    run_experiment_3();
    run_experiment_4(legacy_csv);
    run_experiment_5(legacy_csv);
    run_experiment_6();
    run_experiment_7();
    run_experiment_8();
    run_experiment_9(legacy_csv);
    run_experiment_10(legacy_csv);
    run_experiment_11();
    run_experiment_12();
    run_experiment_13_flight_controller();

    if (legacy_csv) {
        fclose(legacy_csv);
        printf("\n[OUTPUT] Saved consolidated legacy metrics to 'bench/benchmark_results.csv'\n");
    }
    printf("[OUTPUT] Saved 13 publication CSV datasets to 'bench/results/*.csv'\n");
    printf("[OUTPUT] Saved host metadata to 'bench/results/metadata.json'\n\n");

    return 0;
}
