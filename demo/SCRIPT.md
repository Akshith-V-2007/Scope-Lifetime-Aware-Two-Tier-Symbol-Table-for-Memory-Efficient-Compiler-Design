# Timed Narration Script: Scope-Lifetime-Aware Two-Tier Symbol Table Architecture

**Total Target Duration:** 05:20 (320 seconds) at 1.0x playback speed  
**Target Resolution:** 1080p (1920x1080) at 60 FPS  
**Target Venue:** IEEE Transactions on Software Engineering (TSE) / Artifact Submission  

---

## Chapter Overview & Timeline

| Chapter | Title | Timestamp | Duration | Visual Focus |
| :---: | :--- | :---: | :---: | :--- |
| **1** | Problem Statement & Textbook Baseline | `00:00 - 00:35` | 35s | Baseline chained table, heap fragmentation, 160–200 B/sym |
| **2** | Two-Tier Architecture & Temporal Locality | `00:35 - 01:10` | 35s | 90% query locality, Hot innermost $K=2$ vs Cold dormant blocks |
| **3** | 64-bit Packed Symbol Bitfield Architecture | `01:10 - 01:45` | 35s | 8-byte packed record bitfield diagram, 64-byte L1 cache line |
| **4** | Live Scope Entry & Delta + Varint Demotion | `01:45 - 02:30` | 45s | Scope entry stepper, ID sorting, relative delta, LEB128 varint |
| **5** | Multi-Tier Identifier Lookup Walkthrough | `02:30 - 03:10` | 40s | 38.6 ns Hot hit, Bloom short-circuit (>97% rejection), LRU cache |
| **6** | Scope Exit, Arena Reclaim & Knuth 6.4R | `03:10 - 03:45` | 35s | $O(1)$ bulk arena release, Knuth 6.4R backward-shift compaction |
| **7** | Side-by-Side Dynamic Memory Trajectory | `03:45 - 04:20` | 35s | Memory-vs-AST step curves: 41.6 KB Baseline vs 16.6 KB Two-Tier |
| **8** | Empirical Benchmarks & Flight Controller | `04:20 - 04:55` | 35s | 8 workloads (-21.5% to -62.0%), Flight Controller small-scope rule |
| **9** | Architectural Trade-Offs & Conclusion | `04:55 - 05:20` | 25s | Budget cap survival (12 KB–24 KB), reproduction, closing remarks |

---

## Chapter 1: Problem Statement & Textbook Baseline (`00:00 - 00:35`)

**[VISUAL]**: Visualizer loads Chapter 1. The code view displays deeply nested lexical scopes. The metrics counters display Baseline memory inflating rapidly. The Hot table section illustrates heap node pointers and allocator header overhead.

**[NARRATION]**:
> "Welcome to the research demonstration of the Scope-Lifetime-Aware Two-Tier Compressed Symbol Table architecture.
>
> In compiler design and language servers, symbol tables have traditionally relied upon textbook chained hash tables or displays driven by general-purpose system allocators. While simple to implement, this textbook design imposes severe memory inflation: an ordinary 64-bit symbol struct requires 48 to 64 bytes of raw fields, but inflates to 160 to 200 bytes per loaded symbol once bucket pointer arrays, heap allocation headers, and duplicate string copies are accounted for.
>
> Furthermore, exiting a lexical scope mandates an $O(N)$ linked list traversal to invoke `free()` on every individual symbol. In deeply nested embedded codebases or multi-translation-unit language servers, this textbook architecture triggers catastrophic heap fragmentation and Out-Of-Memory aborts."

---

## Chapter 2: Two-Tier Architecture & Scope Temporal Locality (`00:35 - 01:10`)

**[VISUAL]**: Visualizer transitions to Chapter 2. The architecture diagram highlights the Hot Tier in deep blue (`#0072B2`) holding $K=2$ innermost scopes, and the Cold Tier in bluish green (`#009E73`) containing compressed blocks.

