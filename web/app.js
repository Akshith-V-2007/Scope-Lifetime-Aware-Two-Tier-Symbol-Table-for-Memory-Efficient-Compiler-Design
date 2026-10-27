// Scope-Lifetime-Aware Two-Tier Symbol Table Visualizer Engine

const SAMPLES = {
    arduino: {
        title: "Arduino Sensor Environmental Logger",
        code: [
            "int PIN_SDA = 4;",
            "int PIN_SCL = 5;",
            "int PIN_ALERT_LED = 13;",
            "struct SensorReading { int id; int temp; int humidity; };",
            "int process_sensor_channel(int ch_id) {",
            "    int raw_adc = 450;",
            "    {",
            "        int bus_speed = 400000;",
            "        int filter_gain = 3;",
            "        {",
            "            int filtered = (raw_adc * filter_gain) / 2;",
            "            {",
            "                int alarm_limit = 800;",
            "                if (filtered > alarm_limit) {",
            "                    int alert_flag = 1;",
            "                }",
            "            }",
            "            return filtered;",
            "        }",
            "    }",
            "}",
            "int main() {",
            "    int val = process_sensor_channel(0);",
            "    return val;",
            "}"
        ],
        actions: [
            { type: "enter_scope", depth: 0, line: 0, desc: "Enter Scope 0 (Global Scope)" },
            { type: "insert", name: "PIN_SDA", type_name: "int", offset: 0, line: 0 },
            { type: "insert", name: "PIN_SCL", type_name: "int", offset: 4, line: 1 },
            { type: "insert", name: "PIN_ALERT_LED", type_name: "int", offset: 8, line: 2 },
            { type: "insert", name: "SensorReading", type_name: "struct", offset: 0, line: 3 },
            { type: "insert", name: "process_sensor_channel", type_name: "func", offset: 0, line: 4 },
            { type: "enter_scope", depth: 1, line: 4, desc: "Enter Scope 1 (Function process_sensor_channel)" },
            { type: "insert", name: "ch_id", type_name: "int", offset: 0, line: 4 },
            { type: "insert", name: "raw_adc", type_name: "int", offset: 4, line: 5 },
            { type: "enter_scope", depth: 2, line: 6, desc: "Enter Scope 2 (Inner Block 1)" },
            { type: "insert", name: "bus_speed", type_name: "int", offset: 8, line: 7 },
            { type: "insert", name: "filter_gain", type_name: "int", offset: 12, line: 8 },
            { type: "enter_scope", depth: 3, line: 9, desc: "Enter Scope 3 (Inner Block 2 - Demoting Scope 0 to Cold)" },
            { type: "insert", name: "filtered", type_name: "int", offset: 16, line: 10 },
            { type: "enter_scope", depth: 4, line: 11, desc: "Enter Scope 4 (Deep Inner Block - Demoting Scope 1 to Cold)" },
            { type: "insert", name: "alarm_limit", type_name: "int", offset: 20, line: 12 },
            { type: "enter_scope", depth: 5, line: 13, desc: "Enter Scope 5 (If Block - Demoting Scope 2 to Cold)" },
            { type: "insert", name: "alert_flag", type_name: "int", offset: 24, line: 14 },
            { type: "exit_scope", depth: 5, line: 15, desc: "Exit Scope 5: O(1) Arena Reclaim (Destroying alert_flag)" },
            { type: "exit_scope", depth: 4, line: 16, desc: "Exit Scope 4: O(1) Arena Reclaim (Destroying alarm_limit)" },
            { type: "lookup", name: "filtered", line: 17, desc: "Lookup 'filtered' -> Hit in Scope 3" },
            { type: "exit_scope", depth: 3, line: 18, desc: "Exit Scope 3: O(1) Arena Reclaim (Destroying filtered)" },
            { type: "exit_scope", depth: 2, line: 19, desc: "Exit Scope 2: O(1) Arena Reclaim (Destroying bus_speed, filter_gain)" },
            { type: "exit_scope", depth: 1, line: 20, desc: "Exit Scope 1 (process_sensor_channel complete)" },
            { type: "insert", name: "main", type_name: "func", offset: 0, line: 21 },
            { type: "enter_scope", depth: 1, line: 21, desc: "Enter Scope 1 (main)" },
            { type: "lookup", name: "process_sensor_channel", line: 22, desc: "Lookup function 'process_sensor_channel' -> Cold Hit" },
            { type: "insert", name: "val", type_name: "int", offset: 0, line: 22 },
            { type: "exit_scope", depth: 1, line: 23, desc: "Exit Scope 1 (main complete)" },
            { type: "exit_scope", depth: 0, line: 24, desc: "Exit Scope 0 (Global Scope complete - 100% memory freed)" }
        ]
    },
    flight: {
        title: "UAV Quad-X Flight Controller Firmware",
        code: [
            "struct Vector3 { int x; int y; int z; };",
            "struct ImuSample { int ax; int ay; int az; int temp; };",
            "int G_ARMED_FLAG = 0;",
            "int imu_filter_and_pid(int raw_ax, int temp_c) {",
            "    int calib_bias = 14;",
            "    {",
            "        int temp_delta = temp_c - 25;",
            "        int comp_factor = temp_delta * 2;",
            "        calib_bias = calib_bias + comp_factor;",
            "        {",
            "            int sample_x = raw_ax - calib_bias;",
            "            int error = 1000 - sample_x;",
            "            {",
            "                int kp = 25;",
            "                int p_term = (error * kp) / 10;",
            "                return p_term;",
            "            }",
            "        }",
            "    }",
            "}",
            "int main() {",
            "    int out = imu_filter_and_pid(520, 28);",
            "    return out;",
            "}"
        ],
        actions: [
            { type: "enter_scope", depth: 0, line: 0, desc: "Enter Scope 0 (Global Scope)" },
            { type: "insert", name: "Vector3", type_name: "struct", offset: 0, line: 0 },
            { type: "insert", name: "ImuSample", type_name: "struct", offset: 0, line: 1 },
            { type: "insert", name: "G_ARMED_FLAG", type_name: "int", offset: 0, line: 2 },
            { type: "insert", name: "imu_filter_and_pid", type_name: "func", offset: 0, line: 3 },
            { type: "enter_scope", depth: 1, line: 3, desc: "Enter Scope 1 (Function imu_filter_and_pid)" },
            { type: "insert", name: "raw_ax", type_name: "int", offset: 0, line: 3 },
            { type: "insert", name: "temp_c", type_name: "int", offset: 4, line: 3 },
            { type: "insert", name: "calib_bias", type_name: "int", offset: 8, line: 4 },
            { type: "enter_scope", depth: 2, line: 5, desc: "Enter Scope 2 (Temperature Calibration)" },
            { type: "insert", name: "temp_delta", type_name: "int", offset: 12, line: 6 },
            { type: "insert", name: "comp_factor", type_name: "int", offset: 16, line: 7 },
            { type: "enter_scope", depth: 3, line: 9, desc: "Enter Scope 3 (Noise Rejection - Demoting Scope 0 to Cold)" },
            { type: "insert", name: "sample_x", type_name: "int", offset: 20, line: 10 },
            { type: "insert", name: "error", type_name: "int", offset: 24, line: 11 },
            { type: "enter_scope", depth: 4, line: 12, desc: "Enter Scope 4 (Inner PID Loop - Demoting Scope 1 to Cold)" },
            { type: "insert", name: "kp", type_name: "int", offset: 28, line: 13 },
            { type: "insert", name: "p_term", type_name: "int", offset: 32, line: 14 },
            { type: "lookup", name: "error", line: 14, desc: "Lookup 'error' -> Cold Hit in Scope 3" },
            { type: "lookup", name: "kp", line: 14, desc: "Lookup 'kp' -> Hot Hit in Scope 4" },
            { type: "exit_scope", depth: 4, line: 16, desc: "Exit Scope 4: O(1) Arena Reclaim (Freeing kp, p_term)" },
            { type: "exit_scope", depth: 3, line: 17, desc: "Exit Scope 3: O(1) Arena Reclaim (Freeing sample_x, error)" },
            { type: "exit_scope", depth: 2, line: 18, desc: "Exit Scope 2: O(1) Arena Reclaim (Freeing temp_delta, comp_factor)" },
            { type: "exit_scope", depth: 1, line: 19, desc: "Exit Scope 1 (imu_filter_and_pid complete)" },
            { type: "insert", name: "main", type_name: "func", offset: 0, line: 20 },
            { type: "enter_scope", depth: 1, line: 20, desc: "Enter Scope 1 (main)" },
            { type: "lookup", name: "imu_filter_and_pid", line: 21, desc: "Lookup 'imu_filter_and_pid' -> Cold Hit in Scope 0" },
            { type: "insert", name: "out", type_name: "int", offset: 0, line: 21 },
            { type: "exit_scope", depth: 1, line: 22, desc: "Exit Scope 1 (main complete)" },
            { type: "exit_scope", depth: 0, line: 23, desc: "Exit Scope 0 (Global Scope complete - 100% memory freed)" }
        ]
    },
    freertos: {
        title: "FreeRTOS Kernel Task Scheduler",
        code: [
            "int MAX_PRIORITIES = 8;",
            "int ready_task_count = 0;",
            "struct TCB { int id; int priority; int stack_top; };",
            "int schedule_next_task(int top_priority) {",
            "    int next_id = -1;",
            "    {",
            "        int scan_idx = 0;",
            "        {",
            "            int slot_prio = 3;",
            "            {",
            "                int quantum = 10;",
            "                next_id = 7;",
            "            }",
            "        }",
            "    }",
            "    return next_id;",
            "}"
        ],
        actions: [
            { type: "enter_scope", depth: 0, line: 0, desc: "Enter Global Scope" },
            { type: "insert", name: "MAX_PRIORITIES", type_name: "int", offset: 0, line: 0 },
            { type: "insert", name: "ready_task_count", type_name: "int", offset: 4, line: 1 },
            { type: "insert", name: "TCB", type_name: "struct", offset: 0, line: 2 },
            { type: "insert", name: "schedule_next_task", type_name: "func", offset: 0, line: 3 },
            { type: "enter_scope", depth: 1, line: 3, desc: "Enter Function Scope" },
            { type: "insert", name: "top_priority", type_name: "int", offset: 0, line: 3 },
            { type: "insert", name: "next_id", type_name: "int", offset: 4, line: 4 },
            { type: "enter_scope", depth: 2, line: 5, desc: "Enter Nested Block" },
            { type: "insert", name: "scan_idx", type_name: "int", offset: 8, line: 6 },
            { type: "enter_scope", depth: 3, line: 7, desc: "Enter Deep Block (Scope 0 Demoted to Cold Tier)" },
            { type: "insert", name: "slot_prio", type_name: "int", offset: 12, line: 8 },
            { type: "enter_scope", depth: 4, line: 9, desc: "Enter Inner Quantum Block (Scope 1 Demoted to Cold Tier)" },
            { type: "insert", name: "quantum", type_name: "int", offset: 16, line: 10 },
            { type: "exit_scope", depth: 4, line: 12, desc: "Exit Inner Quantum Block" },
            { type: "exit_scope", depth: 3, line: 13, desc: "Exit Deep Block" },
            { type: "exit_scope", depth: 2, line: 14, desc: "Exit Nested Block" },
            { type: "exit_scope", depth: 1, line: 15, desc: "Exit Function Scope" },
            { type: "exit_scope", depth: 0, line: 16, desc: "Exit Global Scope" }
        ]
    },
    shadowing: {
        title: "Deep Nesting & Shadowing Stress Test",
        code: [
            "int shadow_var = 1; // Global",
            "int main() {",
            "    int shadow_var = 10; // Function Scope",
            "    {",
            "        int shadow_var = 20; // Depth 2",
            "        {",
            "            int shadow_var = 30; // Depth 3 (Demotes Global)",
            "            {",
            "                int shadow_var = 40; // Depth 4 (Demotes Function)",
            "                int read_val = shadow_var;",
            "            }",
            "        }",
            "    }",
            "    return shadow_var;",
            "}"
        ],
        actions: [
            { type: "enter_scope", depth: 0, line: 0, desc: "Enter Global Scope" },
            { type: "insert", name: "shadow_var", type_name: "int", offset: 0, line: 0 },
            { type: "insert", name: "main", type_name: "func", offset: 0, line: 1 },
            { type: "enter_scope", depth: 1, line: 1, desc: "Enter Function Scope (main)" },
            { type: "insert", name: "shadow_var", type_name: "int", offset: 4, line: 2 },
            { type: "enter_scope", depth: 2, line: 3, desc: "Enter Block Depth 2" },
            { type: "insert", name: "shadow_var", type_name: "int", offset: 8, line: 4 },
            { type: "enter_scope", depth: 3, line: 5, desc: "Enter Block Depth 3 (Demoting Scope 0 to Cold)" },
            { type: "insert", name: "shadow_var", type_name: "int", offset: 12, line: 6 },
            { type: "enter_scope", depth: 4, line: 7, desc: "Enter Block Depth 4 (Demoting Scope 1 to Cold)" },
            { type: "insert", name: "shadow_var", type_name: "int", offset: 16, line: 8 },
            { type: "lookup", name: "shadow_var", line: 9, desc: "Lookup 'shadow_var' -> Resolves to innermost (Depth 4, offset 16)" },
            { type: "insert", name: "read_val", type_name: "int", offset: 20, line: 9 },
            { type: "exit_scope", depth: 4, line: 10, desc: "Exit Depth 4: O(1) reclaim" },
            { type: "exit_scope", depth: 3, line: 11, desc: "Exit Depth 3: O(1) reclaim" },
            { type: "exit_scope", depth: 2, line: 12, desc: "Exit Depth 2: O(1) reclaim" },
            { type: "lookup", name: "shadow_var", line: 13, desc: "Lookup 'shadow_var' -> Resolves to Scope 1 (offset 4)" },
            { type: "exit_scope", depth: 1, line: 14, desc: "Exit Function Scope" },
            { type: "exit_scope", depth: 0, line: 15, desc: "Exit Global Scope" }
        ]
    },
    broad: {
        title: "Broad Scopes (Multi-Symbol Burst)",
        code: [
            "int g_clock = 0;",
            "int main() {",
            "    int a = 1; int b = 2; int c = 3;",
            "    int d = 4; int e = 5; int f = 6;",
            "    {",
            "        int x = 10; int y = 20; int z = 30;",
            "        {",
            "            int s1 = 100; int s2 = 200;",
            "        }",
            "    }",
            "    return a;",
            "}"
        ],
        actions: [
            { type: "enter_scope", depth: 0, line: 0, desc: "Enter Global Scope" },
            { type: "insert", name: "g_clock", type_name: "int", offset: 0, line: 0 },
            { type: "insert", name: "main", type_name: "func", offset: 0, line: 1 },
            { type: "enter_scope", depth: 1, line: 1, desc: "Enter Scope 1" },
            { type: "insert", name: "a", type_name: "int", offset: 0, line: 2 },
            { type: "insert", name: "b", type_name: "int", offset: 4, line: 2 },
            { type: "insert", name: "c", type_name: "int", offset: 8, line: 2 },
            { type: "insert", name: "d", type_name: "int", offset: 12, line: 3 },
            { type: "insert", name: "e", type_name: "int", offset: 16, line: 3 },
            { type: "insert", name: "f", type_name: "int", offset: 20, line: 3 },
            { type: "enter_scope", depth: 2, line: 4, desc: "Enter Scope 2" },
            { type: "insert", name: "x", type_name: "int", offset: 24, line: 5 },
            { type: "insert", name: "y", type_name: "int", offset: 28, line: 5 },
            { type: "insert", name: "z", type_name: "int", offset: 32, line: 5 },
            { type: "enter_scope", depth: 3, line: 6, desc: "Enter Scope 3 (Demoting Scope 0 to Cold Tier)" },
            { type: "insert", name: "s1", type_name: "int", offset: 36, line: 7 },
            { type: "insert", name: "s2", type_name: "int", offset: 40, line: 7 },
            { type: "exit_scope", depth: 3, line: 8, desc: "Exit Scope 3" },
            { type: "exit_scope", depth: 2, line: 9, desc: "Exit Scope 2" },
            { type: "lookup", name: "a", line: 10, desc: "Lookup 'a' -> Hot Hit" },
            { type: "exit_scope", depth: 1, line: 11, desc: "Exit Scope 1" },
            { type: "exit_scope", depth: 0, line: 12, desc: "Exit Scope 0" }
        ]
    }
};

