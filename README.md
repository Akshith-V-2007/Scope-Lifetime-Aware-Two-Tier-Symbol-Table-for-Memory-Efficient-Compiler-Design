# Scope-Lifetime-Aware Two-Tier Symbol Table

A high-performance, memory-optimized compiler symbol table architecture for C-like languages that reduces peak symbol-table RAM by **up to 62.0%** on nested scopes while preserving fast $O(1)$ lookup for active scopes, accelerating negative lookups by $2.1\times$ via 64-bit multi-hash Bloom filters, and eliminating per-symbol `free()` overhead upon scope exit through bulk linear chunk arenas.

---

## Architecture Overview

Traditional compiler symbol tables store identifiers in separate heap-allocated nodes (`malloc`/`free`) in chained bucket lists, consuming **48–64 bytes per raw node** and **170–210 bytes per symbol** when accounting for bucket arrays and allocator overhead, while incurring $O(N)$ destruction latency upon scope exit.

This project introduces a **Two-Tier Architecture**:
1. **Hot Tier**: Active innermost $K$ scopes are stored in an open-addressing hash table using compact **64-bit packed symbol records** (8 bytes) for rapid $O(1)$ access. Linear probing with **Knuth Algorithm 6.4R** in-place cluster backward-shift deletion guarantees zero heap allocations upon scope exit.
2. **Cold Tier**: Inactive ancestor scopes are demoted and compressed into contiguous blocks using **monotonic ID sorting**, **delta encoding**, **LEB128/zigzag varints**, and a **64-bit Bloom filter**.
3. **Bloom Filter & Range Short-Circuiting**: Queries for identifiers absent from ancestor scopes skip decompression entirely with a **>97% rejection rate** using multi-hash Bloom filters and min/max Name ID bounds.
4. **Bulk Scope Arena Allocator**: Scopes allocate through lightweight linear chunk arenas, freeing all scope symbols in a single bulk pointer reset ($O(1)$ arena reset + $O(M)$ hot table sweep) without per-symbol `free()`.
5. **Pluggable Architecture**: Baseline, Strong Baseline, Two-Tier, and five ablation engines share an identical abstract interface (`symtab_t`).

---

## System Requirements

| Component | Minimum Requirement | Recommended / Verified Environment |
| :--- | :--- | :--- |
| **Operating System** | Windows 10/11 (x64), Linux (Ubuntu 20.04+, Debian, Fedora), or macOS (11.0+) | Windows 11 Pro 64-bit / Linux x86_64 |
| **C Compiler** | GCC 9.0+ or Clang 11.0+ (`-std=c11` support required) | MinGW-w64 GCC 16.1.0 / GCC 13.2.0 |
| **Python** | Python 3.10+ | Python 3.12 (with `pip`) |
| **Python Libraries** | `matplotlib>=3.8.0`, `numpy>=1.26.0`, `scipy>=1.12.0` | Listed in `requirements.txt` |
| **Web Browser** | Any modern evergreen browser (Chrome, Firefox, Edge, Safari) | Google Chrome 120+, Edge 120+ |
| **Trained Models** | **Not Applicable** | *No machine learning models are used; all algorithms are deterministic C code.* |

---

## Directory Structure