**[NARRATION]**:
> "Our core architectural insight is that block-structured programming languages exhibit profound temporal and spatial locality in lexical scope access. Profiling compiler front-ends reveals that the currently active innermost scope and its immediate enclosing parent account for over 88% to 92% of all symbol lookup queries.
>
> Ancestor scopes higher up the lexical dominance tree remain alive purely for variable resolution, yet textbook compilers leave them uncompressed and scattered across the heap.
>
> To exploit this locality invariant, our Two-Tier architecture bifurcates symbol storage: an uncompressed, high-throughput Hot Tier residing in an open-addressing table holds the innermost $K$ scopes—where $K=2$ by default—while dormant ancestor scopes are compressed into read-only, cache-guarded Cold Tier blocks."

---

## Chapter 3: 64-bit Packed Symbol Bitfield Architecture (`01:10 - 01:45`)

**[VISUAL]**: The camera focuses on the 64-bit Packed Symbol Bitfield visualizer. Each slice of the 8-byte record illuminates: `name_id` (16 bits), `kind` (4 bits), `storage` (4 bits), `scope_depth` (8 bits), `type_id` (16 bits), and `offset` (16 bits).

**[NARRATION]**:
> "In the Hot Tier, textbook 64-byte structs are replaced with a dense 64-bit packed bitfield occupying exactly 8 contiguous bytes.
>
> By utilizing a canonical string interning pool, variable names are replaced with a 16-bit identifier ID. Four bits specify the symbol kind—such as variable, function, or struct; four bits define the storage class; 8 bits encode lexical scope depth, supporting up to 256 nesting levels; 16 bits encode the structural type ID from our type-consing table; and 16 bits preserve the stack frame offset.
>
> Because every record is exactly 8 bytes, eight entire symbol entries fit within a single 64-byte CPU cache line. A sequential linear probe traverses up to eight symbols without incurring a single additional main memory access."

---

## Chapter 4: Live Scope Entry & Delta + Varint Demotion (`01:45 - 02:30`)

**[VISUAL]**: The visualizer steps through `arduino_sensors.mc`. The code stepper descends through Scope 0, Scope 1, Scope 2, and into Scope 3. As depth exceeds $K=2$, Scope 0 is automatically demoted: symbols are sorted by ID, deltas are computed, and a 64-bit Bloom filter is populated.

**[NARRATION]**:
> "Let us observe the live compiler simulation as Mini-C processes an Arduino sensor logger.
>
> As scopes 0, 1, and 2 are entered, symbols are interned and inserted directly into the Hot open-addressing table. But when lexical depth reaches Scope 3—exceeding our hot depth threshold of $K=2$—the compiler automatically demotes dormant ancestor Scope 0 to the Cold Tier.
>
> Demotion executes in four mathematical stages: first, symbols are sorted monotonically by interned name ID; second, subsequent identifiers are delta-subtracted against their predecessor; third, deltas, kinds, and offsets are compressed using LEB128 variable-length integers and zigzag signed encoding, packing each symbol into 1 to 2 bytes; and fourth, a 64-bit Bloom filter and orthogonal min/max bounding box are generated to guard the block. Scope 0's uncompressed arena memory is immediately reclaimed."

---

## Chapter 5: Multi-Tier Identifier Lookup Walkthrough (`02:30 - 03:10`)

**[VISUAL]**: The interactive lookup tracer tests identifier `filtered`. The query checks the Hot table, finds a match at slot 3, and resolves in 38.6 ns. Next, an absent identifier is queried: the Bloom filter bits mismatch, skipping the cold block with 0 decompressions.

**[NARRATION]**:
> "Now let us examine how the two-tier hierarchy resolves queries during semantic analysis.
>
> When the compiler resolves the local identifier `filtered`, it first hashes into the open-addressing Hot table. Using linear probing, active inner symbols resolve in an empirical 38.6 nanoseconds—faster than baseline linked list traversals.
>
> If a symbol is absent from the Hot table, the query probes cold blocks from innermost to outermost. Before initiating any decompression, the query checks the block's min/max ID bounds. If within bounds, it tests the 64-bit Bloom filter using three independent hash functions.
>
> If the Bloom filter returns zero, the entire block is skipped with zero decompression! In our benchmarks, this filter rejects over 97% of cold probes for absent identifiers. If a Bloom hit occurs, our 2-slot LRU cache provides decoded symbols in scratch memory without re-running decompression."

---

## Chapter 6: Scope Exit, Arena Reclaim & Knuth 6.4R Compaction (`03:10 - 03:45`)