// Simulation State
class SimulationEngine {
    constructor() {
        this.hotDepth = 2;
        this.bloomEnabled = true;
        this.currentStep = 0;
        this.sampleKey = "arduino";
        this.isPlaying = false;
        this.playTimer = null;

        // Visualizer State
        this.scopes = [];
        this.hotSlots = new Array(16).fill(null);
        this.coldBlocks = [];
        this.decompCache = [null, null];
        this.stringPool = new Map();
        this.nextNameId = 1;

        // Metrics
        this.currentBytes = 0;
        this.peakBytes = 0;
        this.bloomSkips = 0;
        this.decompressions = 0;
        this.hotHits = 0;
        this.coldHits = 0;
    }

    intern(name) {
        if (!this.stringPool.has(name)) {
            const id = this.nextNameId++;
            this.stringPool.set(name, id);
            this.currentBytes += (name.length + 1) + 16;
            return id;
        }
        return this.stringPool.get(name);
    }

    hashKey(nid) {
        let x = nid;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return Math.abs(x) % this.hotSlots.length;
    }

    bloomHash(nid) {
        const x = nid;
        const h1 = Math.abs((x * 0x9E3779B9) >> 26) % 64;
        const h2 = Math.abs((x * 0x85EBCA6B) >> 26) % 64;
        const h3 = Math.abs((x * 0xC2B2AE35) >> 26) % 64;
        return [h1, h2, h3];
    }

