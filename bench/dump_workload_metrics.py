#!/usr/bin/env python3
import subprocess
import re
import csv
import os

workloads = [
    'data/arduino_sensors.mc',
    'data/freertos_tasks.mc',
    'data/zephyr_network.mc',
    'data/mibench_telecom.mc',
    'data/embedded_flight_controller.mc',
    'data/synthetic_broad_scope.mc',
    'data/synthetic_heavy_shadowing.mc',
    'data/synthetic_deep_nesting.mc',
    'data/freertos_queue.mc',
    'data/mibench_qsort.mc',
    'data/mibench_bitcount.mc',
    'data/zephyr_synopsys.mc'
]

rows = []
for w in workloads:
    out = subprocess.check_output(['minicc.exe', '--dump-stats', w], text=True)
    m = re.search(r'\[WORKLOAD-METRICS\] file=(\S+) loc=(\d+) symbols=(\d+) scopes=(\d+) max_depth=(\d+)', out)
    if m:
        f, loc, sym, sc, depth = m.groups()
        rows.append({
            'workload': os.path.basename(f),
            'lines_of_code': int(loc),
            'symbols': int(sym),
            'scopes': int(sc),
            'max_depth': int(depth)
        })

os.makedirs('bench/results', exist_ok=True)
csv_path = 'bench/results/workload_metrics.csv'
with open(csv_path, 'w', newline='', encoding='utf-8') as f:
    writer = csv.DictWriter(f, fieldnames=['workload', 'lines_of_code', 'symbols', 'scopes', 'max_depth'])
    writer.writeheader()
    writer.writerows(rows)

print("Saved workload metrics to:", csv_path)
for r in rows:
    print(f"{r['workload']:<32} LOC: {r['lines_of_code']:<5} Symbols: {r['symbols']:<5} Scopes: {r['scopes']:<4} MaxDepth: {r['max_depth']:<3}")
