# Summary of Changes and Value Audit

This document records all modifications made to "CD Project", documenting every changed file, feature addition, bug fix, and empirical value update.

---

## 1. Files Modified and Added

| File | Change Type | Description |
| :--- | :---: | :--- |
| `bench/bench_runner.c` | **Fix / Feature** | Fixed heap buffer overflow in `generate_synthetic_code`, added bounds-checked `safe_append()`, implemented runtime detection for `metadata.json`, extended Experiment 8 depth sweep to 256, added Experiment 13 flight controller breakdown. |
| `bench/generate_figures.py` | **Feature / Polish** | Updated `fig1` (legend placed outside/below axes, plain tick labels in KB on log scale), updated `fig8` for depth 256 and removed internal titles, added `plot_fig13_flight_controller()` for `fig13_flight_controller_breakdown`. |
| `Makefile` | **Feature** | Added `asan` target compiling with `-fsanitize=address,undefined` and executing all test suites and `bench_runner`. |
| `docs/METRICS.md` | **Documentation** | Extended depth range specification to 256, added Section 3 detailing flight controller memory anomaly attribution (identifying string interner monotonicity and global infrastructure). |
| `report/FINAL_REPORT.md` | **Documentation** | Updated Section 9.3 with depth 256 scalability data, added Section 9.7 with flight controller breakdown analysis and `fig13` figure embed. |
| `README.md` | **Documentation** | Updated directory tree with 13 CSVs and 13 figure sets, added Experiment 7 (Depth 256) and Experiment 8 (Flight Controller overhead) sections. |
| `bench/results/metadata.json` | **Data** | Generated via dynamic runtime detection (real UTC timestamp, CPUID brand string, OS, logical cores, total RAM, compiler, flags). |
| `bench/results/exp8_scalability.csv` | **Data** | Extended with measured runs up to depth 256 and 50,000 symbols. |
| `bench/results/exp13_flight_controller_breakdown.csv` | **New Data** | Recorded per-component bytes across scopes 1 to 39 for Baseline, Two-Tier ($K=2$), and Two-Tier uncompressed ($K=64$). |
| `report/figures/fig1_peak_ram_grouped.*` | **Figure** | Regenerated with legend outside/below axes and plain log-scale tick labels in KB (2, 5, 10, 20, 40, 60). |
| `report/figures/fig8_scalability_lines.*` | **Figure** | Regenerated with depth range 4 to 256 and no internal plot titles. |
| `report/figures/fig13_flight_controller_breakdown.*` | **New Figure** | Generated in PNG @ 300 DPI, vector SVG, and vector PDF showing memory trajectory and stacked component breakdown on flight controller. |
| `CHANGES.md` | **New** | This file. |

---

## 2. Bug Fixes

### 2.1 Heap Buffer Overflow in `bench/bench_runner.c` (`generate_synthetic_code`)
- **Old Buffer Allocation**:
  ```c
  size_t est_size = (size_t)(depth * symbols_per_scope * 45 + 1024);
  ```
  *Issue*: Did not account for the $O(\text{depth}^2)$ growth of indentation spaces (4 spaces per depth level), causing buffer overflows and memory corruption on deep programs ($\ge 64$ levels).
- **New Buffer Allocation**:
  ```c
  size_t est_size = (size_t)depth * (size_t)(symbols_per_scope + 3) * (size_t)(64 + 4 * (depth + 2)) + 8192;
  ```
- **Bounds Checking**: Replaced unchecked `snprintf` calls with `safe_append()`, validating `offset < est_size` and verifying `vsnprintf` return codes prior to advancing the write pointer, terminating with a descriptive error upon truncation or overflow.

---

## 3. Metadata Detection Updates (`bench/results/metadata.json`)

| Field | Old Hardcoded Value | New Runtime Detected Value | Detection Mechanism |
| :--- | :---: | :---: | :--- |
| `timestamp` | `"2026-10-26T17:35:00Z"` | `"2026-10-26T12:56:15Z"` | `time() + gmtime() + strftime("%Y-%m-%dT%H:%M:%SZ")` |
| `os` | `"Windows NT"` | `"Windows (NT kernel x86_64)"` | Dynamic OS detection macros |
| `cpu_model` | `"AMD64 Family 25..."` | `"AMD Ryzen 7 5825U with Radeon Graphics"` | Hardware registers via `__cpuid` brand query functions `0x80000002`–`0x80000004` |
| `logical_cores` | *(Not present)* | `16` | Windows `GetSystemInfo()` / POSIX `sysconf()` |
| `ram_total_bytes` | *(Not present)* | `16442789888` | Windows `GlobalMemoryStatusEx()` / POSIX `sysconf()` |
| `ram_total_gb` | *(Not present)* | `15.31` | Derived from `ram_total_bytes` |
| `compiler` | `"gcc 16.1.0"` | `"gcc 16.1.0"` | Compile-time macro introspection (`__GNUC__`) |
| `flags` | Hardcoded string | `"-std=c11 -Wall -Wextra -O2 -Isrc"` | Inferred from optimization flags (`__OPTIMIZE__`) |

