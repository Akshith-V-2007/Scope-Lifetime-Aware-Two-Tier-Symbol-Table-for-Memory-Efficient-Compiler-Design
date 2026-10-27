# Problem Statement: Scope-Lifetime-Aware Two-Tier Symbol Table

## 1. Context and Motivation
In modern compilers, language servers (Language Server Protocol / LSP), and memory-constrained embedded toolchains (e.g., GCC/LLVM cross-compilers running on microcontrollers or constrained build agents), the symbol table is one of the most heavily exercised data structures. Traditional compiler symbol tables maintain a linked stack of hash tables or chained bucket lists allocated using standard heap allocators (`malloc`/`free`). 

While conceptually simple, this textbook architecture imposes severe memory penalties:
- **Excessive Pointer and Heap Overhead**: Standard 64-bit nodes require 24–48 bytes of heap metadata, bucket pointers, and string pointers per symbol, even when the actual payload is merely 4 to 8 bytes. With 64-entry bucket arrays per scope, baseline symbol tables incur significant structural overhead.
- **Memory Fragmentation**: Thousands of individual small allocations (`malloc`) degrade cache locality and bloat virtual memory footprints.
- **Ignorance of Temporal Locality**: Scopes follow a strict stack discipline. Deep enclosing scopes are rarely queried compared to the innermost active scope, yet they consume identical uncompressed RAM.
- **Costly Scope Tear-Down**: Exiting a scope requires traversing linked lists or bucket arrays to `free()` individual entries, incurring $O(N)$ allocator overhead.

## 2. Core Insight: Scope-Lifetime Awareness
Symbols exhibit distinct lifecycle phases:
1. **Active Phase (Innermost Scopes)**: Actively declared and looked up during lexical analysis and expression parsing. These require $O(1)$ low-latency insertion and retrieval.
2. **Dormant Phase (Enclosing Ancestor Scopes)**: Scopes higher up the stack remain alive but are queried with low probability.
3. **Termination Phase (Closed Scopes)**: When a block or function scope ends, all local declarations become unreachable and can be discarded simultaneously.

By exploiting this lifetime structure, we organize symbol storage into two tiers:
- A **Hot Tier** holding the active innermost $K$ scopes in an open-addressing hash table (32 initial slots, linear probing with backward-shift deletion) for maximum throughput.
- A **Cold Tier** where deeper ancestor scopes are compressed into compact byte arrays using delta encoding, variable-length integers (LEB128/zigzag varints), and packed 64-bit bitfield symbol records.
- **Bloom Filters & Range Checks** attached to cold blocks to short-circuit lookups without decompressing scopes that do not contain the target identifier.
- **Per-Scope Arena Allocators** enabling bulk deallocation upon scope exit without visiting individual symbols ($O(1)$ arena reset plus an $O(M)$ hot-table sweep over hot slots).

## 3. Engineering Goals vs. Evaluated Results Audit

