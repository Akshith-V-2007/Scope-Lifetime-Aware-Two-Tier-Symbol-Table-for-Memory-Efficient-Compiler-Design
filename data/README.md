# Benchmark Workloads and Provenance

This directory contains the test workloads used to evaluate the Scope-Lifetime-Aware Two-Tier Symbol Table against alternative symbol-table architectures.

## Origin and Methodology

Each `.mc` file is formatted for the Mini-C compiler front-end. Preprocessor directives (`#include`, `#define`, `#pragma`) have been stripped, while preserving exact symbol declarations, scoping structures, pointer indirection, and lexical nesting.

| Workload File | Category | Source / Provenance | Description | Lines of Code |
| :--- | :--- | :--- | :--- | :--- |
| `arduino_sensors.mc` | Embedded (Micro) | Translated & simplified | Arduino sensor telemetry polling and circular buffer averaging. | 58 |
| `freertos_tasks.mc` | Embedded (Micro) | Translated & simplified | FreeRTOS task control block (TCB) scheduler priority scan. | 79 |
| `mibench_telecom.mc` | Embedded (Micro) | Translated & simplified | MiBench telecommunications CRC and packet checksum computation. | 65 |
| `zephyr_network.mc` | Embedded (Micro) | Translated & simplified | Zephyr RTOS net_pkt buffer allocation and packet header parsing. | 68 |
| `embedded_flight_controller.mc` | Embedded (Full) | Realistic domain model | UAV flight control system with AHRS, sensor fusion, PID loops, and failsafe state machines. | 416 |
| `synthetic_broad_scope.mc` | Synthetic | Programmatic stress | Shallow lexical depth (depth 2–3) with 1,000+ local identifiers per scope. | 545 |
| `synthetic_deep_nesting.mc` | Synthetic | Programmatic stress | Deeply nested lexical hierarchy (depth 32) testing cold-tier demotion. | 1,824 |
| `synthetic_heavy_shadowing.mc`| Synthetic | Programmatic stress | High identifier reuse and inner-scope shadowing testing scope resolution. | 760 |
| `freertos_queue.mc` | Real Firmware (Full) | FreeRTOS v10.4.3 (`queue.c`) | Real FreeRTOS inter-task communication queue with locks, items, pointers, and wait lists. | 185 |
| `mibench_qsort.mc` | Real Benchmark | MiBench Automotive (`qsort_small.c`) | 3D coordinate sorting via recursive quicksort with insertion sort threshold and partition scopes. | 165 |
| `mibench_bitcount.mc` | Real Benchmark | MiBench Telecomm (`bitcnts.c`) | Multi-algorithm bitwise counting routines (sparse Kernighan, parallel masks, bitstream evaluation). | 145 |
| `zephyr_synopsys.mc` | Real Firmware (Full) | Zephyr RTOS (`subsys/kernel/sched.c`) | Microkernel thread dispatcher, priority bitmask resolution, time-slice preemption, and context switching. | 155 |

## Scoping Characteristics

- **Synthetic Workloads**: Explicitly isolate architectural axes: depth (demotion trigger), breadth (table occupancy), and shadowing (identifier disambiguation).
- **Micro-Embedded Workloads**: Represent resource-constrained micro-controller code with small working sets (58–79 LOC).
- **Full Embedded & Real Workloads**: Represent commercial RTOS distributions and standard embedded benchmarks with complex nested blocks, multi-tier loops, pointer declarators, and multidimensional arrays.
