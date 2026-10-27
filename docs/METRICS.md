# Empirical Measurement Methodology & Metrics Specification

This document provides rigorous mathematical definitions, physical units, experimental measurement methodologies, and statistical aggregation procedures for all metrics evaluated across the **Scope-Lifetime-Aware Two-Tier Compressed Symbol Table** benchmark suite.

---

## 1. Statistical Methodology & Experimental Controls

To eliminate system-level scheduling noise, context switching jitter, and dynamic CPU frequency governor fluctuations:
- **Sample Repetitions**: Every measurement is performed across **$N = 15$ independent runs**.
- **Fixed Deterministic Seeds**: Pseudo-random generators (used in differential fuzzing and workload generation) use fixed seeds (`srand(42 + run_id)`).
- **Non-Parametric Aggregation**: Measurements are reported using **Median** and **Interquartile Range (IQR)**:
  $$\text{Median} = Q_2 = \begin{cases} X_{(N+1)/2} & \text{if } N \text{ is odd} \\ \frac{1}{2}\left(X_{(N/2)} + X_{(N/2 + 1)}\right) & \text{if } N \text{ is even} \end{cases}$$
  $$\text{IQR} = Q_3 - Q_1 = X_{(3N/4)} - X_{(N/4)}$$
  The median provides a robust central tendency insensitive to OS interrupt spikes, and the IQR captures dispersion across runs.

---

## 2. Metrics Catalog

### 2.1 Peak Symbol-Table RAM
- **Definition**: The maximum total physical heap memory (in bytes) concurrently allocated by the symbol table runtime at any point during program compilation.
- **Unit**: Bytes ($B$) or Kilobytes ($KB = 1024\text{ B}$).
- **Formula**:
  $$M_{\text{peak}} = \max_{t \in [0, T]} \left( \sum_{p \in \mathcal{A}_t} \text{size}(p) \right)$$
  where $\mathcal{A}_t$ is the active set of memory allocations tracked by `mem_tracker.c` at time $t$.
- **Methodology**: Monitored on every `tracked_malloc`, `tracked_calloc`, `tracked_realloc`, and `tracked_free` invocation with boundary canaries.

### 2.2 Relative Memory Reduction / Expansion
- **Definition**: The percentage difference in peak symbol-table RAM between Two-Tier and the traditional Baseline chained hash table.
- **Unit**: Percent ($\%$).
- **Formula**:
  $$\Delta M = \left( 1.0 - \frac{M_{\text{two\_tier}}}{M_{\text{baseline}}} \right) \times 100\%$$
- **Sign Convention**:
  - Negative sign (e.g., `-58.8% (saved)`): Two-Tier consumes less RAM than Baseline.
  - Positive sign (e.g., `+47.4% (worse)`): Two-Tier consumes more RAM than Baseline (due to block descriptor overhead on workloads with many tiny scopes).

### 2.3 Effective Bytes Per Symbol
- **Definition**: The peak memory overhead normalized by the maximum concurrent live symbols in the symbol table.
- **Unit**: Bytes per symbol ($B/\text{sym}$).
- **Formula**:
  $$B_{\text{sym}} = \frac{M_{\text{peak}}}{S_{\text{peak}}}$$
  where $S_{\text{peak}} = \max_t (\text{count}(\text{live symbols at } t))$.

### 2.4 Two-Tier Component Memory Breakdown
- **Definition**: The spatial distribution of peak symbol table memory across its six internal subsystems:
  1. **Hot Open-Addressing Table**: Contiguous linear-probing table ($(\text{mask} + 1) \times \text{sizeof}(\text{hot\_slot\_t})$).
  2. **Cold Compressed Blocks**: Dynamic delta-varint encoded byte arrays storing demoted scopes.
  3. **String Intern Pool**: Global hash table and string chunk storage for 16-bit name IDs.
  4. **Type Table**: Interned canonical type representations and struct member definitions.
  5. **Active Scope Arenas**: Bump-allocated chunks for hot symbol staging.
  6. **Block Headers & Metadata**: Descriptors (`compressed_scope_block_t`), 64-bit Bloom filter words, scope arrays, and LRU cache structures.
- **Unit**: Bytes ($B$).

