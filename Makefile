# Makefile for Scope-Lifetime-Aware Two-Tier Symbol Table & Mini-C Compiler
# Compatible with Linux, macOS, and Windows (MinGW / MSYS2 / Clang / GCC)

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2 -Isrc

COMMON_SRCS = src/common/mem_tracker.c \
              src/common/string_pool.c \
              src/common/type_table.c \
              src/common/arena.c \
              src/common/varint.c \
              src/common/scope_block.c

SYMTAB_SRCS = src/symtab/symtab_baseline.c \
              src/symtab/symtab_twotier.c \
              src/symtab/symtab_ablation.c

COMPILER_SRCS = src/compiler/lexer.c \
                src/compiler/ast.c \
                src/compiler/parser.c \
                src/compiler/semantic.c

ALL_SRCS = $(COMMON_SRCS) $(SYMTAB_SRCS) $(COMPILER_SRCS)

# OS Detection
ifeq ($(OS),Windows_NT)
    EXE = .exe
    RM = del /Q /F 2>nul || rm -f
    MKDIR = mkdir 2>nul || true
else
    EXE =
    RM = rm -f
    MKDIR = mkdir -p
endif

TARGETS = minicc$(EXE) \
          test_baseline$(EXE) \
          test_roundtrip$(EXE) \
          test_differential$(EXE) \
          test_edge_cases$(EXE) \
          bench_runner$(EXE)

.PHONY: all clean test bench figures asan

all: $(TARGETS)

minicc$(EXE): src/main.c $(ALL_SRCS)
	$(CC) $(CFLAGS) src/main.c $(ALL_SRCS) -o $@

test_baseline$(EXE): tests/test_baseline.c src/common/mem_tracker.c src/symtab/symtab_baseline.c
	$(CC) $(CFLAGS) tests/test_baseline.c src/common/mem_tracker.c src/symtab/symtab_baseline.c -o $@

test_roundtrip$(EXE): tests/test_roundtrip.c src/common/mem_tracker.c src/common/varint.c src/common/scope_block.c
	$(CC) $(CFLAGS) tests/test_roundtrip.c src/common/mem_tracker.c src/common/varint.c src/common/scope_block.c -o $@

test_differential$(EXE): tests/test_differential.c $(COMMON_SRCS) src/symtab/symtab_baseline.c src/symtab/symtab_twotier.c
	$(CC) $(CFLAGS) tests/test_differential.c $(COMMON_SRCS) src/symtab/symtab_baseline.c src/symtab/symtab_twotier.c -o $@

test_edge_cases$(EXE): tests/test_edge_cases.c $(COMMON_SRCS) src/symtab/symtab_baseline.c src/symtab/symtab_twotier.c
	$(CC) $(CFLAGS) tests/test_edge_cases.c $(COMMON_SRCS) src/symtab/symtab_baseline.c src/symtab/symtab_twotier.c -o $@

bench_runner$(EXE): bench/bench_runner.c $(ALL_SRCS)
	$(CC) $(CFLAGS) bench/bench_runner.c $(ALL_SRCS) -o $@

test: test_baseline$(EXE) test_roundtrip$(EXE) test_differential$(EXE) test_edge_cases$(EXE)
	@echo "--- Running Baseline Unit Tests ---"
	./test_baseline$(EXE)
	@echo "--- Running Compression Round-Trip Tests ---"
	./test_roundtrip$(EXE)
	@echo "--- Running Differential Equivalence Tests ---"
	./test_differential$(EXE)
	@echo "--- Running Stress and Edge-Case Tests ---"
	./test_edge_cases$(EXE)

bench: bench_runner$(EXE)
	./bench_runner$(EXE)

figures: bench
	python bench/generate_figures.py

asan: CFLAGS := -std=c11 -Wall -Wextra -g -fsanitize=address,undefined -Isrc
asan: clean $(TARGETS)
	@echo "--- Running ASan Baseline Unit Tests ---"
	./test_baseline$(EXE)
	@echo "--- Running ASan Compression Round-Trip Tests ---"
	./test_roundtrip$(EXE)
	@echo "--- Running ASan Differential Equivalence Tests ---"
	./test_differential$(EXE)
	@echo "--- Running ASan Stress and Edge-Case Tests ---"
	./test_edge_cases$(EXE)
	@echo "--- Running ASan Benchmark Runner ---"
	./bench_runner$(EXE)

clean:
	$(RM) $(TARGETS) *.o