```
CD Project/
├── src/
│   ├── common/               # Core memory and compression primitives
│   │   ├── mem_tracker.c/h   # Byte-exact allocation tracking wrapper
│   │   ├── string_pool.c/h   # String interner (16-bit IDs, FNV-1a)
│   │   ├── type_table.c/h    # Type hash-consing canonicalizer
│   │   ├── arena.c/h         # Scoped bump-allocator chunk arenas
│   │   ├── bloom.h           # 64-bit multi-hash Bloom filter
│   │   ├── varint.c/h        # LEB128 & zigzag encoding
│   │   ├── scope_block.c/h   # Delta compression & decompression
│   │   └── types.h           # 64-bit packed symbol record
│   ├── symtab/               # Symbol table implementations
│   │   ├── symtab_api.h      # Abstract pluggable symbol table interface
│   │   ├── symtab_baseline.c/h # Traditional malloc chained hash table
│   │   ├── symtab_strong_baseline.c/h # Open-addressed, interned, packed baseline
│   │   ├── symtab_twotier.c/h  # Two-tier compressed symbol table
│   │   └── symtab_ablation.c/h # Incremental ablation variants
│   ├── compiler/             # Mini-C Front End
│   │   ├── token.h           # Mini-C token types
│   │   ├── lexer.c/h         # Hand-written Mini-C lexer
│   │   ├── ast.c/h           # Abstract Syntax Tree nodes
│   │   ├── parser.c/h        # Recursive descent parser
│   │   └── semantic.c/h      # Semantic analyzer & scope driver
│   └── main.c                # minicc CLI driver
├── tests/
│   ├── test_baseline.c       # Baseline unit tests (shadowing, redecl, scopes)
│   ├── test_roundtrip.c      # Bitwise compression/decompression tests
│   ├── test_differential.c   # Differential validation (6,000+ operations)
│   ├── test_edge_cases.c     # Empty scopes, 5k symbols, 64-level nesting
│   └── fuzzer.py             # Differential random program fuzzer
├── bench/
│   ├── bench_runner.c        # Cross-platform high-precision benchmark runner
│   ├── generate_figures.py   # Publication IEEE figure generator (PNG/SVG/PDF)
│   ├── synthetic_gen.py      # Parametric synthetic workload generator
│   ├── build_web_data.py     # CSV-to-JSON aggregator for web visualizer
│   └── results/              # 13 raw publication experiment datasets (CSV + JSON)
├── data/                     # 12 Embedded and synthetic Mini-C workloads
│   ├── arduino_sensors.mc    # Arduino environmental logger
│   ├── freertos_tasks.mc     # FreeRTOS kernel task scheduler
│   ├── zephyr_network.mc     # Zephyr RTOS IPv6 / CoAP packet parser
│   ├── mibench_telecom.mc    # MiBench telecom DSP filter
│   ├── embedded_flight_controller.mc # UAV Quad-X autopilot firmware
│   ├── synthetic_deep_nesting.mc # 48-level deep nesting workload
│   ├── synthetic_broad_scope.mc  # Broad scope multi-symbol burst
│   ├── synthetic_heavy_shadowing.mc # High identifier reuse workload
│   ├── freertos_queue.mc     # Real FreeRTOS v10.4.3 queue manager
│   ├── mibench_qsort.mc      # Real MiBench qsort_small workload
│   ├── mibench_bitcount.mc   # Real MiBench bitcnts bit manipulation workload
│   ├── zephyr_synopsys.mc    # Real Zephyr RTOS thread scheduler
│   └── README.md             # Workload metrics, LOC, symbols, provenance
├── config/                   # Configuration templates
│   ├── default.json          # JSON configuration template
│   └── default.env           # Shell environment configuration template
├── scripts/                  # Unified automation & reproducibility scripts
│   ├── setup.bat / .sh       # Environment setup & dependency installation
│   ├── run_tests.bat / .sh   # Clean build & all test suites execution
│   ├── run_benchmarks.bat / .sh # Run 15-iteration benchmark suite
│   ├── make_figures.bat / .sh# Rebuild IEEE publication figures
│   ├── run_demo.bat / .sh    # Launch web visualizer local server
│   ├── capture_demo_screenshots.py # Headless Chrome screenshot capturer
│   └── preprocess_real_c.py  # C preprocessor for real-world benchmark sources
├── report/
│   ├── MANUSCRIPT.tex        # IEEE Transactions LaTeX manuscript source
│   ├── references.bib        # BibTeX bibliography database
│   ├── manuscript_overleaf.zip # Self-contained bundle ready for Overleaf
│   └── figures/              # Publication figures in PNG (300 DPI), SVG, and PDF
├── results/
│   └── SUMMARY.md            # Verified empirical summary tables & CLI traces
├── demo/
│   ├── SCRIPT.md             # 05:20 video recording script (9 chapters)
│   ├── README.md             # Demonstration video guide & screenshots gallery
│   └── screenshots/          # 1080p automated captures for chapters 1-9
├── web/                      # Zero-dependency Interactive Web Visualizer
│   ├── index.html            # Light/Dark UI, timeline controller, bitfield inspector
│   ├── style.css             # WCAG AA compliant light theme + Okabe-Ito palette
│   ├── app.js                # Guided demo player (9 chapters) & live simulation
│   └── data/results.json     # Compiled empirical benchmarks for chart display
├── Makefile                  # Cross-platform Makefile (Linux, macOS, Windows)
├── build.bat                 # Windows batch build script
├── requirements.txt          # Pinned Python package dependencies
└── SUBMISSION_CHECKLIST.md   # Comprehensive submission verification audit
```

