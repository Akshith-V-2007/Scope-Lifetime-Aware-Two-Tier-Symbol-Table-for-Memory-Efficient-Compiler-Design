# Benchmark Workloads and Dataset Specification

This directory contains the 12 evaluation workloads used to empirically assess the **Scope-Lifetime-Aware Two-Tier Symbol Table** against the Textbook Baseline, Strong Baseline, and architectural ablations.

---

## 1. Workload Summary & Scoping Metrics

All metrics below are generated directly by `minicc --dump-stats` and verified in `bench/results/workload_metrics.csv`:

| Workload File | Category | Origin / Provenance | Lines of Code | Symbol Count | Scope Count | Max Lexical Depth |
| :--- | :--- | :--- | :---: | :---: | :---: | :---: |
| `arduino_sensors.mc` | Embedded (Micro) | Translated from Arduino Core `Wire.h` | 76 | 35 | 14 | 5 |
| `freertos_tasks.mc` | Embedded (Micro) | Translated from FreeRTOS `tasks.c` | 79 | 33 | 16 | 6 |
| `zephyr_network.mc` | Embedded (Micro) | Translated from Zephyr `net_pkt.c` | 67 | 31 | 12 | 6 |
| `mibench_telecom.mc` | Embedded (Micro) | Translated from MiBench CRC/CRC32 | 58 | 24 | 11 | 5 |
| `embedded_flight_controller.mc` | Embedded (Full) | UAV Quad-X Autopilot AHRS Firmware | 416 | 161 | 67 | 6 |
| `synthetic_broad_scope.mc` | Synthetic Stress | Broad lexical scope generator | 390 | 327 | 11 | 9 |
| `synthetic_heavy_shadowing.mc` | Synthetic Stress | Scope shadowing stress generator | 326 | 151 | 27 | 25 |
| `synthetic_deep_nesting.mc` | Synthetic Stress | Deep lexical hierarchy generator | 541 | 199 | 51 | 49 |
| `freertos_queue.mc` | Real Firmware | FreeRTOS v10.4.3 `queue.c` | 203 | 67 | 44 | 6 |
| `mibench_qsort.mc` | Real Benchmark | MiBench Automotive `qsort_small.c` | 173 | 63 | 35 | 6 |
| `mibench_bitcount.mc` | Real Benchmark | MiBench Telecomm `bitcnts.c` | 134 | 50 | 26 | 7 |
| `zephyr_synopsys.mc` | Real Firmware | Zephyr RTOS `subsys/kernel/sched.c`| 168 | 53 | 34 | 6 |

---

## 2. Methodology & Translation

Each `.mc` file is formatted for the Mini-C compiler front-end:
- Preprocessor directives (`#include`, `#define`, `#pragma`) have been stripped using `scripts/preprocess_real_c.py`.
- Exact identifier names, function signatures, pointer declarators (`int* p`), struct declarations, multidimensional arrays, control-flow statements (`for`, `while`, `if`/`else`), and nested lexical blocks are preserved without modification.

---

## 3. Synthetic Workload Generator (`bench/synthetic_gen.py`)

Synthetic stress workloads are generated programmatically using `bench/synthetic_gen.py` to isolate specific architectural axes:

```bash
# Generate a deep nesting workload (depth=48, 4 symbols/scope, 15% shadowing)
python bench/synthetic_gen.py --depth 48 --symbols-per-scope 4 --shadowing 0.15 --output data/synthetic_deep_nesting.mc

# Generate a broad scope workload (depth=4, 50 symbols/scope, 5% shadowing)
python bench/synthetic_gen.py --depth 4 --symbols-per-scope 50 --shadowing 0.05 --output data/synthetic_broad_scope.mc

# Generate a heavy shadowing stress test (depth=24, 6 symbols/scope, 75% shadowing)
python bench/synthetic_gen.py --depth 24 --symbols-per-scope 6 --shadowing 0.75 --output data/synthetic_heavy_shadowing.mc
```

Parameters:
- `--depth <N>`: Maximum lexical block nesting depth.
- `--symbols-per-scope <N>`: Declarations inserted at each scope level.
- `--shadowing <float>`: Probability $[0.0, 1.0]$ that an inner declaration shadows an ancestor symbol name.
- `--output <path>`: Destination path for the `.mc` source file.

---

## 4. Real C Preprocessor Script (`scripts/preprocess_real_c.py`)

Raw C sources (e.g. from MiBench, Zephyr, or FreeRTOS) are converted into Mini-C syntax using `scripts/preprocess_real_c.py`:

```bash
python scripts/preprocess_real_c.py path/to/source.c data/output.mc
```

The script:
1. Strips `#include` and `#define` preprocessor lines, recording stripped directives as comments.
2. Removes GCC-specific compiler attributes (`__attribute__`, `__declspec`, `volatile`, `register`).
3. Normalizes architecture-dependent typedefs (`uint32_t`, `size_t`, `BaseType_t`, `bool`) to primitive C equivalents.
4. Preserves pointer declarators, multidimensional array syntax, member dereferencing (`->`), and block scoping.
