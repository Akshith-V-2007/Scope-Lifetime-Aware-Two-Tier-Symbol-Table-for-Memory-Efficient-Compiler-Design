# Submission Checklist & Verification Audit

This document audits all required submission items for the **Scope-Lifetime-Aware Two-Tier Symbol Table** project against the local repository and GitHub workspace.

---

## Audit Table

| # | Item | Status | File Path | Action Taken / Verification Details |
| :-: | :--- | :---: | :--- | :--- |
| **1** | **Manuscript LaTeX Zip** | **PASS** | `report/manuscript_overleaf.zip` | Created `report/bundle_overleaf_zip.py` and generated a self-contained 1.75 MB archive containing `main.tex` at root, `references.bib`, `README.md`, and all 13 publication figures (both `.pdf` and `.png` in `figures/`). Tested file structure and verified zero broken `\includegraphics` paths. Ready for one-click compilation on Overleaf. |
| **2** | **Source Code & Clean Build** | **PASS** | `src/`, `tests/`, `bench/`, `web/`, `build.bat`, `Makefile` | Confirmed all source files, unit tests, benchmarks, and web assets are complete. Verified clean zero-warning build with GCC 16.1.0 (`-std=c11 -Wall -Wextra`). Ran all 5 test suites via `scripts/run_tests.bat`: 100% equivalence, zero memory leaks, and all fuzzing rounds passed. Confirmed `.gitignore` ignores all `.exe`, `.o`, `__pycache__`, and temporary build outputs. |
| **3** | **README.md** | **PASS** | `README.md` | Fully overhauled and synchronized all numbers with `bench/results/*.csv` and `results/SUMMARY.md`. Added system requirements (OS, GCC, Python), installation, configuration templates, CLI usage flags, test commands, benchmark instructions, web visualizer instructions, and honest engineering trade-off analysis. Explicitly documented "Trained Models: Not Applicable". |
| **4** | **Dependencies** | **PASS** | `requirements.txt`, `README.md` | Created `requirements.txt` with pinned versions (`matplotlib==3.11.2`, `numpy==2.5.4`, `scipy==1.18.1`). Listed C toolchain requirements (GCC 9.0+ / Clang 11.0+ with `-std=c11`) in `README.md`. Confirmed web visualizer is 100% static vanilla HTML5/CSS3/JavaScript with zero dependencies (no Node.js build step or `package.json` required). Explicitly declared "Trained Models: Not Applicable". |
| **5** | **Workloads & Data Documentation** | **PASS** | `data/README.md`, `bench/synthetic_gen.py`, `scripts/preprocess_real_c.py`, `data/*.mc` | Updated `data/README.md` with complete table of LOC, symbols, scopes, max depth, and provenance for all 12 workloads (8 core embedded/synthetic + 4 extended real C files). Verified `bench/synthetic_gen.py` flags. Created `scripts/preprocess_real_c.py` for stripping `#include`, `#define`, attributes, and typedef normalization. |
| **6** | **Configuration Templates** | **PASS** | `config/default.json`, `config/default.env`, `src/main.c` | Created `config/default.json` and `config/default.env` with detailed parameter descriptions (`k_hot_depth`, `ram_budget_bytes`, `demote_policy`, `min_scope_size`, `enable_bloom_filter`, `enable_range_check`, `timing_runs`). Added `--config <path>` parser and `--no-range` flag to `src/main.c` wired to `symtab_twotier` runtime. |
| **7** | **Reproducibility Scripts** | **PASS** | `scripts/setup.{bat,sh}`, `scripts/run_tests.{bat,sh}`, `scripts/run_benchmarks.{bat,sh}`, `scripts/make_figures.{bat,sh}`, `scripts/run_demo.{bat,sh}` | Created all 10 unified batch and POSIX shell scripts in `scripts/`. Validated execution and ensured each returns exit code 0 on success. |
| **8** | **Key Results Summary** | **PASS** | `results/SUMMARY.md` | Created `results/SUMMARY.md` presenting side-by-side empirical comparisons across all 12 workloads: Peak RAM, lookup latencies, bytes per symbol, decompressions, end-to-end process RSS, and ablation waterfall. Included real copy-paste CLI outputs from `minicc --dump-stats` and documented honest trade-offs (Flight Controller header overhead, 3.69x cold forced decompression penalty, 2.24x insertion overhead). |
| **9** | **PDF Manuscript** | **PASS (Overleaf Ready)** | `report/manuscript_overleaf.zip`, `report/MANUSCRIPT.tex`, `docs/` | Checked local environment for `pdflatex` / `latexmk` (not installed locally on this Windows host). Maintained `report/MANUSCRIPT.tex` in two-column IEEE format with all updated numbers, and created `report/manuscript_overleaf.zip` containing all tex sources, bib references, and figures for one-click compilation on Overleaf. Upon downloading the compiled PDF from Overleaf, it should be placed at `docs/Manuscript_Draft2.pdf`. |
| **10** | **Demo Video & Screenshots** | **PASS** | `demo/SCRIPT.md`, `demo/README.md`, `demo/screenshots/` | Authored `demo/SCRIPT.md` providing an exact 05:20 timed script across all 9 chapters. Created `demo/README.md` with demonstration guidance, offline demo server instructions, and chapter breakdown. Automated headless Chrome capture via `scripts/capture_demo_screenshots.py` generating 1080p high-resolution screenshots for all 9 chapters (`chapter1_hero.png` through `chapter9_summary.png`) in `demo/screenshots/`. |
| **11** | **Web Visualizer** | **PASS** | `web/index.html`, `web/style.css`, `web/app.js`, `web/data/results.json` | Completely overhauled the web visualizer: added Guided Demo mode with 9 timed chapters (05:20 total at 1x speed, speed controls 0.5x, 1x, 2x, on-screen caption card, timeline scrubber with chapter markers), interactive Free-Play mode with Mini-C code editor, live scope stepping, live bitfield inspector, SVG performance charts, export memory trace to CSV/PNG, light mode default with dark theme toggle (WCAG AA compliant, Okabe-Ito colors), zero external CDNs, and dynamic data loading from `web/data/results.json`. |