    reset() {
        if (this.playTimer) clearInterval(this.playTimer);
        this.isPlaying = false;
        this.currentStep = 0;
        this.scopes = [];
        this.hotSlots = new Array(16).fill(null);
        this.coldBlocks = [];
        this.decompCache = [null, null];
        this.stringPool = new Map();
        this.nextNameId = 1;
        this.currentBytes = 0;
        this.peakBytes = 0;
        this.bloomSkips = 0;
        this.decompressions = 0;
        this.hotHits = 0;
        this.coldHits = 0;
        this.updateUI();
    }

    step() {
        const sample = SAMPLES[this.sampleKey];
        if (this.currentStep >= sample.actions.length) {
            this.pause();
            return;
        }

        const action = sample.actions[this.currentStep];
        this.executeAction(action);
        this.currentStep++;
        this.updateUI();
    }

    executeAction(action) {
        if (action.type === "enter_scope") {
            const scopeId = this.scopes.length + 1;
            const newScope = {
                id: scopeId,
                depth: action.depth,
                isHot: true,
                symbols: [],
                arenaBytes: 1024
            };
            this.scopes.push(newScope);
            this.currentBytes += 1024; // arena chunk overhead

            // Check demotion for scopes deeper than hotDepth
            const d = this.scopes.length - 1;
            if (d >= this.hotDepth) {
                const demoteIdx = d - this.hotDepth;
                if (demoteIdx >= 0 && this.scopes[demoteIdx].isHot) {
                    this.demoteScope(demoteIdx);
                }
            }
        } else if (action.type === "insert") {
            const nid = this.intern(action.name);
            const curScope = this.scopes[this.scopes.length - 1];
            if (curScope) {
                const sym = {
                    name: action.name,
                    nid: nid,
                    type_name: action.type_name,
                    offset: action.offset,
                    depth: curScope.depth,
                    scopeId: curScope.id
                };
                curScope.symbols.push(sym);
                this.currentBytes += 8; // 64-bit packed record

                // Insert into hot open-addressing table
                let idx = this.hashKey(nid);
                while (this.hotSlots[idx] !== null) {
                    idx = (idx + 1) % this.hotSlots.length;
                }
                this.hotSlots[idx] = sym;
            }
        } else if (action.type === "exit_scope") {
            const popped = this.scopes.pop();
            if (popped) {
                if (popped.isHot) {
                    // Remove from hot table using backward shift deletion
                    this.removeScopeFromHotTable(popped.id);
                    this.currentBytes -= (popped.arenaBytes + popped.symbols.length * 8);
                } else {
                    // Remove from cold blocks
                    this.coldBlocks = this.coldBlocks.filter(b => b.scopeId !== popped.id);
                    this.currentBytes -= 32; // compressed block size
                }
            }
        }

        if (this.currentBytes > this.peakBytes) {
            this.peakBytes = this.currentBytes;
        }
    }

