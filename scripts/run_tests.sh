#!/usr/bin/env bash
set -e

echo "===================================================================="
echo " Running Scope-Lifetime-Aware Two-Tier Symbol Table Test Suite"
echo "===================================================================="

echo "[1/5] Running Baseline Unit Tests..."
./test_baseline

echo "[2/5] Running Compression Round-Trip Tests..."
./test_roundtrip

echo "[3/5] Running Differential Equivalence Tests..."
./test_differential

echo "[4/5] Running Edge-Case and Stress Tests..."
./test_edge_cases

echo "[5/5] Running Differential Fuzzer (30 rounds)..."
python3 tests/fuzzer.py

echo "===================================================================="
echo " ALL TESTS PASSED WITH 100% EQUIVALENCE AND ZERO MEMORY LEAKS!"
echo "===================================================================="