---

## 64-Bit Packed Symbol Record Bit Layout

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Name ID              |  Kind |Storage|  Scope Depth  |
|          (16 bits)            | (4 b) | (4 b) |    (8 bits)   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|          Type ID              |        Offset (Stack/Struct)  |
|          (16 bits)            |        (16 bits, signed)      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

---

## Quick Start & Installation

### 1. Automated Setup
Install Python dependencies and verify the C toolchain:
```bash
# On Windows:
scripts\setup.bat

# On Linux / macOS:
chmod +x scripts/*.sh
./scripts/setup.sh
```

Or manually:
```bash
pip install -r requirements.txt
```

---

### 2. Build All Components
```bash
# Using Makefile (Linux, macOS, Windows with GCC):
make all        # Compiles minicc, bench_runner, and test binaries
make test       # Executes all 5 test suites
make bench      # Runs the high-precision benchmark suite
make figures    # Generates IEEE publication figures in report/figures/
make clean      # Cleans compiled binaries

# Or using Windows Batch:
build.bat
```

---

### 3. Run the Mini-C Compiler (`minicc`)
Compile and analyze any Mini-C workload:
```bash
# Run with Two-Tier symbol table and dump detailed telemetry:
./minicc --symtab twotier --dump-stats data/arduino_sensors.mc

# Run with Baseline malloc-chained table:
./minicc --symtab baseline --dump-stats data/arduino_sensors.mc

# Run with Strong Baseline (open-addressed, linear probing):
./minicc --symtab strong_baseline --dump-stats data/arduino_sensors.mc

# Load parameters from configuration file:
./minicc --config config/default.json --dump-stats data/embedded_flight_controller.mc
```

#### Supported CLI Flags:
- `--symtab <name>`: `twotier` (default), `baseline`, `strong_baseline`, `arena`, `intern`, `packed`, `nombloom`.
- `--hot-depth <K>`: Number of active innermost scopes kept in hot tier (default: 2).
- `--no-bloom`: Disable Bloom filter lookup rejection.
- `--no-range`: Disable min/max Name ID range bounding.
- `--config <path>`: Load configuration parameters from JSON file (e.g., `config/default.json`).
- `--dump-stats`: Output symbol count, scopes, peak RAM, hot/cold hits, Bloom skips, and decompressions.
- `--dump-resolutions`: Print all identifier resolution bindings to stdout.

---

### 4. Run Test Suites
```bash
# On Windows:
scripts\run_tests.bat

# On Linux / macOS:
./scripts/run_tests.sh
```
Or execute individual test binaries directly:
- `./test_baseline`: Unit testing for lexical scoping, shadowing, and redeclaration errors.
- `./test_roundtrip`: Bitwise field-exact compression and decompression roundtrip verification.
- `./test_differential`: 6,000+ randomized operations comparing Two-Tier against Baseline for identical resolutions.
- `./test_edge_cases`: Empty scopes, 5,000 symbol bursts, and 64-level nesting limits.
- `python tests/fuzzer.py`: Automated randomized differential fuzzing.