### 2.5 Cold-Block Compression Ratio
- **Definition**: The compaction factor achieved by delta-varint encoding relative to 64-bit uncompressed packed symbol records.
- **Unit**: Dimensionless ratio ($x$).
- **Formula**:
  $$R_{\text{comp}} = \frac{\sum_{b \in \mathcal{C}} N_b \times 8}{\sum_{b \in \mathcal{C}} S_{\text{varint}}(b)}$$
  where $\mathcal{C}$ is the set of compressed scopes, $N_b$ is the symbol count in block $b$, and $S_{\text{varint}}(b)$ is the compressed payload size.

### 2.6 Compressed Bits Per Symbol
- **Definition**: The mean number of bits required to store an encoded symbol in cold storage.
- **Unit**: Bits ($b/\text{symbol}$).
- **Formula**:
  $$\text{Bits}_{\text{sym}} = \frac{\sum_{b \in \mathcal{C}} S_{\text{varint}}(b) \times 8}{\sum_{b \in \mathcal{C}} N_b}$$

### 2.7 Operation Micro-Latency (p50 / p95 / p99)
- **Definition**: Time elapsed per individual symbol table primitive operation.
- **Unit**: Nanoseconds per operation ($\text{ns/op}$).
- **Evaluated Operations**:
  - **Insertion**: Resolving name ID, hash-consing type, packing into 64 bits, inserting into hot linear-probe table.
  - **Hot Lookup Hit**: Probing the hot open-addressing table and resolving lexical depth.
  - **Cold Lookup Hit (Cache-Warm)**: Querying ancestral scope whose decompressed representation resides in the 2-slot LRU cache.
  - **Cold Lookup Hit (Forced Decompression)**: Cache-defeating query rotating across $>2$ cold scopes forcing on-demand byte-stream decoding.
  - **Negative Miss Lookup**: Non-existent identifier query traversing hot table and Bloom-filtered cold blocks.
- **Methodology**: High-resolution hardware performance counters (`QueryPerformanceCounter` on Windows, `clock_gettime(CLOCK_MONOTONIC)` on POSIX) averaged over bursts of 500 to 2,000 operations per run.

### 2.8 Scope Teardown & Lifecycle Scaling
- **Definition**: Cumulative wall time required to enter a scope, allocate storage, insert $N \in \{10, 100, 1000\}$ symbols, and tear down the scope.
- **Unit**: Nanoseconds ($\text{ns}$) or microseconds ($\mu\text{s}$).
- **Architectural Comparison**:
  - **Baseline**: Traverses linked-list collision chains and frees individual symbol nodes via `free()`.
  - **Two-Tier (Arena + Hot-Table Sweep)**: Frees per-scope arena in $O(1)$ chunk teardown, followed by linear hot table slot deletion.

### 2.9 Bloom Filter Rejection & False-Positive Rates
- **Definition**: Efficiency of the 64-bit multi-hash Bloom filter in short-circuiting cold block traversals without triggering decompression.
- **Unit**: Percent ($\%$).
- **Formulas**:
  - **Bloom Skip Count**: Invocations of `bloom_test` returning false, avoiding decompression.
  - **Range Skip Count**: Invocations short-circuited by min/max name ID bounds prior to Bloom evaluation.
  - **Bloom False-Positive Rate**:
    $$\text{FP}_{\text{rate}} = \frac{\text{Bloom False Positives}}{\text{Bloom False Positives} + \text{Cold Hits}} \times 100\%$$
    (measured strictly after range-check validation).

### 2.10 Decompression Cache Hit Rate
- **Definition**: Fraction of cold scope queries serviced directly from the 2-slot LRU decompressed cache without executing varint byte-stream decoding.
- **Unit**: Percent ($\%$).
- **Formula**:
  $$\text{HitRate}_{\text{cache}} = \frac{\text{Cache Hits}}{\text{Cache Hits} + \text{Cache Misses}} \times 100\%$$

### 2.11 Dynamic Memory Trace
- **Definition**: Time series of active symbol table memory sampled immediately after processing every AST declaration and statement node during semantic analysis.
- **Unit**: Step (Node index) vs. Memory (Bytes).