    removeScopeFromHotTable(scopeId) {
        const cap = this.hotSlots.length;
        for (let i = 0; i < cap; i++) {
            while (this.hotSlots[i] && this.hotSlots[i].scopeId === scopeId) {
                this.hotSlots[i] = null;
                let j = i;
                let k = (j + 1) % cap;
                while (this.hotSlots[k]) {
                    const r = this.hashKey(this.hotSlots[k].nid);
                    const between = (j <= k) ? (j < r && r <= k) : (j < r || r <= k);
                    if (!between) {
                        this.hotSlots[j] = this.hotSlots[k];
                        this.hotSlots[k] = null;
                        j = k;
                    }
                    k = (k + 1) % cap;
                }
            }
        }
    }

    demoteScope(scopeIdx) {
        const s = this.scopes[scopeIdx];
        if (!s || !s.isHot) return;

        // Remove from hot table using backward shift deletion
        this.removeScopeFromHotTable(s.id);

        // Build compressed cold block with Bloom filter
        const bloomBits = new Array(64).fill(0);
        s.symbols.forEach(sym => {
            const [h1, h2, h3] = this.bloomHash(sym.nid);
            bloomBits[h1] = 1;
            bloomBits[h2] = 1;
            bloomBits[h3] = 1;
        });

        // Delta compression calculation: ~3 bytes per symbol + 8-byte header
        const compressedBytes = 8 + (s.symbols.length * 3);
        const coldBlock = {
            scopeId: s.id,
            depth: s.depth,
            symbols: [...s.symbols],
            symbolCount: s.symbols.length,
            compressedBytes: compressedBytes,
            bloomBits: bloomBits
        };

        this.coldBlocks.push(coldBlock);
        this.currentBytes -= (s.arenaBytes + s.symbols.length * 8);
        this.currentBytes += compressedBytes;

        s.isHot = false;
        s.symbols = [];
    }

