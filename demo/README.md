# Architectural Demonstration Video & Interactive Visualizer

This directory contains the timed presentation script, automated 1080p chapter screenshots, and playback instructions for the 5-minute video demonstration of the **Scope-Lifetime-Aware Two-Tier Symbol Table Architecture**.

---

## 1. Interactive Web Visualizer

The interactive visualizer provides an automated **Guided Tour mode (5 minutes 20 seconds)** with on-screen narration captions and chapter controls, plus an interactive **Free-Play mode** with live code stepping and identifier lookup tracing.

### Launching the Visualizer Locally:

On Windows:
```cmd
scripts\run_demo.bat
```

On Linux / macOS:
```bash
bash scripts/run_demo.sh
```

Then open your browser at:
```
http://localhost:8000/
```

- Click **"Guided Demo (5m Tour)"** to start the continuous timed walkthrough.
- Toggle **"Recording Mode"** in the top navigation bar to hide developer controls and display a clean, presentation-ready 1080p interface with on-screen narration captions and progress timer.
- Press **"Light" / "Dark"** to switch color themes (both conform strictly to WCAG AA contrast guidelines).

---

## 2. Timed Narration Script

The complete voiceover and on-screen caption script is documented in [`demo/SCRIPT.md`](SCRIPT.md).

Total Tour Runtime: **05:20** (320 seconds) at 1.0x playback speed.

---

## 3. Demonstration Video File Notice

In compliance with repository size limits (< 50 MB threshold), raw multi-gigabyte lossless recordings are hosted via external release storage. The full 1080p presentation recording is available at:

- **Full Demonstration Video**: [https://github.com/Akshith-V-2007/Scope-Lifetime-Aware-Two-Tier-Symbol-Table-for-Memory-Efficient-Compiler-Design/releases/tag/v2.2-demo](https://github.com/Akshith-V-2007/Scope-Lifetime-Aware-Two-Tier-Symbol-Table-for-Memory-Efficient-Compiler-Design/releases/tag/v2.2-demo)

---

## 4. Visualizer Chapter Snapshot Gallery (1080p Screenshots)

Automated headless Chrome snapshots for each chapter are preserved in `demo/screenshots/`:

| Chapter | Title | Timestamp | Snapshot File |
| :---: | :--- | :---: | :--- |
| **1** | Problem Statement & Textbook Baseline | `00:00 - 00:35` | [`screenshots/chapter_01_problem_baseline.png`](screenshots/chapter_01_problem_baseline.png) |
| **2** | Two-Tier Architecture & Temporal Locality | `00:35 - 01:10` | [`screenshots/chapter_02_twotier_overview.png`](screenshots/chapter_02_twotier_overview.png) |
| **3** | 64-bit Packed Symbol Bitfield Architecture | `01:10 - 01:45` | [`screenshots/chapter_03_packed_64bit_record.png`](screenshots/chapter_03_packed_64bit_record.png) |
| **4** | Live Scope Entry & Delta + Varint Demotion | `01:45 - 02:30` | [`screenshots/chapter_04_live_demotion_varint.png`](screenshots/chapter_04_live_demotion_varint.png) |
| **5** | Multi-Tier Identifier Lookup Walkthrough | `02:30 - 03:10` | [`screenshots/chapter_05_multitier_lookup.png`](screenshots/chapter_05_multitier_lookup.png) |
| **6** | Scope Exit, Arena Reclaim & Knuth 6.4R | `03:10 - 03:45` | [`screenshots/chapter_06_scope_exit_arena.png`](screenshots/chapter_06_scope_exit_arena.png) |
| **7** | Side-by-Side Dynamic Memory Trajectory | `03:45 - 04:20` | [`screenshots/chapter_07_memory_trace_comparison.png`](screenshots/chapter_07_memory_trace_comparison.png) |
| **8** | Empirical Benchmarks & Flight Controller | `04:20 - 04:55` | [`screenshots/chapter_08_flight_controller_results.png`](screenshots/chapter_08_flight_controller_results.png) |
| **9** | Architectural Trade-Offs & Conclusion | `04:55 - 05:20` | [`screenshots/chapter_09_limitations_conclusion.png`](screenshots/chapter_09_limitations_conclusion.png) |