### 2.12 Scalability Scaling Trends
- **Definition**: Growth rate of peak RAM and semantic analysis execution time as a function of:
  1. Total identifiers: $100 \le N_{\text{sym}} \le 50{,}000$ (fixed depth 16).
  2. Scope nesting depth: $4 \le D \le 256$ (fixed 16 symbols/scope).
- **Unit**: Symbols / Depth vs. Bytes ($B$) / Time ($\mu\text{s}$).

### 2.13 Enforced Budget Cap Survival
- **Definition**: Binary indicator (`PASS` or `FAIL (OOM)`) reflecting whether a compilation succeeds under strict physical heap caps from $4\text{ KB}$ to $64\text{ KB}$ enforced by the memory tracker allocator wrapper.

### 2.14 End-to-End Compiler Wall Time
- **Definition**: Total elapsed time from source file ingest through lexical analysis, recursive descent parsing, AST construction, semantic type checking, symbol resolution, and complete AST/table destruction.
- **Unit**: Microseconds ($\mu\text{s}$).

### 2.15 Dynamic Allocation Call Counts
- **Definition**: Total count of `malloc()` and `free()` invocations issued during compilation.
- **Unit**: Integer count.

---

## 3. Workload Memory Anomaly Analysis: `embedded_flight_controller.mc`

### 3.1 Empirical Observation
On the `embedded_flight_controller.mc` workload (161 identifiers, 39 lexical scopes, maximum nesting depth of 4 levels), Two-Tier consumes **11,242 bytes (+47.4% worse than Baseline's 7,626 bytes)** in default eager mode ($K=2$), and consumes **10,624 bytes (+39.3% worse)** even with compression disabled ($K=64$).

### 3.2 Component Attribution & Root Cause Analysis
Fine-grained subsystem decomposition via `symtab_twotier_get_memory_breakdown()` identifies the exact components responsible for the higher memory footprint:

| Subsystem Component | Baseline Allocated Bytes | Two-Tier Allocated Bytes ($K=2$) | Difference vs Baseline | Primary Cause of Memory Expansion |
| :--- | :---: | :---: | :---: | :--- |
| **String Storage** | $\le 1,200\text{ B}$ (peak live) | **6,272 B** (`string_pool`) | **+5,072 B** | **String Interner Monotonicity**: Baseline allocates strings with `strdup()` and frees them when function scopes exit. Two-Tier interns all identifiers into a global monotonic string pool whose geometric chunk buffers never shrink, accumulating 6,272 B across all 39 functions. |
| **Fixed Infrastructure** | $0\text{ B}$ | **2,336 B** (`hot_table` + `type_table`) | **+2,336 B** | Two-Tier allocates a 64-slot open-addressing table (768 B) and an interned type table (800 B) to support $O(1)$ hot lookups and type hash-consing. Baseline allocates no global hash infrastructure. |
| **Metadata & Descriptors** | $\approx 256\text{ B}$ (live scopes) | **1,496 B** (scope arrays & caches) | **+1,240 B** | Two-Tier maintains tracking arrays for 39 scopes, 2 LRU decompression cache buffers, and block descriptors. Baseline retains only a 4-entry stack pointer array for active scopes. |
| **Cold Storage Blocks** | $0\text{ B}$ | **1,138 B** (data + Bloom) | **+1,138 B** | 39 tiny scopes (~4 symbols/scope) incur 39 block headers (min/max IDs, length fields, Bloom filter words) that outweigh delta-varint compression gains. |
| **Active Scope Arenas** | $0\text{ B}$ (heap nodes) | $0\text{ B}$ (after scope exit) | $0\text{ B}$ | Arenas are reclaimed upon scope exit in both systems. |

### 3.3 Key Takeaway
The higher memory footprint on `embedded_flight_controller.mc` is **not caused by compression failure**, but primarily by **monotonic string interning (6,272 B)** and **global lookup infrastructure (2,336 B)**. In shallow, multi-function embedded codes where functions execute sequentially, Baseline reclaims local variable names after each function exits. Two-Tier retains all unique names permanently in its canonical interner pool, which pays off on large, deeply nested programs but incurs net overhead on shallow programs with many small functions.