**[VISUAL]**: The stepper triggers scope exit on Scope 5 and Scope 4. The scoped arena chunk offset resets instantly to 0. In the Hot table, Knuth 6.4R backward-shift compaction removes exited slots in-place.

**[NARRATION]**:
> "When an inner block terminates, traditional compilers suffer an $O(N)$ teardown penalty, traversing pointers to call `free()` on every individual node.
>
> In Two-Tier, active scope symbols reside in a lightweight per-scope chunk arena. Exiting a scope reclaims all local symbol storage in $O(1)$ time simply by resetting the arena allocation pointer, completely eliminating allocator overhead.
>
> To clean the open-addressing Hot table, we implement Donald Knuth's classic Algorithm 6.4R in-place backward-shift cluster compaction. Exited slots are cleared, and subsequent cluster elements are shifted backward into vacant slots without allocating a single tombstone. This guarantees that probe chain lengths remain strictly bounded under a 0.70 load factor."

---

## Chapter 7: Side-by-Side Dynamic Memory Trajectory (`03:45 - 04:20`)

**[VISUAL]**: The visualizer displays Figure 7 (Memory-vs-AST step trace). The Baseline curve climbs monotonically to 41,619 bytes. The Two-Tier curve climbs initially, then flattens at 16,566 bytes (-60.2% RAM reduction).

**[NARRATION]**:
> "Here we inspect the dynamic memory trajectory across AST processing steps on our synthetic deep nesting benchmark.
>
> Notice the stark divergence: textbook Baseline memory climbs monotonically to 41,619 bytes as scope levels accumulate in the heap. In contrast, Two-Tier immediately compresses dormant ancestors into cold blocks and reclaims exited scopes via its arena, capping peak symbol table memory at 16,566 bytes.
>
> This yields an empirical 60.2% RAM reduction on deep nesting and a 62.0% reduction on heavy shadowing, with the symbol table occupying only 1% to 11% of the compiler process resident set."

---

## Chapter 8: Empirical Benchmarks & Flight Controller (`04:20 - 04:55`)

**[VISUAL]**: The visualizer displays the verified empirical results table and Figure 13 (Flight Controller breakdown). The narration highlights the 21.5% to 62.0% savings across seven workloads, and contrasts eager vs. size-aware demotion on the Flight Controller.

**[NARRATION]**:
> "Academic rigor requires transparency regarding counterexamples.
>
> Across eight evaluated codebases, Two-Tier cuts peak RAM by 21.5% to 62.0% on seven workloads. However, the UAV Flight Controller firmware presents an instructive counterexample: it contains 67 tiny scopes with an average of only 2.4 symbols per scope.
>
> Under eager depth-bounded demotion, creating 39 compressed blocks incurs more in metadata headers (compressed block struct, Bloom filter, arena chunk) than the varint encoding saves, causing memory to inflate by 23.9% over Baseline.
>
> To resolve this, we introduced a size-aware policy: scopes holding fewer than 16 symbols remain in the arena and are demoted only if they hold sufficient data. This simple adaptive rule flips the Flight Controller from a 23.9% loss to an 8.9% memory saving over Baseline."

---

## Chapter 9: Architectural Trade-Offs & Conclusion (`04:55 - 05:20`)

**[VISUAL]**: The visualizer displays Figure 10 (Budget Cap Heatmap: 4 KB to 64 KB). Two-Tier survives 12 KB, 16 KB, and 24 KB limits where Baseline aborts with Out-Of-Memory. The final summary slide presents the open-source repository link.

**[NARRATION]**:
> "In summary, the Scope-Lifetime-Aware Two-Tier Symbol Table proves that compiler memory inflation is an avoidable consequence of textbook implementations.
>
> While Two-Tier incurs an insertion latency trade-off due to string interning and a 239 nanosecond decompression cost on cold misses, it accelerates hot queries, eliminates over 97% of cold lookups via Bloom filtering, and survives strict 12 KB to 24 KB physical RAM caps where textbook compilers crash with Out-Of-Memory aborts.
>
> The complete Mini-C compiler, 13-experiment benchmark suite, interactive visualizer, and IEEE manuscript are fully available in our open-source repository. Thank you."
