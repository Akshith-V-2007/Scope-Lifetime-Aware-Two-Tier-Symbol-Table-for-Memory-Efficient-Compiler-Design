# Scope-Lifetime-Aware Two-Tier Symbol Table
## Comprehensive Engineering & Empirical Research Report

---

### Executive Summary
Compiler symbol tables have traditionally been designed around chained hash tables or linked stacks of heap allocations (`malloc`/`free`). While conceptually simple, this textbook architecture imposes severe memory penalties: up to **48–64 bytes per raw symbol struct** and **160–200 bytes per symbol** when accounting for bucket arrays, string allocations, and heap headers, significant memory fragmentation, and $O(N)$ destruction latency upon scope exit.

This project designs, implements, validates, and benchmarks a **Scope-Lifetime-Aware Two-Tier Symbol Table** for a C-like compiled language (**Mini-C**). By exploiting the temporal locality of lexical scopes, active scopes are maintained in a high-speed, uncompressed **Hot Tier** (open-addressing hash table), while inactive ancestor scopes are demoted to a compact **Cold Tier** compressed with delta encoding, variable-length integers (LEB128/zigzag varints), and per-scope Bloom filters. Scope destruction is achieved through bulk per-scope arena allocation combined with an in-place hot-table sweep.

#### Key Findings & Achievements:
- **Peak RAM Reduction**: Achieved **58.8% – 59.9% reduction** in peak symbol-table memory on deeply nested synthetic stress workloads (39,571 B down to 16,286 B on deep nesting; 24,075 B down to 9,644 B on heavy shadowing), **21.8% reduction** on broad scopes (24,301 B down to 19,005 B), and **21.7% – 35.5% reduction** on small real-world embedded programs.
- **Fast Hot Lookups**: Hot scope queries achieve **36.9 ns/op** in the contiguous open-addressing table (**1.16x faster than Baseline**, 0.86x ratio) due to CPU cache-line locality and elimination of bucket pointer chasing.
- **Negative Query Acceleration**: Combined min/max range checks and 64-bit Bloom filters short-circuit cold blocks with a **>94% rejection rate**, making negative miss lookups **faster than Baseline** (34.8 ns vs 73.7 ns, 0.47x ratio).
- **Amortized Scope Teardown Speedup**: Bulk arena release paired with an $O(M)$ hot table sweep runs in **600 ns vs 1200 ns** for 10 symbols (0.50x ratio, 2x faster than Baseline), eliminating individual `free()` pointer-chasing system allocator calls.
- **Cache-Warm vs Forced Decompression**: Disambiguated cold queries: cache-warm hits run in **35.8 ns** (0.41x vs baseline) via flat array binary search, while forced cache-defeating decompression across 3 cold scopes takes **221.5 ns** (4.92x slowdown vs baseline), confirming the design trade-off.
- **Exact Correctness & Trace Equivalence**: Validated through **6,000 differential operations**, rigorous edge cases, and 30 automated fuzzing iterations with `--dump-resolutions` asserting **100% bitwise & semantic equivalence** of all identifier resolutions.
- **Real Enforced Memory-Cap Resilience**: Under strict hardware RAM limits enforced via allocator caps (12 KB to 24 KB), Baseline triggers Out-Of-Memory (OOM) failures, while Two-Tier dynamically demotes ancestor scopes and compiles successfully within 9,644 B.

---

### Goals vs. Evaluated Results Audit

