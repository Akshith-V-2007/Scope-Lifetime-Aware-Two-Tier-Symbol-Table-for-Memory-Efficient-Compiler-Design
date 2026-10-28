#!/usr/bin/env bash
set -e

echo "===================================================================="
echo " Generating IEEE Publication Figures in report/figures/"
echo "===================================================================="

python3 bench/generate_figures.py

echo "===================================================================="
echo " FIGURES COMPLETE! Generated PNG, SVG, and PDF in report/figures/"
echo "===================================================================="
