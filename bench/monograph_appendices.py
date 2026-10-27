# monograph_appendices.py
# Generates Appendices A through E of the comprehensive monograph

def get_appendices():
    return """## Appendix A: Complete Mini-C EBNF Formal Grammar

```ebnf
Program        ::= { StructDecl | FuncDecl | VarDecl } ;

StructDecl     ::= "struct" Identifier "{" { VarDecl } "}" ";" ;

FuncDecl       ::= TypeSpec Identifier "(" [ ParamList ] ")" CompoundStmt ;

ParamList      ::= Param { "," Param } ;
Param          ::= TypeSpec Identifier ;

TypeSpec       ::= "int" 
                 | "float" 
                 | "char" 
                 | "void" 
                 | "struct" Identifier ;

CompoundStmt   ::= "{" { Statement } "}" ;

Statement      ::= CompoundStmt
                 | VarDecl
                 | AssignStmt
                 | IfStmt
                 | WhileStmt
                 | ReturnStmt
                 | ExprStmt ;

VarDecl        ::= TypeSpec Identifier [ "[" IntegerLiteral "]" ] [ "=" Expr ] ";" ;

AssignStmt     ::= Identifier [ "[" Expr "]" ] "=" Expr ";" ;

IfStmt         ::= "if" "(" Expr ")" Statement [ "else" Statement ] ;

WhileStmt      ::= "while" "(" Expr ")" Statement ;

ReturnStmt     ::= "return" [ Expr ] ";" ;

ExprStmt       ::= Expr ";" ;

Expr           ::= RelExpr ;
RelExpr        ::= AddExpr { ( "==" | "!=" | "<" | "<=" | ">" | ">=" ) AddExpr } ;
AddExpr        ::= MulExpr { ( "+" | "-" ) MulExpr } ;
MulExpr        ::= UnaryExpr { ( "*" | "/" ) UnaryExpr } ;
UnaryExpr      ::= "-" UnaryExpr | PrimaryExpr ;
PrimaryExpr    ::= IntegerLiteral 
                 | FloatLiteral 
                 | CharLiteral 
                 | Identifier [ "[" Expr "]" ] 
                 | "(" Expr ")" ;
```

---

## Appendix B: Complete C Header Interface Declarations

### B.1 Pluggable Symbol Table Interface (`src/symtab/symtab_api.h`)

```c
#ifndef SYMTAB_API_H
#define SYMTAB_API_H

#include "../common/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct symtab_stats {
    size_t total_inserts;
    size_t total_lookups;
    size_t hot_hits;
    size_t cold_hits;
    size_t misses;
    size_t decompressions;
    size_t bloom_skips;
    size_t range_skips;
    size_t peak_memory_bytes;
    size_t current_memory_bytes;
} symtab_stats_t;

typedef struct symtab_s symtab_t;

struct symtab_s {
    void* impl;
    void (*enter_scope)(symtab_t* self);
    void (*exit_scope)(symtab_t* self);
    bool (*insert)(symtab_t* self, const char* name, type_id_t type_id, 
                   symbol_kind_t kind, int32_t offset);
    const symbol_record_t* (*lookup)(symtab_t* self, const char* name);
    const symbol_record_t* (*lookup_current_scope)(symtab_t* self, const char* name);
    int (*get_scope_depth)(symtab_t* self);
    void (*get_stats)(symtab_t* self, symtab_stats_t* stats);
    void (*set_param)(symtab_t* self, const char* key, int value);
    void (*destroy)(symtab_t* self);
};

symtab_t* symtab_baseline_create(void);
symtab_t* symtab_twotier_create(int hot_depth);
symtab_t* symtab_ablation_create(const char* variant, int hot_depth);

#ifdef __cplusplus
}
#endif

#endif /* SYMTAB_API_H */
```

### B.2 Core Bitfield Types and Records (`src/common/types.h`)

```c
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t name_id_t;
typedef uint16_t type_id_t;
typedef uint16_t scope_id_t;

#define NAME_ID_INVALID 0
#define TYPE_ID_INVALID 0

typedef enum {
    KIND_VAR = 0,
    KIND_PARAM,
    KIND_FUNC,
    KIND_STRUCT,
    KIND_FIELD,
    KIND_ARRAY,
    KIND_TYPEDEF
} symbol_kind_t;

typedef enum {
    STORAGE_AUTO = 0,
    STORAGE_STATIC,
    STORAGE_EXTERN,
    STORAGE_GLOBAL
} storage_class_t;

typedef enum {
    TYPE_VOID = 0,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_POINTER,
    TYPE_ARRAY_T,
    TYPE_STRUCT_T,
    TYPE_FUNC_T
} type_kind_t;

#pragma pack(push, 1)
typedef struct {
    uint16_t name_id;
    uint8_t  kind_and_storage;
    uint8_t  scope_depth;
    uint16_t type_id;
    int16_t  offset;
} packed_symbol_t;
#pragma pack(pop)

typedef struct symbol_record {
    const char*      name;
    name_id_t        name_id;
    symbol_kind_t    kind;
    storage_class_t  storage;
    type_id_t        type_id;
    int32_t          offset;
    uint32_t         scope_depth;
    scope_id_t       scope_id;
    struct symbol_record* next;
} symbol_record_t;

static inline packed_symbol_t pack_symbol(const symbol_record_t* rec) {
    packed_symbol_t p;
    p.name_id = rec->name_id;
    p.kind_and_storage = (uint8_t)(((rec->storage & 0x0F) << 4) | (rec->kind & 0x0F));
    p.scope_depth = (uint8_t)(rec->scope_depth & 0xFF);
    p.type_id = rec->type_id;
    p.offset = (int16_t)(rec->offset & 0xFFFF);
    return p;
}

static inline void unpack_symbol(const packed_symbol_t* p, symbol_record_t* rec, const char* name) {
    rec->name = name;
    rec->name_id = p->name_id;
    rec->kind = (symbol_kind_t)(p->kind_and_storage & 0x0F);
    rec->storage = (storage_class_t)((p->kind_and_storage >> 4) & 0x0F);
    rec->scope_depth = p->scope_depth;
    rec->type_id = p->type_id;
    rec->offset = p->offset;
    rec->next = NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* TYPES_H */
```

---

## Appendix C: Comprehensive Benchmark Workload Characterization Table

The complete benchmark evaluation suite comprises 8 workloads spanning diverse embedded firmware paradigms and synthetic stress patterns:

| Workload File | Real / Synthetic | Domain / Application | Source LOC | AST Nodes | Lexical Scopes | Max Nesting Depth | Symbol Decl | Lookups Tested |
| :--- | :---: | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **`arduino_sensors.mc`** | Real | Embedded Sensor Bus Driver | 76 | 134 | 8 | 3 | 35 | 38 |
| **`freertos_tasks.mc`** | Real | RTOS Task Dispatcher | 79 | 142 | 9 | 4 | 33 | 33 |
| **`zephyr_network.mc`** | Real | Zephyr IP Packet Filtering | 67 | 120 | 7 | 3 | 31 | 28 |
| **`mibench_telecom.mc`** | Real | MiBench DSP Audio Filter | 58 | 108 | 6 | 3 | 24 | 30 |
| **`embedded_flight_controller.mc`**| Real | UAV Quadcopter Autopilot | 416 | 782 | 39 | 5 | 161 | 303 |
| **`synthetic_broad_scope.mc`** | Synthetic | Wide Flat Local Blocks | 390 | 650 | 12 | 2 | 327 | 124 |
| **`synthetic_heavy_shadowing.mc`**| Synthetic | Multi-Level Shadowing | 326 | 580 | 18 | 6 | 151 | 364 |
| **`synthetic_deep_nesting.mc`** | Synthetic | Extreme Lexical Recursion | 541 | 1,210 | 48 | 48 | 199 | 721 |

---

## Appendix D: Command-Line Options & Flags Reference Manual

### D.1 Mini-C Compiler Driver (`minicc`)
```
Usage: minicc [OPTIONS] <source_file.mc>

Options:
  --symtab <engine>        Select symbol table backend:
                           baseline : Traditional per-scope chained hash table
                           arena    : Bump-allocated chunk arena backend
                           intern   : Canonical string interning backend
                           packed   : 64-bit packed bitfield records
                           nombloom : Two-tier compression without Bloom filters
                           twotier  : Full Two-Tier architecture (Default)
  --hot-depth <K>          Number of innermost scopes kept hot (Default: 2)
  --no-bloom               Disable 64-bit Bloom filter short-circuit pruning
  --dump-stats             Emit detailed runtime telemetry and memory breakdown
  --dump-resolutions       Emit canonical textual trace of every symbol query
  --ram-budget <bytes>     Enforce strict hardware memory cap (OOM testing)
  -o <output_file>         Specify compiled output file path
  -h, --help               Display command-line syntax and option reference
```

### D.2 Automated Empirical Benchmark Runner (`bench_runner`)
```
Usage: bench_runner [OPTIONS]

Options:
  --all                    Execute all 13 empirical experiments sequentially
  --exp <id>               Execute specific experiment ID (1 to 13)
  --runs <N>               Set number of repeated timing runs (Default: 15)
  --csv                    Export raw per-run results to bench/results/*.csv
```

---

## Appendix E: Build, Testing, Benchmarking, and Differential Fuzzing Reproduction Manual

### E.1 Compilation and Build Targets
To compile the complete compiler and test suite on Windows (MinGW-w64 GCC) or POSIX (Linux/macOS):

```bash
# Clean previous build artifacts
make clean

# Compile minicc, bench_runner, and all 4 test binaries
make

# Execute full automated test suite
make test
```

### E.2 Executing Differential Verification and AST Fuzzing
```bash
# Run unit tests and boundary test batteries
./tests/test_baseline
./tests/test_roundtrip
./tests/test_differential
./tests/test_edge_cases

# Run 30-round grammar-directed differential AST fuzzer
python tests/fuzzer.py
```

### E.3 Regenerating Empirical Datasets and Publication Figures
```bash
# Execute 15-run empirical benchmark suite and emit CSVs
./bench/bench_runner

# Generate all 13 IEEE publication-grade figures (PNG, SVG, PDF)
python bench/generate_figures.py
```

All 13 figures are automatically updated in `report/figures/` at 300 DPI ready for publication.
"""

if __name__ == '__main__':
    print("Appendices generator loaded.")
