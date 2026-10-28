@echo off
echo ====================================================================
echo  Executing Full 13-Experiment Publication Benchmark Suite
echo ====================================================================

.\bench_runner.exe
if %ERRORLEVEL% NEQ 0 ( echo Benchmark runner failed! & exit /b %ERRORLEVEL% )

echo Running statistical validation (bootstrap CI and Wilcoxon signed-rank)...
python bench/stats_analysis.py

echo Updating web visualizer data payload...
python bench/build_web_data.py

echo ====================================================================
echo  BENCHMARKS COMPLETE! Results saved to bench/results/*.csv
echo ====================================================================