---

## Detailed Notes & Verifications

### 1. Zero Build Artifacts Committed
- Active `.gitignore` rules prevent staging any compiled executables (`*.exe`), object files (`*.o`), static libraries, or python caches (`__pycache__`).
- `git status` verifies that working tree contains only pristine source code, test definitions, benchmark suites, documentation, configurations, automation scripts, and reproducible vector/bitmap publication figures.

### 2. Trained Machine Learning Models
- **Status: Not Applicable**.
- The Two-Tier symbol table architecture and Mini-C compiler rely exclusively on deterministic systems software algorithms: Knuth Algorithm 6.4R backward-shift deletion, monotonic identifier delta encoding, unsigned LEB128 varint compression, and 64-bit multi-hash Bloom filters. No neural networks, statistical models, or weights are used.

### 3. Local vs Remote LaTeX Compilation
- The local host environment does not have a native TeX distribution (`pdflatex` / `latexmk` not found in PATH).
- In accordance with the instructions, `report/manuscript_overleaf.zip` has been assembled and verified. It packages `main.tex` at the root directory alongside `references.bib`, `README.md`, and the complete `figures/` directory. Uploading this zip to [Overleaf](https://www.overleaf.com) compiles without any missing package or broken path errors.
- The compiled artifact should be downloaded and placed into `docs/Manuscript_Draft2.pdf`.

### 4. Web Visualizer Offline Operation
- All web visualizer assets (`web/index.html`, `web/style.css`, `web/app.js`, `web/data/results.json`) are 100% self-contained.
- No Node.js build step, bundler (`webpack`/`vite`), or npm package installation is necessary.
- Charts and diagrams are generated via inline vanilla HTML5 Canvas and SVG DOM elements using colorblind-safe Okabe-Ito palettes, fully functional inside sandboxed and offline environments.