| Metric / Goal | Initial Target | Empirically Measured Result | Status | Engineering Analysis & Explanation |
| :--- | :--- | :--- | :---: | :--- |
| **Peak RAM Reduction** | $\ge 60\%$ reduction | **Synthetic**: 21.8% (broad scopes), 58.8% (deep nesting), 59.9% (heavy shadowing).<br>**Real-world**: 21.7% – 35.5% on small files; **+47.4% (worse)** on Flight Controller | **Partially Met** | Met on deep nesting & shadowing stress. On small realistic files (24–35 symbols), savings reach 21.7–35.5%. Flight controller (39 scopes, ~4 syms/scope) incurs +3.6 KB net overhead (+47.4%) due to 39 compressed block structures (min/max range, Bloom filter, length fields). |
| **Bytes per Symbol** | 8–14 B (uncompressed packed record) | **82.7 B – 105.1 B** loaded system footprint;<br>**8.0 B** raw packed record | **Partially Met** | Raw packed record meets the 8-byte target (64 bits). Loaded system footprint (82.7 B) includes string pool chunks, scope structures, and open-addressing hot slots. Though missing the 8–14 B loaded target, it reduces baseline loaded memory (200.9 B) by 58.8%. |
| **Lookup Latency Overhead** | $\le 1.8\times$ baseline | **Hot Hit**: 0.86x (faster, 36.9 ns vs 42.9 ns)<br>**Cold Cache-Warm**: 0.41x (faster, 35.8 ns vs 87.6 ns)<br>**Cold Forced Decomp**: 4.92x (slower, 221.5 ns vs 45.0 ns)<br>**Miss**: 0.47x (faster, 34.8 ns vs 73.7 ns)<br>**Insertion**: 1.86x (slower, 233.8 ns vs 126.0 ns)<br>**Whole-Run**: 0.47x (deep nesting) to 1.41x (flight controller) | **Partially Met / Qualified** | Hot lookups and negative misses are faster than baseline. Forced decompression across cold blocks incurs a ~4.9x penalty (mitigated by LRU caching). Insertion is ~1.86x slower due to upfront interning and packing. Timings represent medians over 15 repeated runs. |
| **Bloom & Range Rejection** | $\ge 85\%$ of cold negative lookups skipped | **94.8%** probes skipped (379 Bloom skips out of 400 probes) | **Met** | Combined min/max `name_id` range pruning and 64-bit Bloom filter effectively eliminates cold block decompressions for misses. |
| **Scope Exit Cost** | $O(1)$ symbol memory reclaim | **0.50x** of baseline time (600 ns vs 1200 ns for 10 symbols) | **Met** | Reclaims symbol memory via $O(1)$ arena free, accompanied by an $O(M)$ sweep of the 32-slot hot table ($M \ll N$). Avoids hundreds of individual `free()` system allocator calls. |
| **Correctness & Equivalence** | 100% Differential Identity | **100.0%** match over 6,000 differential ops, edge cases, and 30 randomized fuzzer ASTs | **Met** | Bitwise and semantic identity verified across exit codes and textual symbol resolution traces (`--dump-resolutions`). |
| **Memory-Cap Resilience** | Survives strict RAM budgets | **Survives 12 KB, 16 KB, 24 KB caps** where Baseline fails with OOM | **Met** | Dynamic depth demotion and compact representation compile successfully within 9,644 B while Baseline requires >24 KB. |

## 4. Scope and Source Language Specification (Mini-C)
The implementation compiles a practical subset of C designated **Mini-C**:
- **Primitive Types**: `int` (32-bit signed), `float` (32-bit IEEE 754), `char` (8-bit ASCII), `void`
- **User-Defined Types**: Struct definitions (`struct Point { int x; int y; };`)
- **Composite Types**: 1D Arrays (`int arr[10];`)
- **Expressions & Operators**:
  - Arithmetic: `+`, `-`, `*`, `/`
  - Unary minus: `-x` (handles negative literals and expressions)
  - Relational: `<`, `>`, `<=`, `>=`, `==`, `!=`
  - Assignment: `=`
- **Control Flow & Blocks**:
  - Compound statements: Arbitrarily nested blocks (`{ ... }`)
  - Selection: `if (cond) { ... } else { ... }`
  - Iteration: `while (cond) { ... }`
- **Functions**: Function declarations with parameter lists, function calls, and `return` statements
- **Explicit Grammar & Semantic Exclusions**:
  - *Multi-dimensional arrays* (e.g., `int matrix[10][10];`) are excluded; 1D indexing only.
  - *Pointers and indirection* (e.g., `*ptr`, `&val`, `->`) are excluded from grammar AST generation.
  - *Logical NOT* (`!`) and `for` loops are excluded; use `while` constructs.
  - *Separate function prototypes* (forward declarations without bodies) are not implemented; functions are declared at definition.
  - *Type checking limitation*: Type checking currently does not validate argument counts, `void` variable declarations, struct fields, or pointer-from-float assignments; semantic analysis exercises symbol table scoping, shadowing, and resolution, but full static type checking is beyond the scope of this project.
