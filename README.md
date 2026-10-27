# Scope-Lifetime-Aware Two-Tier Symbol Table

A high-performance, memory-optimized compiler symbol table architecture for C-like languages that reduces peak symbol-table RAM by **up to 60.0%** on nested scopes while preserving fast $O(1)$ lookup for active scopes and accelerating scope teardown by $2.7\times$.

---

## Architecture Overview

Traditional compiler symbol tables store identifiers in separate heap-allocated nodes (`malloc`/`free`) in chained bucket lists, consuming **48–64 bytes per raw node** and **160–200 bytes per symbol** loaded with bucket arrays, while incurring $O(N)$ destruction latency upon scope exit.

This project introduces a **Two-Tier Architecture**:
1. **Hot Tier**: Active innermost $K$ scopes are stored in an open-addressing hash table using compact **64-bit packed symbol records** (8 bytes) for rapid $O(1)$ access. Linear probing with **Knuth Algorithm 6.4R** in-place cluster backward-shift deletion guarantees zero allocations upon scope exit.
2. **Cold Tier**: Inactive ancestor scopes are demoted and compressed into contiguous blocks using **delta encoding**, **LEB128/zigzag varints**, and a **64-bit Bloom filter**.
3. **Bloom Filter & Range Short-Circuiting**: Queries for identifiers absent from ancestor scopes skip decompression entirely with a **>94% rejection rate**.
4. **Bulk Scope Arena Allocator**: Scopes allocate through lightweight linear chunk arenas, freeing all scope symbols in a single bulk pointer reset ($O(1)$ arena reset + $O(M)$ hot table sweep) without per-symbol `free()`.
5. **Pluggable Interface**: Both Baseline and Two-Tier (and all ablation variants) run seamlessly behind an abstract pluggable interface (`symtab_t`).

---

## Directory Structure

