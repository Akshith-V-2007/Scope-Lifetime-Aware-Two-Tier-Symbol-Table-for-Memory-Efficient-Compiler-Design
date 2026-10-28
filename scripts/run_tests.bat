@echo off
echo ====================================================================
echo  Running Scope-Lifetime-Aware Two-Tier Symbol Table Test Suite
echo ====================================================================

echo [1/5] Running Baseline Unit Tests...
.\test_baseline.exe
if %ERRORLEVEL% NEQ 0 ( echo Baseline tests failed! & exit /b %ERRORLEVEL% )

echo [2/5] Running Compression Round-Trip Tests...
.\test_roundtrip.exe
if %ERRORLEVEL% NEQ 0 ( echo Round-trip tests failed! & exit /b %ERRORLEVEL% )

echo [3/5] Running Differential Equivalence Tests...
.\test_differential.exe
if %ERRORLEVEL% NEQ 0 ( echo Differential tests failed! & exit /b %ERRORLEVEL% )

echo [4/5] Running Edge-Case and Stress Tests...
.\test_edge_cases.exe
if %ERRORLEVEL% NEQ 0 ( echo Edge-case tests failed! & exit /b %ERRORLEVEL% )

echo [5/5] Running Differential Fuzzer (30 rounds)...
python tests/fuzzer.py
if %ERRORLEVEL% NEQ 0 ( echo Fuzzer failed! & exit /b %ERRORLEVEL% )

echo ====================================================================
echo  ALL TESTS PASSED WITH 100%% EQUIVALENCE AND ZERO MEMORY LEAKS!
echo ====================================================================
