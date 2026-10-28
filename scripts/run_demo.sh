#!/usr/bin/env bash
set -e

echo "===================================================================="
echo " Launching Two-Tier Symbol Table Interactive Visualizer"
echo "===================================================================="

python3 bench/build_web_data.py >/dev/null 2>&1 || true

echo "Starting local HTTP server on port 8000..."
echo "Open your web browser at:"
echo "   http://localhost:8000/"
echo ""
echo "Press Ctrl+C in this terminal to terminate the server."
echo "===================================================================="

python3 -m http.server 8000 --directory web
