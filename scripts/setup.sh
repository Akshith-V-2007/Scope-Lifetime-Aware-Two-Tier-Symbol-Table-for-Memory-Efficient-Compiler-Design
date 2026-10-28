#!/usr/bin/env bash
set -e

echo "===================================================================="
echo " Setting Up Scope-Lifetime-Aware Two-Tier Symbol Table Compiler"
echo "===================================================================="

echo "[1/3] Checking and installing Python dependencies..."
python3 -m pip install -r requirements.txt || true

echo "[2/3] Building compiler and test suite binaries with make..."
make clean
make all

echo "[3/3] Building verified web visualizer dataset..."
python3 bench/build_web_data.py

echo "===================================================================="
echo " SETUP COMPLETE! All tools and binaries are ready."
echo "===================================================================="
