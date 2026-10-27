#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "common/mem_tracker.h"
#include "common/type_table.h"
#include "symtab/symtab_api.h"
#include "symtab/symtab_baseline.h"
#include "symtab/symtab_twotier.h"
#include "symtab/symtab_ablation.h"
#include "compiler/parser.h"
#include "compiler/semantic.h"

static void print_usage(const char* prog) {
    printf("Usage: %s [options] <input_file.mc>\n", prog);
    printf("Options:\n");
    printf("  --symtab <baseline|twotier|arena|intern|packed|nombloom>  Select symbol table engine (default: twotier)\n");
    printf("  --hot-depth <N>                                          Set hot scope depth K (default: 2)\n");
    printf("  --min-scope-size <N>                                     Minimum symbols in scope before demoting (default: 0)\n");
    printf("  --ram-budget <bytes>                                     Enable budget mode with RAM cap\n");
    printf("  --no-bloom                                               Disable Bloom filters in cold blocks\n");
    printf("  --dump-stats                                             Print detailed memory & lookup stats\n");
    printf("  --help                                                   Show this help message\n");
}

static char* read_entire_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    size_t read_bytes = fread(buffer, 1, size, f);
    buffer[read_bytes] = '\0';
    fclose(f);
    return buffer;
}

int main(int argc, char** argv) {
    mem_tracker_init();

    const char* filename = NULL;
    const char* symtab_type = "twotier";
    int hot_depth = 2;
    int min_scope_size = 0;
    size_t ram_budget = 0;
    bool enable_bloom = true;
    bool dump_stats = false;
    bool dump_resolutions = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--symtab") == 0 && i + 1 < argc) {
            symtab_type = argv[++i];
        } else if (strcmp(argv[i], "--hot-depth") == 0 && i + 1 < argc) {
            hot_depth = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--min-scope-size") == 0 && i + 1 < argc) {
            min_scope_size = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--ram-budget") == 0 && i + 1 < argc) {
            ram_budget = (size_t)strtoull(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--no-bloom") == 0) {
            enable_bloom = false;
        } else if (strcmp(argv[i], "--dump-stats") == 0) {
            dump_stats = true;
        } else if (strcmp(argv[i], "--dump-resolutions") == 0) {
            dump_resolutions = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }

    if (!filename) {
        /* Provide an interactive demonstration program */
        printf("No input file provided. Running built-in demonstration...\n");
        const char* demo_code =
            "int g_system_state = 1;\n"
            "int g_counter = 0;\n"
            "struct SensorData { int id; int temp; int humidity; };\n"
            "int process_sensors(int cycle) {\n"
            "    int local_reading = cycle * 10;\n"
            "    {\n"
            "        int filtered = local_reading + g_system_state;\n"
            "        {\n"
            "            int filtered = 42; /* Shadowing outer filtered */\n"
            "            int depth_3_var = filtered + cycle;\n"
            "        }\n"
            "    }\n"
            "    return local_reading;\n"
            "}\n";

        type_table_t* type_table = type_table_create();
        parser_t parser;
        parser_init(&parser, demo_code, type_table);
        ast_node_t* program = parser_parse_program(&parser);

        symtab_t* tab = NULL;
        if (strcmp(symtab_type, "baseline") == 0) {
            tab = symtab_baseline_create();
        } else {
            tab = symtab_twotier_create(hot_depth, ram_budget);
            if (!enable_bloom) tab->set_param(tab, "bloom", 0);
        }

        semantic_context_t sem_ctx;
        semantic_init(&sem_ctx, tab, type_table);
        bool success = semantic_analyze(&sem_ctx, program);

        printf("\n=======================================================\n");
        printf(" COMPILATION RESULTS (%s)\n", tab->name);
        printf("=======================================================\n");
        printf(" Status: %s (Errors: %zu)\n", success ? "SUCCESS" : "FAILED", sem_ctx.error_count);

        symtab_stats_t stats;
        tab->get_stats(tab, &stats);
        printf(" Peak Symbol Table RAM: %zu bytes\n", stats.peak_bytes);
        printf(" Total Inserts:         %llu\n", (unsigned long long)stats.total_inserts);
        printf(" Total Lookups:         %llu (Hot hits: %llu, Cold hits: %llu, Misses: %llu)\n",
               (unsigned long long)stats.total_lookups,
               (unsigned long long)stats.hot_hits,
               (unsigned long long)stats.cold_hits,
               (unsigned long long)stats.misses);
        printf(" Bloom Filter Skips:    %llu (False Positives: %llu)\n",
               (unsigned long long)stats.bloom_skips,
               (unsigned long long)stats.bloom_false_positives);
        printf(" Decompressions:        %llu (Cache Hits: %llu)\n",
               (unsigned long long)stats.decompressions,
               (unsigned long long)stats.cache_hits);
        printf("=======================================================\n");

        tab->destroy(tab);
        ast_destroy_node(program);
        type_table_destroy(type_table);
        return 0;
    }

    char* source = read_entire_file(filename);
    if (!source) return 1;

    type_table_t* type_table = type_table_create();
    parser_t parser;
    parser_init(&parser, source, type_table);
    ast_node_t* program = parser_parse_program(&parser);

    if (parser.had_error) {
        fprintf(stderr, "Syntax error during parsing: %s\n", parser.error_msg);
        free(source);
        return 1;
    }

    symtab_t* tab = NULL;
    if (strcmp(symtab_type, "baseline") == 0) {
        tab = symtab_baseline_create();
    } else if (strcmp(symtab_type, "arena") == 0) {
        tab = symtab_ablation_create(SYMTAB_ARENA_ONLY);
    } else if (strcmp(symtab_type, "intern") == 0) {
        tab = symtab_ablation_create(SYMTAB_INTERN_ONLY);
    } else if (strcmp(symtab_type, "packed") == 0) {
        tab = symtab_ablation_create(SYMTAB_PACKED_ONLY);
    } else if (strcmp(symtab_type, "nobloom") == 0) {
        tab = symtab_ablation_create(SYMTAB_COMPRESSED_NO_BLOOM);
    } else {
        tab = symtab_twotier_create(hot_depth, ram_budget);
        if (!enable_bloom) tab->set_param(tab, "bloom", 0);
        if (min_scope_size > 0) tab->set_param(tab, "min_scope_size", min_scope_size);
    }

    semantic_context_t sem_ctx;
    semantic_init(&sem_ctx, tab, type_table);
    sem_ctx.dump_resolutions = dump_resolutions;
    bool success = semantic_analyze(&sem_ctx, program);

    if (dump_stats) {
        symtab_stats_t stats;
        tab->get_stats(tab, &stats);
        printf("\n[%s] Status: %s | Peak RAM: %zu bytes | Inserts: %llu | Lookups: %llu | Hot Hits: %llu | Cold Hits: %llu | Skips: %llu | Decomps: %llu\n",
               tab->name, success ? "OK" : "ERROR", stats.peak_bytes,
               (unsigned long long)stats.total_inserts,
               (unsigned long long)stats.total_lookups,
               (unsigned long long)stats.hot_hits,
               (unsigned long long)stats.cold_hits,
               (unsigned long long)stats.bloom_skips,
               (unsigned long long)stats.decompressions);
    }

    tab->destroy(tab);
    ast_destroy_node(program);
    type_table_destroy(type_table);
    free(source);

    return success ? 0 : 1;
}
