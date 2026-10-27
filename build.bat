@echo off
echo ====================================================================
echo  Building Scope-Lifetime-Aware Two-Tier Symbol Table and Compiler
echo ====================================================================

echo [1/5] Compiling minicc compiler executable...
gcc -O2 -std=c11 -Wall -Wextra -Isrc src/main.c src/common/mem_tracker.c src/common/string_pool.c src/common/type_table.c src/common/arena.c src/common/varint.c src/common/scope_block.c src/symtab/symtab_baseline.c src/symtab/symtab_twotier.c src/symtab/symtab_ablation.c src/compiler/lexer.c src/compiler/ast.c src/compiler/parser.c src/compiler/semantic.c -o minicc.exe
if %ERRORLEVEL% NEQ 0 ( echo Failed to build minicc.exe & exit /b %ERRORLEVEL% )

echo [2/5] Compiling baseline unit tests...
gcc -std=c11 -Wall -Wextra -Isrc tests/test_baseline.c src/common/mem_tracker.c src/symtab/symtab_baseline.c -o test_baseline.exe

echo [3/5] Compiling compression round-trip tests...
gcc -std=c11 -Wall -Wextra -Isrc tests/test_roundtrip.c src/common/mem_tracker.c src/common/varint.c src/common/scope_block.c -o test_roundtrip.exe

echo [4/5] Compiling differential and edge case verification tests...
gcc -std=c11 -Wall -Wextra -Isrc tests/test_differential.c src/common/mem_tracker.c src/common/string_pool.c src/common/type_table.c src/common/arena.c src/common/varint.c src/common/scope_block.c src/symtab/symtab_baseline.c src/symtab/symtab_twotier.c -o test_differential.exe
gcc -std=c11 -Wall -Wextra -Isrc tests/test_edge_cases.c src/common/mem_tracker.c src/common/string_pool.c src/common/type_table.c src/common/arena.c src/common/varint.c src/common/scope_block.c src/symtab/symtab_baseline.c src/symtab/symtab_twotier.c -o test_edge_cases.exe

echo [5/5] Compiling high-precision benchmark runner...
gcc -O2 -std=c11 -Wall -Wextra -Isrc bench/bench_runner.c src/common/mem_tracker.c src/common/string_pool.c src/common/type_table.c src/common/arena.c src/common/varint.c src/common/scope_block.c src/symtab/symtab_baseline.c src/symtab/symtab_twotier.c src/symtab/symtab_ablation.c src/compiler/lexer.c src/compiler/ast.c src/compiler/parser.c src/compiler/semantic.c -o bench_runner.exe

echo ====================================================================
echo  BUILD COMPLETE! All binaries compiled successfully.
echo ====================================================================
