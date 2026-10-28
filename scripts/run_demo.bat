@echo off
echo ====================================================================
echo  Launching Two-Tier Symbol Table Interactive Visualizer
echo ====================================================================

python bench/build_web_data.py >nul 2>&1

echo Starting local HTTP server on port 8000...
echo Open your web browser at:
echo    http://localhost:8000/
echo.
echo Press Ctrl+C in this console to terminate the server.
echo ====================================================================

python -m http.server 8000 --directory web