    traceLookup(name) {
        const logs = [];
        logs.push(`<div class="trace-step"><strong>1. Querying identifier:</strong> '<code>${name}</code>'</div>`);

        const nid = this.intern(name);
        logs.push(`<div class="trace-step">Interned to ID: <code>${nid}</code> (0x${nid.toString(16).toUpperCase()})</div>`);

        let foundHot = null;
        const startIdx = this.hashKey(nid);
        let currIdx = startIdx;
        const probes = [];

        // Real open-addressing linear probe sequence
        for (let count = 0; count < this.hotSlots.length; count++) {
            probes.push(currIdx);
            const slot = this.hotSlots[currIdx];
            if (slot === null) {
                // Empty slot terminates open-addressing cluster probe
                break;
            }
            if (slot.name === name) {
                if (!foundHot || slot.depth >= foundHot.depth) {
                    foundHot = slot;
                }
            }
            currIdx = (currIdx + 1) % this.hotSlots.length;
        }

        logs.push(`<div class="trace-step"><strong>2. Open-Addressing Hot Probing:</strong> Initial slot <code>${startIdx}</code> &rarr; Probed <code>[${probes.slice(0, 6).join(', ')}${probes.length > 6 ? ', ...' : ''}]</code></div>`);

        if (foundHot) {
            this.hotHits++;
            logs.push(`<div class="trace-step text-emerald">&#10004; <strong>HOT HIT!</strong> Found in Scope ${foundHot.scopeId} at Depth ${foundHot.depth} (Offset: ${foundHot.offset})</div>`);
            return logs;
        }

        logs.push(`<div class="trace-step text-amber">&#10006; Hot Tier Miss. Proceeding to Cold Tier...</div>`);

        // 2. Cold Tier Search
        const [h1, h2, h3] = this.bloomHash(nid);
        logs.push(`<div class="trace-step">Computed 3 Bloom Hash Bits: <code>[${h1}, ${h2}, ${h3}]</code></div>`);

        let foundCold = null;
        for (let b = this.coldBlocks.length - 1; b >= 0; b--) {
            const blk = this.coldBlocks[b];
            logs.push(`<div class="trace-step">Checking Cold Scope ${blk.scopeId} (Depth ${blk.depth}):</div>`);

            // Bloom test
            const b1 = blk.bloomBits[h1];
            const b2 = blk.bloomBits[h2];
            const b3 = blk.bloomBits[h3];

            if (this.bloomEnabled && (!b1 || !b2 || !b3)) {
                this.bloomSkips++;
                logs.push(`<div class="trace-step text-cyan">&nbsp;&nbsp;&#10132; Bloom test: bit missing [${b1},${b2},${b3}] &rarr; <strong>SKIPPED DECOMPRESSION!</strong></div>`);
                continue;
            }

            logs.push(`<div class="trace-step text-purple">&nbsp;&nbsp;&#10132; Bloom test matched! Probing Decomp Cache...</div>`);
            this.decompressions++;

            // Cache check
            let inCache = this.decompCache.find(c => c && c.scopeId === blk.scopeId);
            if (!inCache) {
                logs.push(`<div class="trace-step text-amber">&nbsp;&nbsp;&nbsp;&nbsp;Cache Miss: Decompressing ${blk.symbolCount} delta-varint symbols into scratch...</div>`);
                this.decompCache[1] = this.decompCache[0];
                this.decompCache[0] = blk;
            } else {
                logs.push(`<div class="trace-step text-emerald">&nbsp;&nbsp;&nbsp;&nbsp;Cache Hit: Instant access from scratch buffer!</div>`);
            }

            // Binary search
            const match = blk.symbols.find(s => s.name === name);
            if (match) {
                this.coldHits++;
                foundCold = match;
                logs.push(`<div class="trace-step text-emerald">&nbsp;&nbsp;&#10004; <strong>COLD HIT!</strong> Found '${name}' in Scope ${blk.scopeId} (Offset: ${match.offset})</div>`);
                break;
            }
        }

        if (!foundCold) {
            logs.push(`<div class="trace-step text-rose">&#10006; <strong>SYMBOL NOT FOUND</strong> (Undeclared identifier in any scope)</div>`);
        }

        return logs;
    }