---

### 5. Run Benchmarks
```bash
# On Windows:
scripts\run_benchmarks.bat

# On Linux / macOS:
./scripts/run_benchmarks.sh
```
Executes all 13 publication experiments over 15 runs per measurement with monotonic timers and writes raw CSV distributions to `bench/results/*.csv`.

---

### 6. Generate IEEE Publication Figures
```bash
# On Windows:
scripts\make_figures.bat

# On Linux / macOS:
./scripts/make_figures.sh
```
Generates all 13 figures formatted to IEEE specifications into `report/figures/` in **PNG @ 300 DPI**, **vector SVG**, and **PDF**.

---

### 7. Launch Interactive Web Visualizer
```bash
# On Windows:
scripts\run_demo.bat

# On Linux / macOS:
./scripts/run_demo.sh
```
Then open your browser at `http://localhost:8000`.

The visualizer includes:
- **Guided Demo Mode**: 9 automated chapters (05:20 total duration) walking through the motivation, 64-bit packed records, linear probing, delta-varint compression, Bloom filtering, live lookups, real workload comparison, the flight controller trade-off, and scalability.
- **Free-Play Mode**: Interactive Mini-C editor, live stepping through scope entries and exits, real-time memory breakdown chart, and interactive bitfield inspector.
- **Data Export**: Export memory traces to CSV and SVG charts.
- **Accessible Design**: Clean light mode default (WCAG AA compliant) with dark mode toggle and Okabe-Ito colorblind-safe color schemes.
- **Self-Contained**: 100% vanilla JavaScript, HTML5 Canvas, and SVG. Zero external CDN dependencies; operates entirely offline.

---

## Verified Empirical Results

All metrics below are medians of 15 consecutive executions on an AMD Ryzen 7 5825U host running 64-bit Windows with GCC 16.1.0 (`-std=c11 -O2`). Raw sample distributions are archived in `bench/results/*.csv`.

### 1. Peak Symbol-Table Memory Comparison (Experiment 1)

| Benchmark Workload | Symbols | Scopes | Max Depth | Baseline RAM | Strong Baseline | Two-Tier RAM | RAM Delta (vs Base) | Delta (vs Strong) | Slowdown (vs Base) | Cold Decomps |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Arduino Sensors** (`arduino_sensors.mc`) | 35 | 14 | 5 | 8,767 B | 5,208 B | 5,276 B | **-39.8% (saved)** | +1.3% | 1.23x | 6 |
| **FreeRTOS Tasks** (`freertos_tasks.mc`) | 33 | 16 | 6 | 9,138 B | 5,472 B | 5,239 B | **-42.7% (saved)** | -4.3% | 1.35x | 7 |
| **Zephyr Network** (`zephyr_network.mc`) | 31 | 12 | 6 | 9,354 B | 5,536 B | 4,990 B | **-46.7% (saved)** | -9.9% | 1.24x | 2 |
| **MiBench Telecom** (`mibench_telecom.mc`) | 24 | 11 | 5 | 8,388 B | 4,608 B | 4,645 B | **-44.6% (saved)** | +0.8% | 1.38x | 5 |
| **Flight Controller** (`embedded_flight_controller.mc`) | 161 | 67 | 6 | 9,674 B | 8,272 B | 9,450 B | **-2.3% (saved)** | +14.2% | 1.65x | 39 |
| **Broad Scopes** (`synthetic_broad_scope.mc`) | 327 | 11 | 9 | 26,349 B | 21,792 B | 19,285 B | **-26.8% (saved)** | -11.5% | 1.28x | 24 |
| **Heavy Shadowing** (`synthetic_heavy_shadowing.mc`) | 151 | 27 | 25 | 26,123 B | 16,720 B | 9,924 B | **-62.0% (saved)** | -40.6% | **0.53x (faster)** | 58 |
| **Deep Nesting** (`synthetic_deep_nesting.mc`) | 199 | 51 | 49 | 41,619 B | 33,544 B | 16,566 B | **-60.2% (saved)** | -50.6% | **0.52x (faster)** | 198 |