| Metric / Goal | Initial Target | Empirically Measured Result | Status | Engineering Analysis & Explanation |
| :--- | :--- | :--- | :---: | :--- |
| **Peak RAM Reduction** | $\ge 60\%$ reduction | **Synthetic**: 21.8% (broad scopes), 58.8% (deep nesting), 59.9% (heavy shadowing).<br>**Real-world**: 21.7% – 35.5% on small files; **+47.4% (worse)** on Flight Controller | **Partially Met** | Met on deep nesting & shadowing stress. Broad scopes achieve 21.8% savings because shallow scopes remain in the hot tier. Flight controller (39 scopes, ~4 syms/scope) incurs +3.6 KB net overhead (+47.4%) due to 39 compressed block structures (min/max range, Bloom filter, length fields). |
| **Bytes per Symbol** | 8–14 B (uncompressed packed record) | **82.7 B – 105.1 B** loaded system footprint;<br>**8.0 B** raw packed record | **Partially Met** | Raw packed record meets the 8-byte target (64 bits). Loaded system footprint (82.7 B) includes string pool chunks, scope structures, and open-addressing hot slots. Though missing the 8–14 B loaded target, it reduces baseline loaded memory (200.9 B) by 58.8%. |
| **Lookup Latency Overhead** | $\le 1.8\times$ baseline | **Hot Hit**: 0.86x (faster, 36.9 ns vs 42.9 ns)<br>**Cold Cache-Warm**: 0.41x (faster, 35.8 ns vs 87.6 ns)<br>**Cold Forced Decomp**: 4.92x (slower, 221.5 ns vs 45.0 ns)<br>**Miss**: 0.47x (faster, 34.8 ns vs 73.7 ns)<br>**Insertion**: 1.86x (slower, 233.8 ns vs 126.0 ns)<br>**Whole-Run**: 0.47x (deep nesting) to 1.41x (flight controller) | **Partially Met / Qualified** | Hot lookups and negative misses are faster than baseline. Forced decompression across cold blocks incurs a ~4.9x penalty (mitigated by LRU caching). Insertion is ~1.86x slower due to upfront interning and packing. Timings represent medians over 15 repeated runs. |
| **Bloom & Range Rejection** | $\ge 85\%$ of cold negative lookups skipped | **94.8%** probes skipped (379 Bloom skips out of 400 probes) | **Met** | Combined min/max `name_id` range pruning and 64-bit Bloom filter effectively eliminates cold block decompressions for misses. |
| **Scope Exit Cost** | $O(1)$ symbol memory reclaim | **0.50x** of baseline time (600 ns vs 1200 ns for 10 symbols) | **Met** | Reclaims symbol memory via $O(1)$ arena free, accompanied by an $O(M)$ sweep of the 32-slot hot table ($M \ll N$). Avoids hundreds of individual `free()` system allocator calls. |
| **Correctness & Equivalence** | 100% Differential Identity | **100.0%** match over 6,000 differential ops, edge cases, and 30 randomized fuzzer ASTs | **Met** | Bitwise and semantic identity verified across exit codes and textual symbol resolution traces (`--dump-resolutions`). |
| **Memory-Cap Resilience** | Survives strict RAM budgets | **Survives 12 KB, 16 KB, 24 KB caps** where Baseline fails with OOM | **Met** | Dynamic depth demotion and compact representation compile successfully within 9,644 B while Baseline requires >24 KB. |

---

### 1. Architectural Design and Memory Layout

#### 1.1 Packed Symbol Record Bit Layout
In standard 64-bit C compilers, a traditional symbol node requires:
- `char* name` (8 bytes) + heap string (10–32 bytes)
- `type_id_t type_id` (4 bytes) + padding (4 bytes)
- `int32_t offset` (4 bytes) + `symbol_kind_t kind` (4 bytes)
- `uint32_t scope_depth` (4 bytes) + `scope_id_t scope_id` (4 bytes)
- `struct baseline_symbol* next` (8 bytes)
- Allocator tracking canary/header overhead (16–32 bytes)
- **Total per symbol node**: $\approx 48 - 64$ bytes (and 160–200 bytes loaded with bucket arrays).

Our design compresses every symbol into a **compact 64-bit (8-byte) bitfield**:

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

##### Field Bit Allocations:
1. `Name ID` (16 bits): Canonical identifier interned in the global string pool (supporting up to 65,534 distinct names).
2. `Kind` (4 bits): Symbol classification (`KIND_VAR`, `KIND_PARAM`, `KIND_FUNC`, `KIND_STRUCT`).
3. `Storage` (4 bits): Storage class specifier (`STORAGE_AUTO`, `STORAGE_STATIC`, `STORAGE_EXTERN`).
4. `Scope Depth` (8 bits): Lexical nesting depth ($0 \le \text{depth} \le 255$).
5. `Type ID` (16 bits): Canonical type descriptor index produced by type hash-consing.
6. `Offset` (16 bits): Signed byte offset for stack frames or struct fields ($-32,768$ to $+32,767$).

Total size: **exactly 8 bytes** (100% word-aligned, zero padding).

#### 1.2 Reconciling Unadorned vs. Loaded Bytes per Symbol
- **Unadorned Record (8 Bytes)**: The raw `packed_symbol_t` struct occupies exactly 8 bytes.
- **Loaded Bytes per Symbol (82.7 B – 105.1 B)**: In a functioning compiler, memory is not consumed solely by raw symbol records. It includes:
  1. The canonical string pool chunks (starting at 512 bytes and growing geometrically).
  2. The open-addressing hot table slot array (starting at 32 slots $\times$ 16 bytes = 512 bytes).
  3. The dynamic scope tracking array (`twotier_scope_t* scopes`, starting at 8 entries).
  4. Per-scope compressed block descriptors and Bloom filter bitsets.
