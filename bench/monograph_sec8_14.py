# monograph_sec8_14.py
# Generates Sections 8 through 14 of the comprehensive monograph

def get_section_8_to_14():
    return """## 8. Publication-Quality Figure Directory and Visual Analysis

All figures are compiled according to IEEE formatting standards (single-column width 3.5 in, double-column width 7.0 in, 9 pt serif font, Okabe-Ito colorblind-safe palette with distinct hatches and markers, white background, no internal titles) and exported to [`report/figures/`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/) in **PNG @ 300 DPI**, **vector SVG**, and **vector PDF**:

1. **Figure 1 ([`fig1_peak_ram_grouped`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig1_peak_ram_grouped.png))**: Grouped bar chart comparing peak RAM across all 6 engines on all 8 workloads. Features an unclipped bottom legend and logarithmic y-axis with plain integer tick labels in KB (`[2, 5, 10, 20, 40, 60]`).
2. **Figure 2 ([`fig2_memory_breakdown_stacked`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig2_memory_breakdown_stacked.png))**: Subsystem stacked bar chart showing the spatial distribution of hot table, cold blocks, string pool, type table, active arenas, and control metadata.
3. **Figure 3 ([`fig3_ablation_waterfall`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig3_ablation_waterfall.png))**: Waterfall chart illustrating the step-by-step reduction from Baseline (39,571 B) to Full Two-Tier (16,286 B) as arena allocation (-14.3%), string interning (-31.5%), 64-bit packing (-47.7%), and delta-varint compression (-58.8%) are applied incrementally.
4. **Figure 4 ([`fig4_k_pareto_curve`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig4_k_pareto_curve.png))**: Pareto trade-off curve sweeping hot depth $K \in [1, 64]$ against peak RAM and cold decompressions, confirming $K=2$ as the Pareto-optimal knee.
5. **Figure 5 ([`fig5_latency_cdfs`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig5_latency_cdfs.png))**: Cumulative distribution functions (CDFs) and box plots for operation micro-latencies, contrasting hot hits (36.9 ns), cache-warm hits (35.8 ns), and forced decompressions (221.5 ns).
6. **Figure 6 ([`fig6_scope_scaling`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig6_scope_scaling.png))**: Scope lifecycle scaling curves comparing Two-Tier bulk arena resets against Baseline chained `free()` across 10, 100, and 1,000 symbols.
7. **Figure 7 ([`fig7_ast_memory_trace`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig7_ast_memory_trace.png))**: Dynamic node-by-node AST semantic step memory trace contrasting Baseline's monotonic expansion against Two-Tier's sawtooth bounded trajectory.
8. **Figure 8 ([`fig8_scalability_lines`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig8_scalability_lines.png))**: Asymptotic scalability lines sweeping total symbols up to 50,000 (log-log) and nesting depth from 4 to 256 levels without internal plot titles.
9. **Figure 9 ([`fig9_sensitivity_curves`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig9_sensitivity_curves.png))**: Minimum scope size threshold sensitivity curves ($\theta \in [0, 32]$) for size-aware demotion on shallow vs. deeply nested programs.
10. **Figure 10 ([`fig10_budget_cap_heatmap`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig10_budget_cap_heatmap.png))**: Heatmap matrix illustrating Baseline OOM failures vs. Two-Tier survival across hardware RAM budget caps from 4 KB to 64 KB.
11. **Figure 11 ([`fig11_compression_metrics`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig11_compression_metrics.png))**: Dual-axis chart displaying cold block compression ratios ($2.3\times - 2.7\times$) and compressed bits per symbol ($24 - 27\text{ b/sym}$).
12. **Figure 12 ([`fig12_bloom_cache_effectiveness`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig12_bloom_cache_effectiveness.png))**: Grouped bar chart comparing Bloom filter rejection rates ($>94\%$) against LRU decompression cache hit rates ($>98\%$).
13. **Figure 13 ([`fig13_flight_controller_breakdown`](file:///c:/Users/hp/Desktop/CD%20Project/report/figures/fig13_flight_controller_breakdown.png))**: Dual-panel monograph figure illustrating the flight controller memory trajectory (Panel 1) and stacked component area chart (Panel 2) proving that string pool retention drives the overhead.

---

## 9. Detailed Subsystem Component Attribution: The Flight Controller Analysis

An examiner or peer reviewer evaluating Experiment 1 will immediately observe that on `embedded_flight_controller.mc`, Two-Tier uses **11,242 bytes vs. Baseline's 7,626 bytes (+47.4% worse)**. 

To prevent misattributing this overhead to compression inefficiency, Experiment 13 isolated each component's contribution across all 39 scopes:

### 1. String Interner Monotonicity (6,272 Bytes / 55.8% of Total RAM)
In Baseline, strings are allocated using `strdup()` and freed immediately upon exiting that scope via `free(sym->name)`. In `embedded_flight_controller.mc`, 161 unique symbols are declared across 9 sequential functions. Because functions execute sequentially, Baseline reclaims each function's local strings as soon as the function terminates. Baseline's live string footprint never exceeds $\approx 1.2\text{ KB}$.

In Two-Tier, `string_pool_t` canonicalizes strings globally. Geometric chunk allocations (512, 1024, 2048, 4096 bytes) **never shrink during the compilation lifecycle**, accumulating **6,272 bytes**. The string pool alone accounts for more than 82% of Baseline's entire memory budget.

### 2. Global Fixed Infrastructure Overhead (2,336 Bytes)
Two-Tier maintains permanent global infrastructure to support $O(1)$ hot lookups and type canonicalization:
- 64-slot open-addressing table: **768 bytes**
- Interned type table hash table: **800 bytes**
- Decompression cache & scope descriptor array: **1,496 bytes**

Baseline allocates no global lookup infrastructure, maintaining only an open-scope stack pointer array.

### 3. Small Scope Granularity (~4 Symbols/Scope)
With 39 scopes containing an average of only 4 symbols, the fixed 56-byte header of each compressed block descriptor (Bloom filter, range bounds, array metadata) exceeds the bytes saved by delta-varint encoding.

### 4. Behavior with Compression Disabled
When compression is completely disabled ($K=64$), Two-Tier consumes **10,624 bytes (+39.3% worse than Baseline)**. This empirically proves that the expansion on shallow, multi-function embedded codes is **not caused by compression**, but by the intentional architectural trade-off of maintaining a persistent, global string interner pool and open-addressing infrastructure.

---

## 10. Threats to Validity

1. **Construct Validity (Physical vs. Virtual Accounting)**: Memory measurements in this study capture physical heap allocations via tracked allocator wrappers (`mem_tracker.c`). While this tracks exact byte consumption, it does not model OS virtual memory page table granularity (4 KB pages).
2. **Internal Validity (Timer Jitter & CPU Throttling)**: Micro-benchmarking sub-microsecond operations is susceptible to OS interrupts and dynamic CPU frequency governors. To mitigate this threat, every metric was evaluated across 15 runs reporting non-parametric medians and IQRs.
3. **External Validity (Language Scope)**: Mini-C includes structs, 1D arrays, and control flow, but excludes multi-dimensional arrays, raw pointer indirection, and C++ template instantiation. In languages with template expansion, symbol density per scope is substantially higher, which would amplify Two-Tier's memory savings.

---

## 11. Related Work and Comparative Taxonomy

Traditional compiler symbol tables have universally prioritized implementation simplicity over cache locality and memory density. Aho et al. (Dragon Book) describe chained hash tables where each scope points to an ancestor bucket array. Hanson's *C Interfaces and Implementations* describes an open-addressing symbol table with dynamic resizing, but maintains symbols uncompressed throughout compilation.

Succinct and compressed data structures (Jacobson; Navarro) have explored Elias-Fano encoding and wavelet trees for static text indexing. However, traditional succinct data structures are static and read-only. Two-Tier introduces a **dynamically mutating two-tier hierarchy** that combines dynamic $O(1)$ linear probing with on-demand delta-varint compression, specifically tailored to the LIFO scope lifecycle of programming language compilers.

In modern industrial toolchains (Clang/LLVM, Rust Analyzer, Roslyn), language servers consume gigabytes of memory to maintain ASTs and index databases across hundreds of translation units. The Two-Tier architecture provides a practical, drop-in paradigm for language server symbol indexing, reducing persistent ancestor memory while preserving sub-40ns lookup latency.

---

## 12. Future Research Directions

1. **Generational String Interner Reclamation**: Introducing scope-depth-aware string chunks to reclaim local identifier strings from closed functions would eliminate the 6.3 KB string pool expansion observed on `embedded_flight_controller.mc`.
2. **SIMD Vectorized Bloom Filtering**: Utilizing AVX2 or ARM NEON vector instructions to probe 4 or 8 Bloom filter masks concurrently across cold blocks.
3. **Multi-Threaded Concurrent Hot Tiers**: Extending the Hot Tier to support lock-free open-addressing with atomic Knuth 6.4R compaction to support parallel parsing of independent function bodies.
4. **Persistent Hierarchical AST Indexing**: Integrating Two-Tier compression directly into language server AST serialization formats to enable sub-10 MB resident memory profiles for full compiler daemons.

---

## 13. Conclusion

The Scope-Lifetime-Aware Two-Tier Symbol Table architecture demonstrates that traditional compiler symbol tables suffer from massive, unnecessary memory inflation due to their ignorance of scope temporal locality. 

By unifying **64-bit packed records**, **open-addressing with Knuth 6.4R cluster compaction**, **delta-varint cold compression**, **Bloom filter short-circuiting**, and **scoped arena allocation**, Two-Tier:
- Cuts peak symbol table RAM by **up to 60.0%** on nested programs.
- Accelerates hot lookups by **16% (36.9 ns vs. 42.9 ns)**.
- Accelerates negative misses by **53% (34.8 ns vs. 73.7 ns)**.
- Accelerates scope teardown by **2.0x (600 ns vs. 1,200 ns)**.
- Scales cleanly to **256 nesting levels** (saving 118 KB RAM and running 2.66x faster than Baseline).
- Survives strict **12 KB to 24 KB hardware RAM caps** where traditional compilers crash.

This architecture establishes a new Pareto-optimal frontier for compiler front-ends, embedded cross-compilers, and high-concurrency language servers operating in memory-constrained computing environments.

---

## 14. Complete References and Academic Bibliography

1. Aho, A. V., Lam, M. S., Sethi, R., & Ullman, J. D. (2006). *Compilers: Principles, Techniques, and Tools* (2nd ed.). Addison-Wesley.
2. Cooper, K. D., & Torczon, L. (2011). *Engineering a Compiler* (2nd ed.). Morgan Kaufmann.
3. Knuth, D. E. (1998). *The Art of Computer Programming, Volume 3: Sorting and Searching* (2nd ed., Algorithm 6.4R). Addison-Wesley.
4. Fraser, C. W., & Hanson, D. R. (1995). *A Retargetable C Compiler: Design and Implementation*. Addison-Wesley.
5. Hanson, D. R. (1997). *C Interfaces and Implementations: Techniques for Creating Reusable Software*. Addison-Wesley.
6. Bloom, B. H. (1970). Space/time trade-offs in hash coding with allowable errors. *Communications of the ACM*, 13(7), 422–426.
7. Elias, P. (1974). Efficient storage and retrieval by content and address of static files. *Journal of the ACM*, 21(2), 246–260.
8. Navarro, G. (2016). *Compact Data Structures: A Practical Approach*. Cambridge University Press.
9. Lemire, D., & Boytsov, L. (2015). Decoding billions of integers per second through vectorization. *Software: Practice and Experience*, 45(1), 1–29.
10. Lattner, C., & Adve, V. (2004). LLVM: A compilation framework for lifelong program analysis & transformation. In *Proceedings of the international symposium on Code generation and optimization (CGO)* (pp. 75–86).
11. Matsakis, N. D., & Klock, F. S. (2014). The Rust language. *ACM SIGAda Ada Letters*, 34(3), 103–104.
12. Microsoft Corporation. (2020). *Language Server Protocol Specification (v3.16)*. Microsoft Developer Network.
13. FreeRTOS Kernel. (2024). *FreeRTOS Real-Time Kernel Technical Documentation*. Real Time Engineers Ltd.
14. Zephyr Project. (2024). *Zephyr RTOS Architecture and Network Subsystem Specification*. Linux Foundation.
15. Guthaus, M. R., Ringenberg, J. S., Ernst, D., Austin, T. M., Mudge, T., & Brown, R. B. (2001). MiBench: A free, commercially representative embedded benchmark suite. In *Proceedings of the IEEE 4th Annual Workshop on Workload Characterization* (pp. 3–14).

---
"""

if __name__ == '__main__':
    print("Section 8-14 generator loaded.")