---

### 2. Extended Real-World Workloads & End-to-End Compiler RSS (Experiment 11)

Measured via OS Process Working Set (`GetProcessMemoryInfo` on Windows):

| Workload | Provenance | LOC | Symbols | Scopes | Process RSS (Base) | Process RSS (Two-Tier) | SymTab RAM Peak |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **freertos_queue.mc** | FreeRTOS Kernel v10.4.3 `queue.c` | 203 | 67 | 44 | 76,645 B | 73,181 B | 7,614 B |
| **mibench_qsort.mc** | MiBench Automotive `qsort_small.c` | 173 | 63 | 35 | 85,493 B | 82,023 B | 6,842 B |
| **mibench_bitcount.mc** | MiBench Telecom `bitcnts.c` | 134 | 50 | 26 | 60,003 B | 56,031 B | 5,980 B |
| **zephyr_synopsys.mc** | Zephyr RTOS `sched.c` | 168 | 53 | 34 | 69,003 B | 65,136 B | 6,324 B |
| **synthetic_depth_256.mc** | Asymptotic Scalability (Depth 256) | 1,024 | 512 | 256 | 1,585,773 B | 1,458,753 B | 222,473 B |

---

### 3. Micro-Latency Operation Breakdown (Experiment 4)

Measured on AMD Ryzen 7 5825U ($N=15$ runs per operation type):

| Operation Type | Baseline ($p50$) | Strong Baseline ($p50$) | Two-Tier ($p50$) | Two-Tier vs Baseline | Two-Tier vs StrongBase | Architectural Characteristic |
| :--- | :---: | :---: | :---: | :---: | :---: | :--- |
| **Insert** | 22.0 ns | 24.3 ns | 49.3 ns | 2.24x | 2.03x | Slower due to interning and open-address linear probing |
| **Hot Scope Lookup** | 44.5 ns | 36.8 ns | 38.6 ns | **0.87x (faster)** | 1.05x | Contiguous open-addressing table |
| **Outer Scope Lookup** | 68.2 ns | 51.2 ns | 64.1 ns | **0.94x (faster)** | 1.25x | Lookups in active ancestor scopes |
| **Cold Forced Decompress** | 64.9 ns | 48.6 ns | 239.5 ns | 3.69x (slower) | 4.93x | Decompression penalty when local cache misses |
| **Negative Miss Lookup** | 73.1 ns | 49.7 ns | 34.2 ns | **0.47x (2.1x faster)** | **0.69x (faster)** | Early rejection via Bloom filter and range check |

---

### 4. Architectural Ablation Breakdown (Deep Nesting)

Evaluated step-by-step on `synthetic_deep_nesting.mc`:

| Stage | Implementation Variant | Peak Memory | Delta vs Base | Bytes / Symbol | Architectural Contribution |
| :---: | :--- | :---: | :---: | :---: | :--- |
| **1** | **Baseline (Malloc-Chained)** | 41,619 B | 0.0% (ref) | 211.3 B | Individual node heap allocations + bucket pointers |
| **2** | **Strong Baseline (Open-Address)** | 33,544 B | -19.4% | 170.3 B | Linear probing table; eliminates bucket pointer overhead |
| **3** | **+ Scoped Chunk Arena** | 33,912 B | -18.5% | 172.1 B | Bulk bump-pointer allocation; $O(1)$ scope reclamation |
| **4** | **+ String Interning** | 27,376 B | -34.2% | 139.0 B | Deduplicates symbol names to 16-bit identifier IDs |
| **5** | **+ 64-bit Packed Symbols** | 20,976 B | -49.6% | 106.5 B | Bitfield packing: name, kind, storage, depth, type, offset |
| **6** | **+ Delta / Varint Compression** | 16,566 B | -60.2% | 84.1 B | Monotonic sort + delta encoding + LEB128 varints |
| **7** | **+ Bloom Filter (Full Two-Tier)** | **16,566 B** | **-60.2%** | **84.1 B** | 64-bit Bloom filter short-circuits negative queries |