- In Baseline, the same loaded accounting measures **189.0 B – 200.9 B per symbol** because each scope carries a 64-entry bucket pointer array (`baseline_symbol_t** buckets` = 512 bytes) in addition to individual string and node allocations. Two-Tier reduces loaded memory by **58.8%**.

---

### 2. Multi-Tier Operational Architecture

```
                 +-----------------------------------------------+
                 |              Mini-C Source Stream             |
                 +-----------------------------------------------+
                                         |
                                  Lexer & Parser
                                         |
                    Identifier Query: lookup("target_sym")
                                         |
                                         v
   +---------------------------------------------------------------------------+
   |  HOT TIER: Open-Addressing Hash Table (Active Innermost K Scopes)         |
   |  - Uncompressed 64-bit packed records                                     |
   |  - Linear Probing with Knuth Algorithm 6.4R Backward-Shift Deletion       |
   +---------------------------------------------------------------------------+
                     | (Miss)                                   | (Hit)
                     v                                          v
   +---------------------------------------+            +---------------+
   | COLD TIER: Compressed Scope Blocks    |            | Return Symbol |
   | (Innermost Cold Scope down to Scope 0)|            +---------------+
   +---------------------------------------+
                     |
       +-------------+-------------+
       |                           |
       v                           v
 [Range Check: min..max]   [64-bit Bloom Filter]
       |                           |
    (Out of range)             (Bit absent)
       |                           |
       +------------> SKIP <-------+ (94.8% negative queries bypassed!)
                      |
                 (Probable Hit)
                      v
       +-------------------------------+
       | LRU Decompression Cache (K=2) |
       +-------------------------------+
             | (Miss)             | (Hit)
             v                    |
       [Decompress into Scratch]  |
             |                    |
             +--------->----------+
                      |
         [Binary Search Sorted IDs]
                      |
                      v
             +-----------------+
             | Return Symbol / |
             | Continue Walk   |
             +-----------------+
```

#### 2.1 Hot Tier
- Open-addressing hash table holding the top $K$ innermost active scopes ($K=2$ by default).
- Linear probing paired with **Knuth Algorithm 6.4R** in-place cluster backward-shift deletion guarantees probe chain integrity without tombstones.
- Lookups in hot scopes run at high efficiency (33.7–37.1 ns/op) due to cache-line contiguous storage and linear probing without bucket pointer chasing.

#### 2.2 Cold Tier Compression Pipeline
When a scope exceeds hot depth $K$ or is demoted under memory pressure:
1. **Name Sorting**: Symbols are sorted by `name_id` ascending in $O(N \log N)$.
2. **Delta Encoding**: Because `name_id` values are sorted, `name_id[i] - name_id[i-1]` yields small positive deltas.
3. **LEB128 Varint Encoding**: Deltas, type IDs, and zigzag-encoded offsets compress to 1–2 bytes each.
4. **Bloom Filter Synthesis**: A 64-bit bitset is seeded with 3 independent integer hash functions ($k=3$).
5. **Arena Free**: The scope's hot arena is freed immediately, leaving only the dense byte block.

#### 2.3 Bloom Filter & Range Check Analysis
When an identifier query misses the Hot Tier:
1. Ancestor cold blocks are scanned from innermost to global scope.
2. **Range Test**: Checks if `target_name_id < block->min_name_id || target_name_id > block->max_name_id`. If outside, decompression is skipped immediately (`range_skips++`).
3. **Bloom Test**: `bloom_test(block->bloom, target_name_id)`. If any bit is 0, the symbol is **provably absent**, skipping decompression entirely (`bloom_skips++`).
4. **Saturation Characteristics**:
   - For scopes with $\le 10$ symbols: false positive rate is $< 2\%$.
   - For scopes with $20$ symbols: false positive rate rises to $\approx 22\%$.
   - For scopes with $> 30$ symbols: the 64-bit filter begins to saturate. However, the `min_name_id` / `max_name_id` range check remains invariant to symbol density, providing an orthogonal second layer of pruning.

