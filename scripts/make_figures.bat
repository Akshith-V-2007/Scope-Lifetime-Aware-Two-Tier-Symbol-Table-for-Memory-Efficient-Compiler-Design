@echo off
echo ====================================================================
echo  Generating IEEE Publication Figures in report/figures/
echo ====================================================================

python bench/generate_figures.py
if %ERRORLEVEL% NEQ 0 ( echo Figure generation failed! & exit /b %ERRORLEVEL% )

echo ====================================================================
echo  FIGURES COMPLETE! Generated PNG, SVG, and PDF in report/figures/
echo ====================================================================