---

## Honest Engineering Trade-offs & Limitations

1. **Insertion Overhead (2.24x Slower)**: Upfront string interning (FNV-1a), type table canonicalization, and 64-bit bitfield packing increase insertion latency from 22.0 ns to 49.3 ns. Paying this small overhead at declaration time enables up to 62.0% RAM reduction and accelerates subsequent lookups.
2. **Cold Decompression Penalty (3.69x Slower)**: When a lookup misses in the 2-entry LRU scratch decompression cache and hits a cold compressed scope, decompressing the varint stream costs ~239.5 ns. In typical compilation workflows, outer-scope lookups exhibit high temporal locality, and absent symbols are short-circuited by the Bloom filter (>97% rejection rate).
3. **Flight Controller Counterexample**: In programs with many tiny scopes (e.g. 67 scopes holding ~2.4 symbols each), eager depth-bounded demotion ($K=2$) compresses 39 small blocks whose metadata overhead (compressed block headers + Bloom filter) exceeds the varint delta savings. Enabling `DEMOTE_POLICY_SIZE_AWARE` (demoting only when a scope holds $\ge 16$ symbols) avoids this penalty, cutting peak RAM to 8,810 B (-8.9% vs Baseline).
4. **Scope Teardown Advantage**: In deep nesting workloads, Two-Tier is **$0.52\times$ faster in total compile time** (121.0 $\mu$s vs 257.5 $\mu$s) because bulk chunk arena reclamation avoids the hundreds of pointer traversals and individual `free()` invocations required by Baseline.

---

## Configuration Reference (`config/default.json`)

The compiler behavior can be configured via `config/default.json` or CLI flags:

```json
{
  "symtab_engine": "twotier",
  "k_hot_depth": 2,
  "ram_budget_bytes": 0,
  "demote_policy": "eager",
  "min_scope_size": 16,
  "enable_bloom_filter": true,
  "enable_range_check": true,
  "timing_runs": 15
}
```

- `k_hot_depth`: Number of active scopes maintained in the hot open-addressing table.
- `ram_budget_bytes`: Strict RAM budget cap (0 = unlimited; triggers budget-mode demotion if exceeded).
- `demote_policy`: Demotion strategy (`"eager"`, `"lazy"`, or `"size_aware"`).
- `min_scope_size`: Minimum symbol count required to compress a scope under `size_aware` policy.
- `enable_bloom_filter`: Enables 64-bit Bloom filter rejection on cold scopes.
- `enable_range_check`: Enables min/max Name ID range check prior to decompression.
- `timing_runs`: Repetition count for benchmark statistical evaluation (median + IQR).

---

## Overleaf Manuscript Compilation

The full IEEE Transactions manuscript is maintained in `report/MANUSCRIPT.tex`.

A pre-packaged, self-contained Overleaf bundle is available at `report/manuscript_overleaf.zip`.
To compile:
1. Upload `report/manuscript_overleaf.zip` directly to [Overleaf](https://www.overleaf.com) via **New Project -> Upload Project**.
2. Select **pdfLaTeX** or **LaTeXmk** compiler.
3. Overleaf will build `main.pdf` with all embedded figures and references.
4. If compiling locally with LaTeX installed, run `pdflatex main.tex && bibtex main && pdflatex main.tex && pdflatex main.tex`.