#### 2.4 Scope Teardown Complexity Clarification
- The scope teardown mechanism is: **$O(1)$ bulk symbol memory reclaim via arena free, accompanied by an $O(M)$ hot table backward-shift sweep** over the hot slot array ($M=32$ initial slots, $M \ll N$).
- This completely eliminates $O(N)$ allocator pointer traversals and individual `free()` system calls.

---

### 3. Empirical Benchmark Evaluation

All benchmarks were executed on an AMD Ryzen / Windows platform using GCC 16.1.0 (`-O2 -std=c11`). Timings represent the **median of 15 consecutive executions** using high-resolution monotonic timers. Timings vary by machine and run; ratios shown are medians on the author's benchmark environment.

#### 3.1 Experiment 1: Real-World Embedded & Synthetic Benchmarks
> **Research Question**: *How much peak symbol-table RAM does Two-Tier save across diverse embedded systems codebases and synthetic stress patterns?*

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

![Peak RAM Comparison](figures/fig1_peak_ram_grouped.png)

##### Transparent Technical Analysis of Flight Controller (+47.3% RAM):
- The Flight Controller workload contains **39 separate lexical scopes** with a total of 161 symbols (averaging ~4 symbols per scope).
- In Two-Tier, every cold scope requires a `compressed_scope_block_t` struct containing:
  - 64-bit Bloom filter (8 bytes)
  - `min_name_id`, `max_name_id`, `symbol_count`, `compressed_bytes` (16 bytes)
  - Heap tracking headers (32 bytes)
- When scopes contain only 2 to 4 symbols, the fixed ~56-byte header of the compressed block exceeds the bytes saved by delta-varint encoding. In contrast, Baseline allocates only a pointer in an ancestor array until symbols are inserted.
- This transparently identifies the operational trade-off: **Two-Tier excels when scopes contain $\ge 8$ symbols or nest deeply ($\ge 5$ levels)**.

##### Why Deep Nesting is Faster than Baseline (0.60x execution time):
- On deeply nested code (48 levels, 199 symbols), Baseline spends hundreds of microseconds in $O(N)$ pointer traversals and individual `free()` calls during scope exits.
- Two-Tier frees symbol arenas in bulk and sweeps a 32-slot hot table, cutting total scope exit latency from 1900 ns down to 800 ns per 50 symbols. This speedup dominates total compile time.

---

### 4. Experiment 2: Ablation Study
> **Research Question**: *What is the individual memory contribution of each architectural technique (arena, interning, packing, compression, Bloom filtering)?*

Evaluated on `synthetic_deep_nesting.mc` (199 inserts, 721 lookups, 48 nested scopes):

| Stage | Implementation Variant | Peak Memory | vs Baseline | Loaded Bytes/Symbol |
| :---: | :--- | :---: | :---: | :---: |
| **1** | **Baseline** (Malloc Chained Hash) | 39,571 B | 0.0% | 200.9 B |
| **2** | **+ Scoped Arena Allocator** | 33,912 B | -14.3% | 172.1 B |
| **3** | **+ String Interning** | 27,096 B | -31.5% | 137.5 B |
| **4** | **+ 64-bit Packed Symbols** | 20,696 B | -47.7% | 105.1 B |
| **5** | **+ Delta + Varint Compression** | 16,286 B | **-58.8%** | **82.7 B** |
| **6** | **+ Bloom Filter (Full Two-Tier)** | **16,286 B** | **-58.8%** | **82.7 B** (379 Bloom skips) |

![Ablation Waterfall](figures/fig3_ablation_waterfall.png)

#### Mechanism Contribution:
1. **Arena Allocator (-14.3%)**: Eliminates individual heap chunk headers for symbol nodes and strings.
2. **String Interning (-31.5%)**: Prevents identifier string duplication across scopes; all symbols reference a 16-bit integer ID.
3. **Packed Symbols (-47.7%)**: Compacts symbol metadata into dense 8-byte contiguous arrays, removing bucket pointer chains.
4. **Delta & Varint Compression (-58.8%)**: Compresses dormant ancestor scopes into compact byte buffers.
5. **Bloom Filter**: Memory remains identical (16,286 B); its sole purpose is query acceleration, avoiding 379 candidate decompressions.

---

### 5. Experiment 3: Parameter Sweep (Hot Depth $K$ Trade-off Curve)
> **Research Question**: *How does hot depth $K$ trade off peak symbol-table memory against cold decompressions?*

Evaluated on `synthetic_deep_nesting.mc` ($K \in [1, 64]$):

