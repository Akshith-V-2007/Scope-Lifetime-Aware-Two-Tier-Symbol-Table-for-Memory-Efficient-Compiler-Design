#!/usr/bin/env bash
set -e

echo "===================================================================="
echo " Executing Full 13-Experiment Publication Benchmark Suite"
echo "===================================================================="

./bench_runner

echo "Running statistical validation (bootstrap CI and Wilcoxon signed-rank)..."
python3 bench/stats_analysis.py

echo "Updating web visualizer data payload..."
python3 bench/build_web_data.py

echo "===================================================================="
echo " BENCHMARKS COMPLETE! Results saved to bench/results/*.csv"
echo "===================================================================="
