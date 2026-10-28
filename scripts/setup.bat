@echo off
echo ====================================================================
echo  Setting Up Scope-Lifetime-Aware Two-Tier Symbol Table Compiler
echo ====================================================================

echo [1/3] Checking and installing Python dependencies...
python -m pip install -r requirements.txt
if %ERRORLEVEL% NEQ 0 (
    echo Warning: Pip install failed or partially completed. Continuing...
)

echo [2/3] Building compiler and test suite binaries...
call build.bat
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b %ERRORLEVEL%
)

echo [3/3] Building verified web visualizer dataset...
python bench/build_web_data.py

echo ====================================================================
echo  SETUP COMPLETE! All tools and binaries are ready.
echo ====================================================================