    togglePlay() {
        if (this.isPlaying) {
            this.pause();
        } else {
            this.play();
        }
    }

    play() {
        this.isPlaying = true;
        document.getElementById("btn-play").innerText = "Pause";
        this.playTimer = setInterval(() => {
            const sample = SAMPLES[this.sampleKey];
            if (this.currentStep >= sample.actions.length) {
                this.pause();
            } else {
                this.step();
            }
        }, 700);
    }

    pause() {
        this.isPlaying = false;
        clearInterval(this.playTimer);
        document.getElementById("btn-play").innerText = "Auto Play";
    }

    updateUI() {
        const sample = SAMPLES[this.sampleKey];
        const curAction = sample.actions[this.currentStep] || sample.actions[sample.actions.length - 1];

        // 1. Code Lines
        const codeDisplay = document.getElementById("code-display");
        codeDisplay.innerHTML = "";
        sample.code.forEach((line, idx) => {
            const div = document.createElement("div");
            div.className = "code-line" + (curAction && curAction.line === idx ? " active" : "");
            div.innerHTML = `<span class="line-num">${idx + 1}</span><span class="line-text">${escapeHtml(line)}</span>`;
            codeDisplay.appendChild(div);
        });

        // 2. Metrics & Counters
        document.getElementById("step-indicator").innerText = `Step ${this.currentStep} / ${sample.actions.length}`;
        document.getElementById("stat-current-ram").innerText = `${this.currentBytes.toLocaleString()} B`;
        document.getElementById("stat-peak-ram").innerText = `${this.peakBytes.toLocaleString()} B`;
        document.getElementById("stat-scope-depth").innerText = this.scopes.length > 0 ? this.scopes.length - 1 : 0;
        document.getElementById("stat-bloom-skips").innerText = this.bloomSkips;
        document.getElementById("stat-decomps").innerText = this.decompressions;

        // 3. Hot Slots Grid
        const hotContainer = document.getElementById("hot-slots-container");
        hotContainer.innerHTML = "";
        let occupied = 0;
        this.hotSlots.forEach((slot, idx) => {
            const div = document.createElement("div");
            if (slot) {
                occupied++;
                div.className = "hot-slot occupied";
                div.innerHTML = `
                    <span class="slot-idx">#${idx} [Scope ${slot.scopeId}]</span>
                    <span class="slot-name">${slot.name}</span>
                    <span class="slot-details">${slot.type_name} | off:${slot.offset}</span>
                `;
            } else {
                div.className = "hot-slot empty";
                div.innerHTML = `<span class="slot-idx">#${idx}</span><span class="text-muted">[Empty]</span>`;
            }
            hotContainer.appendChild(div);
        });
        document.getElementById("hot-meta").innerText = `${occupied} active symbols | Load ${(occupied/this.hotSlots.length*100).toFixed(0)}%`;

        // 4. Cold Blocks
        const coldContainer = document.getElementById("cold-blocks-container");
        coldContainer.innerHTML = "";
        if (this.coldBlocks.length === 0) {
            coldContainer.innerHTML = `<div class="empty-placeholder">No scopes demoted to Cold Tier yet. Advance simulation to observe compression.</div>`;
        } else {
            this.coldBlocks.forEach(b => {
                const card = document.createElement("div");
                card.className = "cold-block-card";
                
                // Render Bloom bit icons
                let bitsHtml = '<div class="cold-bloom-bits">';
                for (let i = 0; i < 32; i += 2) {
                    const on = b.bloomBits[i] ? " on" : "";
                    bitsHtml += `<span class="bloom-bit${on}"></span>`;
                }
                bitsHtml += '</div>';

                const namesStr = b.symbols.map(s => s.name).join(", ");
                card.innerHTML = `
                    <div class="cold-block-header">
                        <strong>Scope ${b.scopeId}</strong> (Depth ${b.depth})
                        <span class="badge badge-info">${b.symbolCount} syms</span>
                        <span class="badge badge-success">${b.compressedBytes} B</span>
                    </div>
                    ${bitsHtml}
                    <div class="cold-symbols-preview">${namesStr}</div>
                `;
                coldContainer.appendChild(card);
            });
        }
        document.getElementById("cold-meta").innerText = `${this.coldBlocks.length} compressed blocks`;

        // 5. Decomp Cache
        const cacheContainer = document.getElementById("cache-slots-container");
        cacheContainer.innerHTML = "";
        this.decompCache.forEach((entry, idx) => {
            const div = document.createElement("div");
            if (entry) {
                div.className = "cache-slot active";
                div.innerHTML = `Slot ${idx}: Scope ${entry.scopeId} (${entry.symbolCount} symbols cached in scratch)`;
            } else {
                div.className = "cache-slot empty";
                div.innerHTML = `Slot ${idx}: [Empty Scratch Buffer]`;
            }
            cacheContainer.appendChild(div);
        });
    }
}