---

## 4. Empirical Benchmark Value Updates

### 4.1 Experiment 8: Nesting Depth Scalability Sweep (16 symbols/scope)

| Nesting Depth | Baseline RAM (Old) | Baseline RAM (New) | Two-Tier RAM (Old) | Two-Tier RAM (New) | RAM Reduction | Two-Tier Time (New) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **Depth 4** | 8,845 B | 8,845 B | 7,129 B | 7,129 B | **-19.4% (saved)** | 45.9 $\mu$s |
| **Depth 8** | 14,117 B | 14,117 B | 9,849 B | 9,849 B | **-30.2% (saved)** | 52.3 $\mu$s |
| **Depth 16** | 24,773 B | 24,773 B | 17,367 B | 17,367 B | **-29.9% (saved)** | 76.5 $\mu$s |
| **Depth 32** | 46,117 B | 46,117 B | 32,383 B | 32,383 B | **-29.8% (saved)** | 136.4 $\mu$s |
| **Depth 64** | 88,805 B | 88,805 B | 62,391 B | 62,391 B | **-29.7% (saved)** | 306.7 $\mu$s |
| **Depth 128** | 174,645 B | 174,645 B | 122,383 B | 122,383 B | **-29.9% (saved)** | 575.6 $\mu$s |
| **Depth 256** | *(Not tested)* | **346,357 B** | *(Not tested)* | **227,977 B** | **-34.2% (saved)** | **1,945.1 $\mu$s (2.6x faster than Baseline 5,168.8 $\mu$s)** |

### 4.2 Experiment 8: Total Symbols Scalability Sweep (fixed depth 16)

| Total Symbols | Baseline RAM | Two-Tier RAM | RAM Reduction | Two-Tier Wall Time |
| :---: | :---: | :---: | :---: | :---: |
| **100** | 16,927 B | 8,969 B | **-47.0% (saved)** | 40.2 $\mu$s |
| **500** | 36,638 B | 24,207 B | **-33.9% (saved)** | 155.9 $\mu$s |
| **1,000** | 61,159 B | 44,541 B | **-27.2% (saved)** | 336.6 $\mu$s |
| **2,500** | 136,409 B | 114,676 B | **-15.9% (saved)** | 721.2 $\mu$s |
| **5,000** | 262,301 B | 212,462 B | **-19.0% (saved)** | 1,479.2 $\mu$s |
| **10,000** | 514,892 B | 423,178 B | **-17.8% (saved)** | 4,645.4 $\mu$s |
| **25,000** | 1,280,043 B | 1,008,599 B | **-21.2% (saved)** | 14,332.3 $\mu$s |
| **50,000** | 2,566,392 B | 2,031,893 B | **-20.8% (saved)** | 39,793.5 $\mu$s |

### 4.3 Experiment 13: Embedded Flight Controller Component Breakdown (New)

| Scope Count Traversal | Baseline RAM | Two-Tier RAM ($K=2$) | Two-Tier RAM (No Comp, $K=64$) | String Pool (Two-Tier) | Dominant Overhead Source |
| :---: | :---: | :---: | :---: | :---: | :--- |
| **Scope 1** | 3,321 B | 3,344 B | 3,344 B | 976 B | Initial string chunk & hot table |
| **Scope 5** | 5,894 B | 4,162 B | 4,712 B | 976 B | Initial string chunk |
| **Scope 10** | 5,752 B | 4,748 B | 4,968 B | 1,232 B | String chunk expansion |
| **Scope 20** | 5,795 B | 5,922 B | 6,144 B | 2,408 B | String pool exceeds Baseline live string memory |
| **Scope 30** | 5,844 B | 6,428 B | 6,656 B | 2,920 B | String pool continues growing monotonically |
| **Scope 39 (Final)** | **4,888 B** (post-exit) | **10,288 B** (post-exit) | **9,640 B** (post-exit) | **6,272 B** | **String pool (6.3 KB) + Metadata (1.5 KB)** |
| **Peak During Run** | **7,626 B** | **11,242 B (+47.4%)** | **10,624 B (+39.3%)** | **6,272 B** | Two-Tier overhead is driven by string interning |