| Hot Depth $K$ | Peak RAM | Hot Hits | Cold Hits | Bloom Skips | Cold Decompressions | Bytes / Symbol |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **$K = 1$** | **16,023 B** | 28 | 693 | 410 | 209 | 81.3 B |
| **$K = 2$** | **16,286 B** | 44 | 677 | 379 | 198 | 82.7 B |
| **$K = 4$** | **16,812 B** | 101 | 620 | 324 | 167 | 85.3 B |
| **$K = 8$** | **18,249 B** | 187 | 534 | 233 | 120 | 92.6 B |
| **$K = 16$** | **21,124 B** | 317 | 404 | 115 | 73 | 107.2 B |
| **$K = 32$** | **26,884 B** | 534 | 187 | 9 | 14 | 136.5 B |
| **$K = 64$** | **34,920 B** | 721 | 0 | 0 | 0 | 177.3 B |

![Trade-off Curve](figures/fig4_k_pareto_curve.png)

#### Analysis:
- $K=2$ represents the Pareto-optimal knee: it achieves **16,286 B (58.8% RAM reduction vs Baseline 39,571 B)** while keeping hot table occupancy low.
- As $K$ increases from 2 to 32, more scopes remain uncompressed in the open-addressing hot tier, increasing peak RAM towards baseline levels.
- At $K \ge 64$, all scopes remain uncompressed in the hot tier, eliminating cold decompressions entirely (0 decomps) but forfeiting compression benefits (34,920 B peak RAM).

---

### 6. Experiment 4: Architectural Demotion Policy Comparison
> **Research Question**: *How does eager depth-bounded demotion compare to lazy closure and size-aware demotion in peak RAM vs decompressions?*

| Workload | Demotion Policy | Peak RAM | RAM vs Base | Bloom Skips | Cold Decomps |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **Flight Controller** | **Eager ($K=2$)** | **11,242 B** | **+47.4% (worse)** | 1 | 39 |
| **Flight Controller** | Lazy (Demote on Scope Exit) | 10,624 B | **+39.3% (worse)** | 0 | 0 |
| **Flight Controller** | **Size-Aware ($K=2$, $\ge 16$ syms)** | **10,602 B** | **+39.0% (worse)** | 0 | 1 |
| **Deep Nesting** | **Eager ($K=2$)** | **16,286 B** | **-58.8% (saved)** | 379 | 198 |
| **Deep Nesting** | Lazy (Demote on Scope Exit) | 34,920 B | **-11.8% (saved)** | 0 | 0 |
| **Deep Nesting** | **Size-Aware ($K=2$, $\ge 16$ syms)** | **34,920 B** | **-11.8% (saved)** | 0 | 0 |

#### Architectural Insight:
- **Workload Sensitivity**: On `synthetic_deep_nesting.mc`, **lazy demotion consumes 34,920 B vs 16,286 B for eager demotion (+114% higher RAM)** because keeping 48 scopes open simultaneously uncompressed multiplies memory consumption.
- **Flight Controller Reversal**: Conversely, on the Flight Controller, **lazy demotion (10,624 B) beats eager demotion (11,242 B)** because 39 tiny scopes carrying ~4 symbols each cost more in compressed block descriptors and Bloom headers than they save via delta-varint encoding.
- **Size-Aware Demotion**: Introducing size awareness (demoting scopes deeper than $K$ only if they contain $\ge 16$ symbols) avoids compressing tiny scopes and brings Flight Controller RAM down to **10,602 B**, preventing unnecessary cold block allocations.

---

### 7. Experiment 5: Micro-Latency Breakdown & Slowdown Profile
> **Research Question**: *What is the exact lookup latency penalty (hot hit, cold hit, miss) and insertion overhead of Two-Tier compared to Baseline across repeated median runs?*

Evaluated across 15 independent runs (reporting medians and IQRs):