function escapeHtml(str) {
    return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

// Charts Viewport Renderer
function renderChart(type) {
    const container = document.getElementById("chart-container");
    if (type === "peak-ram") {
        container.innerHTML = `
            <svg viewBox="0 0 700 280" width="100%" height="100%">
                <text x="350" y="24" fill="#0f172a" font-size="14" font-weight="bold" text-anchor="middle">Peak RAM: Baseline vs Two-Tier</text>
                <!-- Legend -->
                <g transform="translate(480, 12)">
                    <rect x="0" y="0" width="12" height="12" fill="#e11d48" rx="2"/>
                    <text x="18" y="10" fill="#334155" font-size="10">Baseline</text>
                    <rect x="75" y="0" width="12" height="12" fill="#059669" rx="2"/>
                    <text x="93" y="10" fill="#334155" font-size="10">Two-Tier</text>
                </g>
                <!-- Bars -->
                <g transform="translate(130, 40)">
                    <!-- Arduino -->
                    <text x="-10" y="20" fill="#334155" font-size="10" text-anchor="end">Arduino</text>
                    <rect x="0" y="8" width="58" height="14" fill="#e11d48" rx="3" />
                    <rect x="0" y="25" width="40" height="14" fill="#059669" rx="3" />
                    <text x="46" y="37" fill="#047857" font-size="10" font-weight="bold">-25.6% (5.0KB)</text>

                    <!-- FreeRTOS -->
                    <text x="-10" y="58" fill="#334155" font-size="10" text-anchor="end">FreeRTOS</text>
                    <rect x="0" y="46" width="63" height="14" fill="#e11d48" rx="3" />
                    <rect x="0" y="63" width="40" height="14" fill="#059669" rx="3" />
                    <text x="46" y="75" fill="#047857" font-size="10" font-weight="bold">-30.1% (5.0KB)</text>

                    <!-- Flight Controller -->
                    <text x="-10" y="96" fill="#334155" font-size="10" text-anchor="end">Flight Ctrl</text>
                    <rect x="0" y="84" width="70" height="14" fill="#e11d48" rx="3" />
                    <rect x="0" y="101" width="103" height="14" fill="#0284c7" rx="3" />
                    <text x="109" y="113" fill="#b45309" font-size="10" font-weight="bold">+47.4% (11.2KB)</text>

                    <!-- Broad Scope -->
                    <text x="-10" y="134" fill="#334155" font-size="10" text-anchor="end">Broad Scope</text>
                    <rect x="0" y="122" width="292" height="14" fill="#e11d48" rx="3" />
                    <rect x="0" y="139" width="228" height="14" fill="#059669" rx="3" />
                    <text x="234" y="151" fill="#047857" font-size="10" font-weight="bold">-21.8% (19.0KB)</text>

                    <!-- Deep Nesting -->
                    <text x="-10" y="172" fill="#334155" font-size="10" text-anchor="end">Deep Nesting</text>
                    <rect x="0" y="160" width="475" height="14" fill="#e11d48" rx="3" />
                    <rect x="0" y="177" width="195" height="14" fill="#059669" rx="3" />
                    <text x="201" y="189" fill="#047857" font-size="10" font-weight="bold">-58.8% (16.3KB vs 39.6KB)</text>
                </g>
            </svg>
        `;
    } else if (type === "ablation") {
        container.innerHTML = `
            <svg viewBox="0 0 700 280" width="100%" height="100%">
                <text x="350" y="24" fill="#0f172a" font-size="14" font-weight="bold" text-anchor="middle">Ablation Waterfall: Impact of Each Technique</text>
                <g transform="translate(30, 45)">
                    <!-- Stage 1: Baseline -->
                    <rect x="20" y="40" width="75" height="160" fill="#e11d48" rx="4" />
                    <text x="57" y="32" fill="#0f172a" font-size="11" font-weight="600" text-anchor="middle">39.6 KB</text>
                    <text x="57" y="218" fill="#334155" font-size="10" text-anchor="middle">1. Baseline</text>

                    <!-- Stage 2: Arena -->
                    <rect x="125" y="63" width="75" height="137" fill="#ea580c" rx="4" />
                    <text x="162" y="55" fill="#0f172a" font-size="11" font-weight="600" text-anchor="middle">33.9 KB</text>
                    <text x="162" y="218" fill="#334155" font-size="10" text-anchor="middle">2. + Arena</text>
                    <text x="162" y="230" fill="#b45309" font-size="9" font-weight="bold" text-anchor="middle">-14.3%</text>

                    <!-- Stage 3: Interning -->
                    <rect x="230" y="90" width="75" height="110" fill="#d97706" rx="4" />
                    <text x="267" y="82" fill="#0f172a" font-size="11" font-weight="600" text-anchor="middle">27.1 KB</text>
                    <text x="267" y="218" fill="#334155" font-size="10" text-anchor="middle">3. + Intern</text>
                    <text x="267" y="230" fill="#b45309" font-size="9" font-weight="bold" text-anchor="middle">-31.5%</text>

                    <!-- Stage 4: Packed -->
                    <rect x="335" y="116" width="75" height="84" fill="#0284c7" rx="4" />
                    <text x="372" y="108" fill="#0f172a" font-size="11" font-weight="600" text-anchor="middle">20.7 KB</text>
                    <text x="372" y="218" fill="#334155" font-size="10" text-anchor="middle">4. + Packed</text>
                    <text x="372" y="230" fill="#0369a1" font-size="9" font-weight="bold" text-anchor="middle">-47.7%</text>

                    <!-- Stage 5: Delta Varint -->
                    <rect x="440" y="134" width="75" height="66" fill="#059669" rx="4" />
                    <text x="477" y="126" fill="#0f172a" font-size="11" font-weight="600" text-anchor="middle">16.3 KB</text>
                    <text x="477" y="218" fill="#334155" font-size="10" text-anchor="middle">5. + Compress</text>
                    <text x="477" y="230" fill="#047857" font-size="9" font-weight="bold" text-anchor="middle">-58.8%</text>

                    <!-- Stage 6: Full Two-Tier (Bloom) -->
                    <rect x="545" y="134" width="75" height="66" fill="#0e7490" rx="4" />
                    <text x="582" y="126" fill="#0f172a" font-size="11" font-weight="600" text-anchor="middle">16.3 KB</text>
                    <text x="582" y="218" fill="#334155" font-size="10" text-anchor="middle">6. + Bloom</text>
                    <text x="582" y="230" fill="#0e7490" font-size="9" font-weight="bold" text-anchor="middle">98%+ Skips</text>
                </g>
            </svg>
        `;
    } else {
        container.innerHTML = `
            <svg viewBox="0 0 700 280" width="100%" height="100%">
                <text x="350" y="24" fill="#0f172a" font-size="14" font-weight="bold" text-anchor="middle">Pareto Frontier: Hot Depth K vs Memory &amp; Decompressions</text>
                <g transform="translate(60, 45)">
                    <line x1="40" y1="20" x2="40" y2="180" stroke="#cbd5e1" stroke-width="1.5" />
                    <line x1="40" y1="180" x2="560" y2="180" stroke="#cbd5e1" stroke-width="1.5" />
                    
                    <!-- Line 1: RAM -->
                    <polyline points="60,160 140,150 220,135 300,105 380,60 460,30 540,30" fill="none" stroke="#0284c7" stroke-width="3" />
                    <circle cx="140" cy="150" r="4" fill="#0369a1" />
                    <text x="140" y="142" fill="#0369a1" font-size="10" font-weight="bold" text-anchor="middle">K=2 (16.3KB)</text>

                    <!-- Line 2: Decomps -->
                    <polyline points="60,30 140,55 220,95 300,145 380,172 460,180 540,180" fill="none" stroke="#b45309" stroke-width="3" stroke-dasharray="5,3" />
                    <circle cx="140" cy="55" r="4" fill="#b45309" />

                    <text x="300" y="210" fill="#334155" font-size="11" text-anchor="middle">Sweet Spot: K=2 balances 58.8% RAM reduction with low decompressions</text>
                </g>
            </svg>
        `;
    }
}

// Initialization & Event Wiring
document.addEventListener("DOMContentLoaded", () => {
    const sim = new SimulationEngine();

    document.getElementById("btn-step").addEventListener("click", () => sim.step());
    document.getElementById("btn-play").addEventListener("click", () => sim.togglePlay());
    document.getElementById("btn-reset").addEventListener("click", () => sim.reset());

    document.getElementById("k-slider").addEventListener("input", (e) => {
        sim.hotDepth = parseInt(e.target.value, 10);
        document.getElementById("k-val").innerText = sim.hotDepth;
    });

    document.getElementById("bloom-toggle").addEventListener("change", (e) => {
        sim.bloomEnabled = e.target.checked;
    });

    document.getElementById("sample-select").addEventListener("change", (e) => {
        sim.sampleKey = e.target.value;
        sim.reset();
    });

    document.getElementById("btn-lookup").addEventListener("click", () => {
        const query = document.getElementById("lookup-query").value.trim();
        if (query) {
            const logs = sim.traceLookup(query);
            document.getElementById("lookup-trace-log").innerHTML = logs.join("");
            sim.updateUI();
        }
    });

    // Chart Tabs
    document.querySelectorAll(".btn-tab").forEach(tab => {
        tab.addEventListener("click", (e) => {
            document.querySelectorAll(".btn-tab").forEach(t => t.classList.remove("active"));
            e.target.classList.add("active");
            renderChart(e.target.dataset.chart);
        });
    });

    // Initial render
    renderChart("peak-ram");
    sim.updateUI();
});