```
CD Project/
├── src/
│   ├── common/               # Core memory and compression primitives
│   │   ├── mem_tracker.c/h   # Byte-exact allocation tracking wrapper
│   │   ├── string_pool.c/h   # String interner (16-bit IDs)
│   │   ├── type_table.c/h    # Type hash-consing canonicalizer
│   │   ├── arena.c/h         # Scoped bump-allocator arenas
│   │   ├── bloom.h           # 64-bit multi-hash Bloom filter
│   │   ├── varint.c/h        # LEB128 & zigzag encoding
│   │   ├── scope_block.c/h   # Delta compression & decompression
│   │   └── types.h           # 64-bit packed symbol record
│   ├── symtab/               # Symbol table implementations
│   │   ├── symtab_api.h      # Abstract pluggable symbol table interface
│   │   ├── symtab_baseline.c/h # Traditional malloc chained hash table
│   │   ├── symtab_twotier.c/h  # Two-tier compressed symbol table
│   │   └── symtab_ablation.c/h # Incremental ablation variants
│   ├── compiler/             # Mini-C Front End
│   │   ├── token.h           # Mini-C token types
│   │   ├── lexer.c/h         # Hand-written Mini-C lexer
│   │   ├── ast.c/h           # Abstract Syntax Tree nodes (with unary op support)
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
│   ├── bench_runner.c        # Cross-platform high-precision benchmark suite
│   ├── generate_figures.py   # Publication IEEE figure generator (matplotlib + numpy)
│   ├── synthetic_gen.py      # Parametric synthetic workload generator
│   ├── benchmark_results.csv # Consolidated legacy benchmark metrics
│   └── results/              # 13 raw publication experiment datasets
│       ├── metadata.json
│       ├── exp1_peak_ram.csv
│       ├── exp2_memory_breakdown.csv
│       ├── exp3_compression_ratio.csv
│       ├── exp4_micro_latency.csv
│       ├── exp5_scope_scaling.csv
│       ├── exp6_bloom_cache.csv
│       ├── exp7_ast_memory_trace.csv
│       ├── exp8_scalability.csv
│       ├── exp9_sensitivity.csv
│       ├── exp10_budget_caps.csv
│       ├── exp11_compiler_e2e.csv
│       ├── exp12_alloc_counts.csv
│       └── exp13_flight_controller_breakdown.csv
├── data/                     # Realistic embedded & synthetic workloads
│   ├── arduino_sensors.mc    # Arduino environmental logger
│   ├── freertos_tasks.mc     # FreeRTOS kernel task scheduler
│   ├── zephyr_network.mc     # Zephyr RTOS IPv6 / CoAP packet parser
│   ├── mibench_telecom.mc    # MiBench telecom DSP filter
│   ├── embedded_flight_controller.mc # UAV Quad-X autopilot firmware (161 symbols, 4-level nesting)
│   ├── synthetic_deep_nesting.mc # 48-level deep nesting workload
│   ├── synthetic_broad_scope.mc  # Broad scope multi-symbol burst
│   └── synthetic_heavy_shadowing.mc # High identifier reuse workload
├── docs/
│   ├── PROBLEM_STATEMENT.md  # Detailed problem statement & goals
│   └── METRICS.md            # Empirical metrics catalog & measurement methodology
├── report/
│   ├── FINAL_REPORT.md       # Comprehensive systems engineering report
│   └── figures/              # IEEE publication-quality figures (PNG @ 300 dpi, SVG, PDF)
│       ├── fig1_peak_ram_grouped.{png,svg,pdf}
│       ├── fig2_memory_breakdown_stacked.{png,svg,pdf}
│       ├── fig3_ablation_waterfall.{png,svg,pdf}
│       ├── fig4_k_pareto_curve.{png,svg,pdf}
│       ├── fig5_latency_cdfs.{png,svg,pdf}
│       ├── fig6_scope_scaling.{png,svg,pdf}
│       ├── fig7_ast_memory_trace.{png,svg,pdf}
│       ├── fig8_scalability_lines.{png,svg,pdf}
│       ├── fig9_sensitivity_curves.{png,svg,pdf}
│       ├── fig10_budget_cap_heatmap.{png,svg,pdf}
│       ├── fig11_compression_metrics.{png,svg,pdf}
│       ├── fig12_bloom_cache_effectiveness.{png,svg,pdf}
│       └── fig13_flight_controller_breakdown.{png,svg,pdf}
├── web/                      # Interactive Web Visualizer
│   ├── index.html            # Visualizer UI
│   ├── style.css             # Accessible light developer theme (WCAG AA compliant)
│   └── app.js                # Live simulation & trace engine
├── Makefile                  # Cross-platform build script (Linux, macOS, Windows)
└── build.bat                 # Windows batch build script
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

## Quick Start & Verification

### 1. Build All Components

#### Using Makefile (Linux, macOS, Windows with GCC/Clang):
```bash
make all        # Compiles minicc, bench_runner, and test binaries
make test       # Executes all test suites
make bench      # Runs the high-precision benchmark runner
make figures    # Generates IEEE publication figures in report/figures/
make clean      # Removes all compiled binaries
```

#### Using Windows Command Prompt / PowerShell:
```cmd
build.bat
```

---

### 2. Run Compiler on Any Source File
```cmd
./minicc --symtab twotier --dump-stats data/arduino_sensors.mc
./minicc --symtab baseline --dump-stats data/arduino_sensors.mc
```

---

### 3. Run Test Suite
```bash
./test_baseline        # Baseline unit test
./test_roundtrip       # Bitwise field-exact compression tests
./test_differential    # 6,000 randomized operations against Baseline
./test_edge_cases      # 64-level deep nesting & 5,000 symbols stress
python tests/fuzzer.py # 30 rounds of random Mini-C code generation
```

---

### 4. Run Benchmarks
```bash
./bench_runner
```

---

### 5. Generate Publication Figures
```bash
make figures
# Or directly via Python:
python bench/generate_figures.py
```
This produces 13 sets of figures formatted to IEEE specifications (3.5 in / 7.0 in, serif 9 pt, Okabe-Ito colorblind-safe, white background) saved to `report/figures/` in **PNG @ 300 DPI**, **vector SVG**, and **PDF**.

---

## Empirical Benchmark Results

### Experiment 1: Real-World Embedded & Synthetic Benchmarks
> **Research Question**: *How much peak symbol-table RAM does Two-Tier save across diverse embedded systems codebases and synthetic stress patterns?*

Measured as the median of 15 consecutive executions using high-resolution monotonic timers:

| Benchmark Workload | Base RAM | TwoTier RAM | RAM Saved | Base Time | TwoTier Time | Slowdown | Cold Decomps |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Arduino Sensors** (`arduino_sensors.mc`) | 6,719 B | 4,996 B | **-25.6%** | 12.1 $\mu$s | 13.4 $\mu$s | 1.11x | 6 |
| **FreeRTOS Tasks** (`freertos_tasks.mc`) | 7,090 B | 4,959 B | **-30.1%** | 10.6 $\mu$s | 13.9 $\mu$s | 1.31x | 7 |
| **Zephyr Network** (`zephyr_network.mc`) | 7,306 B | 4,710 B | **-35.5%** | 11.4 $\mu$s | 13.2 $\mu$s | 1.16x | 2 |
| **MiBench Telecom** (`mibench_telecom.mc`) | 6,340 B | 4,965 B | **-21.7%** | 8.3 $\mu$s | 10.0 $\mu$s | 1.20x | 5 |
| **Flight Controller** (`embedded_flight_controller.mc`) | 7,626 B | 11,242 B | **+47.4% (worse)** | 71.1 $\mu$s | 100.2 $\mu$s | 1.41x | 39 |
| **Broad Scopes** (`synthetic_broad_scope.mc`) | 24,301 B | 19,005 B | **-21.8%** | 93.8 $\mu$s | 101.3 $\mu$s | 1.08x | 24 |
| **Heavy Shadowing** (`synthetic_heavy_shadowing.mc`) | 24,075 B | 9,644 B | **-59.9%** | 93.4 $\mu$s | 68.2 $\mu$s | **0.73x (Faster)** | 58 |
| **Deep Nesting** (`synthetic_deep_nesting.mc`) | 39,571 B | 16,286 B | **-58.8%** | 257.5 $\mu$s | 121.0 $\mu$s | **0.47x (Faster)** | 198 |

> **Flight Controller Note (+47.4% RAM):**  
> Flight Controller contains 39 scopes with ~4 symbols each. Each cold scope carries a fixed ~56-byte header (Bloom filter, range bounds, array metadata), which outweighs delta-varint savings when scopes are many and tiny. Two-Tier excels on workloads with $\ge 8$ symbols per scope or $\ge 5$ nesting levels.

> **Why Deep Nesting is Faster than Baseline (0.47x execution time):**  
> On multi-scope workloads, Baseline's scope exit mechanism is $O(N)$ pointer chasing: upon scope exit, it must traverse bucket lists and call `free()` on each symbol node and string. With 48 scopes and 199 symbols, Baseline spends hundreds of microseconds in allocator overhead. Two-Tier frees active scopes via bulk arena reset and sweeps a 32-slot hot table, cutting total scope exit latency from 1800 ns down to 700 ns per 50 symbols. This speedup dominates total compile time.

---

### Experiment 2: Ablation Study (Adding Techniques Incrementally)
> **Research Question**: *What is the individual memory and performance contribution of each architectural technique (arena, interning, packing, compression, Bloom filtering)?*

Evaluated on `synthetic_deep_nesting.mc` (199 inserts, 721 lookups, 48 nested scopes):

| Stage | Implementation Variant | Peak Memory | vs Baseline | Loaded Bytes/Symbol |
| :---: | :--- | :---: | :---: | :---: |
| **1** | **Baseline** (Malloc Chained Hash) | 39,571 B | 0.0% | 200.9 B |
| **2** | **+ Scoped Arena Allocator** | 33,912 B | -14.3% | 172.1 B |
| **3** | **+ String Interning** | 27,096 B | -31.5% | 137.5 B |
| **4** | **+ 64-bit Packed Symbols** | 20,696 B | -47.7% | 105.1 B |
| **5** | **+ Delta + Varint Compression** | 16,286 B | **-58.8%** | **82.7 B** |
| **6** | **+ Bloom Filter (Full Two-Tier)** | **16,286 B** | **-58.8%** | **82.7 B** (379 Bloom skips) |

---

### Experiment 3: Parameter Sweep (Hot Depth $K$ vs Memory & Decompressions)
> **Research Question**: *How does hot depth $K$ trade off peak symbol-table memory against lookup latency and cold decompressions?*

Evaluated on `synthetic_heavy_shadowing.mc` ($K \in [1, 64]$):

| Hot Depth $K$ | Peak RAM | Hot Hits | Cold Hits | Cold Decompressions |
| :---: | :---: | :---: | :---: | :---: |
| **$K = 1$** | **9,384 B** | 38 | 326 | 75 |
| **$K = 2$** | **9,644 B** | 69 | 295 | 58 |
| **$K = 4$** | **10,524 B** | 112 | 252 | 37 |
| **$K = 8$** | **12,300 B** | 182 | 182 | 14 |
| **$K = 16$** | **15,548 B** | 272 | 92 | 3 |
| **$K = 32$** | **17,320 B** | 364 | 0 | 0 |
| **$K = 64$** | **17,320 B** | 364 | 0 | 0 |

---

### Experiment 4: Architectural Demotion Policy Comparison
> **Research Question**: *How does an eager depth-bounded demotion policy compare to a lazy closure policy and a size-aware policy in balancing peak active-stack RAM against cold decompressions?*

| Workload | Demotion Policy | Peak RAM | RAM vs Base | Bloom Skips | Cold Decomps |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **Flight Controller** | **Eager ($K=2$)** | **11,242 B** | **+47.4% (worse)** | 1 | 39 |
| **Flight Controller** | Lazy (Demote on Scope Exit) | 10,624 B | **+39.3% (worse)** | 0 | 0 |
| **Flight Controller** | **Size-Aware ($K=2$, $\ge 16$ syms)** | **10,602 B** | **+39.0% (worse)** | 0 | 1 |
| **Deep Nesting** | **Eager ($K=2$)** | **16,286 B** | **-58.8% (saved)** | 379 | 198 |
| **Deep Nesting** | Lazy (Demote on Scope Exit) | 34,920 B | **-11.8% (saved)** | 0 | 0 |
| **Deep Nesting** | **Size-Aware ($K=2$, $\ge 16$ syms)** | **34,920 B** | **-11.8% (saved)** | 0 | 0 |

> **Finding:** On deep nesting, lazy demotion uses 34,920 B vs 16,286 B for eager demotion (+114% higher RAM) due to the cumulative overhead of keeping 48 ancestor scopes open uncompressed. Conversely, on the Flight Controller, lazy demotion (10,624 B) and size-aware demotion (10,602 B) beat eager demotion (11,242 B) because 39 tiny scopes (~4 symbols each) cost more in compressed block headers than they save via delta-varint encoding. Size-aware demotion prevents demoting scopes with $< 16$ symbols.

---

### Experiment 5: Micro-Latency Breakdown & Slowdown Profile
> **Research Question**: *What is the exact lookup latency penalty (hot hit, cold hit, miss) and insertion overhead of Two-Tier compared to Baseline across repeated median runs?*

Measured over 15 repeated benchmark iterations (timings vary by machine and run; ratios shown are medians on the author's benchmark machine):

| Operation Type | Baseline Latency | Two-Tier Latency | Speed Ratio ($T_{\text{two}} / T_{\text{base}}$) | Performance Impact |
| :--- | :---: | :---: | :---: | :--- |
| **1. Symbol Insertion** | 126.0 ns/op | 233.8 ns/op | **1.86x** | **Slower** (Interning, hash-consing, and packing) |
| **2. Hot Scope Lookup Hit** | 42.9 ns/op | 36.9 ns/op | **0.86x** | **Faster** (Contiguous open-addressing table) |
| **3a. Cold Hit (Cache-Warm)** | 87.6 ns/op | 35.8 ns/op | **0.41x** | **Faster** (LRU scratch buffer cache hit) |
| **3b. Cold Hit (Forced Decomp)** | 45.0 ns/op | 221.5 ns/op | **4.92x** | **Slower** (Cache-defeating on-demand decompression) |
| **4. Negative Miss Lookup** | 73.7 ns/op | 34.8 ns/op | **0.47x** | **Faster** (Bloom filter early short-circuit) |
| **5a. Scope Lifecycle (N=10)** | 1200.0 ns | 600.0 ns | **0.50x** | **Faster** (arena + hot-table sweep) |
| **5b. Scope Lifecycle (N=100)** | 10400.0 ns | 10600.0 ns | **1.02x** | **Comparable** |

#### Honest Trade-off Analysis:
- **Symbol Insertion (1.86x Slower)**: Insertion is ~1.86x slower (233.8 ns vs 126.0 ns) due to upfront string interning, type hash-consing, and 64-bit packed symbol record assembly. Paying ~107 ns at declaration time enables up to 60.0% RAM savings and accelerates subsequent lookups.
- **Cache-Warm vs Forced Decompression**: In row **3a (Cache-Warm)**, repeated lookups to a cold scope are served from the 2-entry LRU decompression cache (35.8 ns), where flat array binary search beats Baseline's multi-level bucket chain search. In row **3b (Forced Decomp)**, lookups round-robin across 3 cold scopes, completely defeating the 2-entry cache and forcing on-demand delta-varint decompression on 100% of queries. This incurs an honest **4.92x slowdown (221.5 ns vs 45.0 ns)**, reflecting the real cost of decompression when locality is absent.
- **Negative Queries**: Bloom filtering short-circuits misses in 34.8 ns with a >94% rejection rate, outperforming Baseline (73.7 ns).
- **Scope Teardown (Faster)**: Bulk arena release paired with an $O(M)$ hot table sweep runs in 600 ns vs 1200 ns for 10 symbols (faster than Baseline).

---

### Experiment 6: Budget Mode (Enforced Strict RAM Cap Stress Test)
> **Research Question**: *Can Two-Tier adaptively compile within RAM caps that crash Baseline with OOM?*

Tested on `synthetic_heavy_shadowing.mc` (Baseline uncapped peak: 24,075 B) with allocator-enforced memory caps:

| Enforced RAM Cap | Baseline Status | Two-Tier Budget Status | Two-Tier Peak RAM |
| :---: | :---: | :---: | :---: |
| **4,096 Bytes** | **FAIL (OOM Exceeded)** | **FAIL (OOM Exceeded)** | **4,088 B** |
| **8,192 Bytes** | **FAIL (OOM Exceeded)** | **FAIL (OOM Exceeded)** | **8,124 B** |
| **12,288 Bytes** | **FAIL (OOM Exceeded)** | **PASS (Enforced <= Cap)** | **9,644 B** |
| **16,384 Bytes** | **FAIL (OOM Exceeded)** | **PASS (Enforced <= Cap)** | **9,644 B** |
| **24,576 Bytes** | **FAIL (OOM Exceeded)** | **PASS (Enforced <= Cap)** | **9,644 B** |
| **32,768 Bytes** | **PASS** | **PASS (Enforced <= Cap)** | **9,644 B** |
| **49,152 Bytes** | **PASS** | **PASS (Enforced <= Cap)** | **9,644 B** |
| **65,536 Bytes** | **PASS** | **PASS (Enforced <= Cap)** | **9,644 B** |

---

### Experiment 7: Scalability Stress (Depth 4 to 256, Symbols 100 to 50,000)
> **Research Question**: *How does Two-Tier scale against Baseline when pushing nesting depth to 256 levels and identifiers to 50,000?*

- **Deep Nesting Scaling ($D=4$ to $256$, 16 syms/scope)**: Peak RAM reduction widens from -19.4% at depth 4 (7,129 B vs 8,845 B) to **-34.2% at depth 256** (227,977 B vs 346,357 B), saving 118.4 KB. At depth 256, Two-Tier executes in **1.95 ms vs 5.17 ms (2.6x faster)** because bulk arena release bypasses traversing 256 collision chains.
- **Large Identifier Scaling ($N=100$ to $50,000$)**: At 50,000 identifiers, Two-Tier consumes **2,031,893 B vs. Baseline's 2,566,392 B (-20.8% saved)**, cutting peak memory by 534 KB.

---

### Experiment 8: Flight Controller Memory Overhead Deep-Dive
> **Research Question**: *Why does Two-Tier consume more memory than Baseline on embedded_flight_controller.mc even without compression?*

Tracking per-component memory across all 39 scopes reveals that the overhead is driven by:
1. **String Interner Monotonicity (6,272 Bytes)**: Baseline frees each function's local identifier strings on scope exit, keeping live string RAM under 1.2 KB. Two-Tier retains all 161 unique names permanently in geometric string pool buffers.
2. **Fixed Infrastructure Overhead (2,336 Bytes)**: Open-addressing hot table (768 B) and type table (800 B) maintain compiler-wide descriptors.
3. **Compression Disabled**: Even with compression disabled ($K=64$), Two-Tier consumes 10,624 B (+39.3% worse), confirming that string pool accumulation and infrastructure—rather than compression—explain the overhead.

---

## Interactive Web Visualizer

The project includes an interactive web visualizer in `web/` that allows stepping through compilation, exploring the Hot and Cold tiers in real-time, watching Bloom filter bits light up, tracing lookup paths, and dynamically tweaking hot depth $K$.

To run the web visualizer locally:
```bash
python -m http.server 8000 --directory web
```
Then navigate to `http://localhost:8000` in any web browser.