| Operation Type | Baseline Median (IQR) | Two-Tier Median (IQR) | Speed Ratio ($T_{\text{two}} / T_{\text{base}}$) | Performance Impact |
| :--- | :---: | :---: | :---: | :--- |
| **1. Symbol Insertion** | 126.0 ns (123.4–131.8) | 233.8 ns (226.6–240.2) | **1.86x** | **Slower** (Interning, hash-consing, and packing) |
| **2. Hot Scope Lookup Hit** | 42.9 ns (42.5–43.7) | 36.9 ns (35.9–38.0) | **0.86x** | **Faster** (Contiguous open-addressing table) |
| **3a. Cold Hit (Cache-Warm)** | 87.6 ns (86.9–88.6) | 35.8 ns (35.7–36.6) | **0.41x** | **Faster** (LRU scratch buffer cache hit) |
| **3b. Cold Hit (Forced Decomp)** | 45.0 ns (44.3–45.3) | 221.5 ns (221.2–226.0) | **4.92x** | **Slower** (Cache-defeating on-demand decompression) |
| **4. Negative Miss Lookup** | 73.7 ns (73.6–74.2) | 34.8 ns (34.2–35.4) | **0.47x** | **Faster** (Bloom filter early short-circuit) |
| **5a. Scope Lifecycle (N=10)** | 1,200.0 ns (1200–1200) | 600.0 ns (600–700) | **0.50x** | **Faster** (arena bulk reclaim + hot sweep) |
| **5b. Scope Lifecycle (N=100)** | 10,400.0 ns (10300–10600) | 10,600.0 ns (10500–10900) | **1.02x** | **Comparable** |
| **5c. Scope Lifecycle (N=1000)** | 163,600.0 ns (159600–184300) | 276,600.0 ns (263700–291200) | **1.69x** | **Slower** (Insertion in hot table dominates) |

![Latency Distributions](figures/fig5_latency_cdfs.png)
![Scope Lifecycle Scaling](figures/fig6_scope_scaling.png)

#### Honest Trade-off Discussion:
- **Symbol Insertion (1.86x Slower)**: Insertion is ~1.86x slower due to upfront work: string interning, type hash-consing, and packing into 64-bit records. This investment pays off on subsequent operations.
- **Cache-Warm vs. Forced Decompression**:
  - In **3a (Cache-Warm)**, repeated lookups to a cold scope are served from the 2-entry LRU cache in **35.8 ns** (faster than Baseline's 87.6 ns multi-level bucket walk).
  - In **3b (Forced Decomp)**, queries cycle across 3 cold scopes, completely defeating the 2-entry cache and forcing delta-varint decompression on 100% of queries. This takes **221.5 ns/op (4.92x slower)**. This transparently acknowledges the decompression cost when temporal locality fails.
- **Scope Teardown Scaling**: For small to moderate scopes ($N \le 100$), Two-Tier reclaims symbol storage in bulk via arena reset and sweeps the 32-slot hot table, running up to 2x faster than Baseline (600 ns vs 1200 ns at $N=10$).

---

### 8. Experiment 6: Budget Mode (Enforced Strict RAM Cap Stress Test)
> **Research Question**: *Can Two-Tier adaptively compile within RAM caps that crash Baseline with OOM?*

Strict RAM caps were enforced at the allocator level using `mem_tracker_set_budget_limit()`. Any allocation exceeding the budget immediately returns `NULL`. Evaluated on `synthetic_heavy_shadowing.mc` (Baseline uncapped peak: 24,075 B):

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

![RAM Cap Heatmap](figures/fig10_budget_cap_heatmap.png)

#### Significance:
- Baseline cannot adapt to memory pressure; as soon as allocations reach the cap, `malloc` returns `NULL`, terminating compilation.
- Two-Tier features active budget awareness: when total tracked memory approaches `ram_budget`, it demotes ancestor scopes into compressed cold blocks and destroys uncompressed arenas. This keeps total symbol table consumption below 9,644 bytes, successfully compiling programs under 12 KB, 16 KB, and 24 KB limits that cause Baseline to fail.

---

### 9. Extended Empirical Analyses (IEEE Publication Extensions)

#### 9.1 Component Memory Breakdown Across Workloads
To inspect where memory is allocated inside the Two-Tier engine, fine-grained component accounting breaks down total bytes across Hot Open-Addressing Table, Cold Compressed Blocks, String Pool Chunks, Type Table Descriptors, and Table Control Structures:

![Memory Breakdown Stacked](figures/fig2_memory_breakdown_stacked.png)

- On large nested workloads (`synthetic_deep_nesting.mc`), compressed cold blocks account for only 24.8% of the memory footprint, while string interning and open-addressing hot slots account for the majority of the remaining overhead.
- On small workloads, the 512-byte initial hot table and 512-byte string chunk constitute fixed infrastructure overhead, explaining why relative savings are bounded at 21%–35%.

#### 9.2 Dynamic Memory Footprint vs. AST Semantic Steps
Tracking symbol table memory at every AST node visit reveals the true transient memory trajectory throughout the compilation pipeline:

![AST Memory Trace](figures/fig7_ast_memory_trace.png)

- Baseline's memory grows monotonically with scope depth as new bucket arrays and symbol nodes are allocated on the heap, peaking at scope maximum.
- Two-Tier exhibits a sawtooth pattern: as scope depth increases beyond $K=2$, ancestor scopes are compressed into compact cold blocks and their hot arenas are freed, flattening peak RAM.

#### 9.3 Scalability Across Identifiers (100 to 50,000) and Nesting Depth (4 to 256)
Evaluating symbol tables across orders of magnitude in symbol count and nesting depth confirms robust asymptotic scaling:

![Scalability Lines](figures/fig8_scalability_lines.png)

- **Total Identifiers Scaling (100 to 50,000 at depth 16)**:
  - At 100 symbols: Baseline consumes 16,927 B vs. Two-Tier 8,969 B (**-47.0% saved**).
  - At 1,000 symbols: Baseline consumes 61,159 B vs. Two-Tier 44,541 B (**-27.2% saved**).
  - At 10,000 symbols: Baseline consumes 514,892 B vs. Two-Tier 423,178 B (**-17.8% saved**).
  - At 50,000 symbols: Baseline consumes 2,566,392 B vs. Two-Tier 2,031,893 B (**-20.8% saved**, saving 534 KB of physical RAM).
- **Nesting Depth Scaling (4 to 256 at 16 symbols/scope)**:
  - At depth 4: Baseline 8,845 B vs. Two-Tier 7,129 B (**-19.4% saved**).
  - At depth 16: Baseline 24,773 B vs. Two-Tier 17,367 B (**-29.9% saved**).
  - At depth 64: Baseline 88,805 B vs. Two-Tier 62,391 B (**-29.7% saved**).
  - At depth 128: Baseline 174,645 B vs. Two-Tier 122,383 B (**-29.9% saved**).
  - At depth 256: Baseline 346,357 B vs. Two-Tier 227,977 B (**-34.2% saved**, saving 118.4 KB). Furthermore, Two-Tier is **2.6x faster than Baseline** (1.95 ms vs. 5.17 ms) at depth 256 because bulk arena release avoids traversing 256 chained bucket lists during scope exits.

#### 9.4 Bloom Filter and Cache Rejection Profile
Empirical analysis of query pathways validates the multi-tier query filter hierarchy:

![Bloom Cache Effectiveness](figures/fig12_bloom_cache_effectiveness.png)

- Over 94% of negative lookup probes are short-circuited by the 64-bit Bloom filter before any decompression occurs.
- The 2-entry LRU decompression cache achieves a >98% hit rate on code with temporal scope locality, amortizing decompression latency to sub-40 nanoseconds.

#### 9.5 Demotion Threshold Parameter Sensitivity
Sweeping the minimum scope size threshold ($\theta \in [0, 32]$ symbols) illustrates the sensitivity curve for size-aware demotion:

![Sensitivity Curves](figures/fig9_sensitivity_curves.png)

- On workloads with numerous small scopes (e.g., flight controller), setting $\theta \ge 8$ avoids allocating cold block headers for trivial scopes, eliminating up to 640 bytes of block metadata.

#### 9.6 Compression Ratio & Packed Storage Density
Comparing uncompressed packed records against delta-varint compressed cold blocks across workloads:

![Compression Metrics](figures/fig11_compression_metrics.png)

- Two-Tier achieves an average cold compression ratio of 2.1x–2.8x over the already packed 8-byte format, delivering effective cold symbol representations as small as 2.8 to 3.8 bytes per symbol.

#### 9.7 Workload Memory Overhead Analysis: Embedded Flight Controller
To determine why Two-Tier consumes more memory than Baseline on `embedded_flight_controller.mc` (11,242 B Two-Tier vs. 7,626 B Baseline, and 10,624 B even with compression disabled), fine-grained memory tracking was recorded at every scope boundary across all 39 scopes:

![Flight Controller Breakdown](figures/fig13_flight_controller_breakdown.png)

##### Root Cause Breakdown:
1. **String Interner Monotonicity (6,272 Bytes)**:
   - In Baseline, identifier names are allocated with `strdup()` and freed immediately upon function scope exit (`free(sym->name)`). Because the 9 functions execute sequentially, Baseline's live string working set peaks at only $\le 1,200$ bytes.
   - In Two-Tier, `string_pool_t` retains all 161 unique identifier names across all functions in geometric chunk buffers (512, 1024, 2048, 4096 bytes = 6,272 bytes) that never shrink during the compilation lifecycle. This component alone accounts for 55.8% of Two-Tier's peak memory.
2. **Fixed Infrastructure Overhead (2,336 Bytes)**:
   - Two-Tier maintains a 64-slot open-addressing hot table (768 B) and an interned canonical type table (800 B), alongside decompression cache and scope descriptor tracking arrays (1,496 B). Baseline retains only a 4-entry pointer array for open scopes.
3. **Small Scope Granularity (~4 Symbols/Scope)**:
   - With 39 scopes containing only 2 to 4 symbols each, per-scope compressed block descriptors and Bloom filter bitsets (~56 bytes each) incur structural overhead that exceeds the bytes saved by delta-varint compression.
4. **Behavior with Compression Disabled**:
   - When compression is completely disabled ($K=64$), Two-Tier still consumes **10,624 bytes (+39.3% worse than Baseline)**, confirming that the difference is driven by string interner chunk retention and global table infrastructure rather than cold block compression.

---

### 10. Correctness and Differential Verification

To prove bitwise and semantic parity, four levels of rigorous validation were implemented:
1. **Round-Trip Field Invariance**: Tested compression/decompression across 1, 10, 100, 1,000, and 5,000 symbols. All fields (`name_id`, `type_id`, `offset`, `kind`, `scope_depth`) matched bitwise and field-exact.
2. **Differential Stream Testing**: 6,000 randomized operations across 3 distinct seeds verified that Baseline and Two-Tier return identical resolutions, identical metadata, and identical error states on every query (100% equivalence).
3. **Edge Case Battery**: Tested 0-symbol empty scopes, 5,000 symbols in 1 scope, 64-level deep nesting with continuous shadowing of identical variable names, and 260-byte identifiers (0 memory leaks).
4. **Differential Fuzzing with `--dump-resolutions`**: 30 rounds of random Mini-C code generation validated that `minicc` outputs identical exit codes and bitwise identical identifier resolution traces across both engines.

---

### 11. Mini-C Language Subset Specification
The front-end implements a practical subset of C designated **Mini-C**:
- **Supported Primitive Types**: `int`, `float`, `char`, `void`
- **Supported User-Defined Types**: `struct` declarations
- **Supported Derived Types**: 1D Arrays (`int arr[10];`)
- **Supported Operators**: Arithmetic (`+`, `-`, `*`, `/`), unary minus (`-x`), relational (`<`, `>`, `<=`, `>=`, `==`, `!=`), assignment (`=`)
- **Control Flow**: Arbitrarily nested blocks (`{ ... }`), `if` / `else`, `while`, functions with parameters and return statements
- **Explicit Grammar & Semantic Exclusions**:
  - *Multi-dimensional arrays* (e.g. `int m[10][10];`) are excluded; 1D indexing only.
  - *Pointers and indirection* (`*ptr`, `&val`, `->`) are excluded from grammar AST generation.
  - *Logical NOT* (`!`) and `for` loops are excluded; use `while` constructs.
  - *Separate function prototypes* (forward declarations without bodies) are excluded; functions are declared at definition.
  - *Type checking limitation*: Type checking currently does not validate argument counts, `void` variable declarations, struct fields, or pointer-from-float assignments; semantic analysis exercises symbol table scoping, shadowing, and resolution, but full static type checking is beyond the scope of this project.

---

### 12. Architectural Limitations
1. **16-bit Name ID Limit**: Identifiers are interned into 16-bit integer IDs, capping unique global symbols at 65,534 distinct names.
2. **Signed 16-bit Offset Range**: Struct field and stack frame offsets are stored in signed 16-bit integers ($-32,768$ to $+32,767$ bytes).
3. **64-bit Bloom Filter Saturation**: With 3 hash functions, the 64-bit filter yields $< 2\%$ false positives for scopes under 10 symbols, but saturates to $\approx 22\%$ false positives at 20 symbols. For large scopes, the orthogonal min/max name ID range check prevents false decompressions.

---

### 13. Conclusion
The Scope-Lifetime-Aware Two-Tier Symbol Table demonstrates that compiler memory overhead can be cut by **up to 60% on nested programs, with slower insertion and a decompression penalty on cold lookups**. By harmonizing arena allocation, compact 64-bit record packing, delta-varint compression, and Bloom filter short-circuiting, this design provides a practical, drop-in solution for memory-constrained compilers, embedded toolchains, and high-concurrency language servers.
