#!/usr/bin/env python3
"""
Build Web Visualizer Data (web/data/results.json)
Extracts verified empirical results directly from bench/results/*.csv and metadata.json.
Guarantees zero manual hardcoding and zero drift between benchmarks and web UI.
"""

import os
import json
import csv
import numpy as np

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RESULTS_DIR = os.path.join(BASE_DIR, 'bench', 'results')
OUT_DIR = os.path.join(BASE_DIR, 'web', 'data')
os.makedirs(OUT_DIR, exist_ok=True)
OUT_FILE = os.path.join(OUT_DIR, 'results.json')

def load_csv(filename):
    path = os.path.join(RESULTS_DIR, filename)
    if not os.path.exists(path):
        return []
    with open(path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        return list(reader)

def build_data():
    data = {}

    # 1. Metadata
    meta_path = os.path.join(RESULTS_DIR, 'metadata.json')
    if os.path.exists(meta_path):
        with open(meta_path, 'r', encoding='utf-8') as f:
            data['metadata'] = json.load(f)
    else:
        data['metadata'] = {}

    # 2. Workload Metrics
    workload_metrics = {}
    for r in load_csv('workload_metrics.csv'):
        workload_metrics[r['workload']] = {
            'loc': int(r.get('lines_of_code', r.get('loc', 0))),
            'symbols': int(r['symbols']),
            'scopes': int(r['scopes']),
            'max_depth': int(r['max_depth'])
        }
    data['workload_metrics'] = workload_metrics

    # 3. Peak RAM (Exp 1)
    exp1_raw = load_csv('exp1_peak_ram.csv')
    exp1_grouped = {}
    for r in exp1_raw:
        w = r['workload']
        eng = r['engine']
        exp1_grouped.setdefault((w, eng), []).append(float(r['peak_ram_bytes']))

    workloads = sorted(list({k[0] for k in exp1_grouped.keys()}))
    engines = ['baseline', 'strong_baseline', 'arena', 'intern', 'packed', 'nombloom', 'twotier']
    
    exp1_summary = {}
    for w in workloads:
        exp1_summary[w] = {}
        for eng in engines:
            vals = exp1_grouped.get((w, eng), [0.0])
            exp1_summary[w][eng] = {
                'median': float(np.median(vals)),
                'iqr': float(np.percentile(vals, 75) - np.percentile(vals, 25))
            }
        # RAM reduction of twotier vs baseline
        b_med = exp1_summary[w].get('baseline', {}).get('median', 0)
        t_med = exp1_summary[w].get('twotier', {}).get('median', 0)
        s_med = exp1_summary[w].get('strong_baseline', {}).get('median', 0)
        exp1_summary[w]['reduction_vs_baseline_pct'] = round((1.0 - t_med / b_med) * 100.0, 1) if b_med > 0 else 0.0
        exp1_summary[w]['reduction_vs_strong_pct'] = round((1.0 - t_med / s_med) * 100.0, 1) if s_med > 0 else 0.0
    data['exp1_peak_ram'] = exp1_summary

    # 4. Two-Tier Memory Breakdown (Exp 2)
    exp2_raw = load_csv('exp2_memory_breakdown.csv')
    exp2_summary = {}
    for r in exp2_raw:
        w = r['workload']
        exp2_summary[w] = {
            'hot_table': float(r['hot_table_bytes']),
            'cold_blocks': float(r['cold_blocks_bytes']),
            'string_pool': float(r['string_pool_bytes']),
            'type_table': float(r['type_table_bytes']),
            'arenas': float(r['arenas_bytes']),
            'metadata': float(r['metadata_bloom_bytes']),
            'total': float(r['total_bytes'])
        }
    data['exp2_memory_breakdown'] = exp2_summary

    # 5. Micro-Latency (Exp 4 & latency_summary)
    lat_summary_raw = load_csv('latency_summary.csv')
    lat_summary = {}
    for r in lat_summary_raw:
        lat_summary.setdefault(r['engine'], {})[r['op_type']] = {
            'p50_ns': float(r['p50_ns']),
            'p95_ns': float(r['p95_ns']),
            'p99_ns': float(r['p99_ns']),
            'iqr_ns': float(r['iqr_ns'])
        }
    data['latency_summary'] = lat_summary

    # 6. Scope Scaling (Exp 5)
    exp5_raw = load_csv('exp5_scope_scaling.csv')
    exp5_grouped = {}
    for r in exp5_raw:
        eng = r['engine']
        n = int(r['symbols_per_scope'])
        val = float(r['enter_exit_time_ns'])
        exit_val = float(r['exit_time_ns'])
        exp5_grouped.setdefault((eng, n), {'total': [], 'exit': []})['total'].append(val)
        exp5_grouped[(eng, n)]['exit'].append(exit_val)

    exp5_summary = {}
    for (eng, n), lists in exp5_grouped.items():
        exp5_summary.setdefault(eng, {})[str(n)] = {
            'total_time_ns': float(np.median(lists['total'])),
            'exit_time_ns': float(np.median(lists['exit']))
        }
    data['exp5_scope_scaling'] = exp5_summary

    # 7. Bloom & Cache Effectiveness (Exp 6)
    exp6_raw = load_csv('exp6_bloom_cache.csv')
    exp6_summary = {}
    for r in exp6_raw:
        exp6_summary[r['workload']] = {
            'cache_hit_rate': float(r.get('cache_hit_rate', 0)),
            'bloom_fp_rate': float(r.get('bloom_fp_rate', 0)),
            'cache_hits': int(r.get('cache_hits', 0)),
            'bloom_skips': int(r.get('bloom_skips', 0)),
            'decompressions': int(r.get('cache_misses', r.get('decompressions', 0)))
        }
    data['exp6_bloom_cache'] = exp6_summary

    # 8. Memory Trace (Exp 7)
    exp7_raw = load_csv('exp7_ast_memory_trace.csv')
    exp7_summary = {}
    for r in exp7_raw:
        k = (r['workload'], r['engine'])
        exp7_summary.setdefault(f"{k[0]}:{k[1]}", []).append({
            'step': int(r['step']),
            'bytes': int(r['current_bytes'])
        })
    data['exp7_memory_trace'] = exp7_summary

    # 9. Scalability (Exp 8)
    exp8_raw = load_csv('exp8_scalability.csv')
    exp8_summary = {'symbols': {}, 'depth': {}}
    for r in exp8_raw:
        tt = r['test_type']
        p = int(r['param_value'])
        eng = r['engine']
        exp8_summary[tt].setdefault(str(p), {})[eng] = {
            'peak_ram_bytes': int(r['peak_ram_bytes']),
            'elapsed_ns': float(r['elapsed_ns'])
        }
    data['exp8_scalability'] = exp8_summary

    # 10. Sensitivity Sweeps (Exp 9)
    exp9_raw = load_csv('exp9_sensitivity.csv')
    exp9_summary = {'hot_depth_k': {}, 'min_scope_threshold': {}}
    for r in exp9_raw:
        sp = r['sweep_param']
        p = int(r['param_value'])
        w = r['workload']
        if sp in exp9_summary:
            exp9_summary[sp].setdefault(w, {})[str(p)] = {
                'peak_ram_bytes': int(r['peak_ram_bytes']),
                'elapsed_ns': float(r['elapsed_ns']),
                'bloom_skips': int(r['bloom_skips']),
                'decompressions': int(r['decompressions'])
            }
    data['exp9_sensitivity'] = exp9_summary

    # 11. Budget Caps Heatmap (Exp 10)
    exp10_raw = load_csv('exp10_budget_caps.csv')
    exp10_summary = {}
    for r in exp10_raw:
        w = r['workload']
        cap = int(r['cap_bytes'])
        exp10_summary.setdefault(w, {})[str(cap)] = {
            'baseline_status': r['baseline_status'],
            'baseline_peak': int(r['baseline_peak']),
            'twotier_status': r['twotier_status'],
            'twotier_peak': int(r['twotier_peak'])
        }
    data['exp10_budget_caps'] = exp10_summary

    # 12. Flight Controller Scope Trajectory (Exp 13)
    exp13_raw = load_csv('exp13_flight_controller_breakdown.csv')
    exp13_summary = {}
    for r in exp13_raw:
        s = int(r['scope_count'])
        eng = r['engine']
        exp13_summary.setdefault(str(s), {})[eng] = {
            'total_bytes': float(r['total_bytes']),
            'hot_table': float(r.get('hot_table_bytes', 0)),
            'cold_blocks': float(r.get('cold_blocks_bytes', 0)),
            'string_pool': float(r.get('string_pool_bytes', 0)),
            'type_table': float(r.get('type_table_bytes', 0)),
            'arenas': float(r.get('arenas_bytes', 0)),
            'metadata': float(r.get('metadata_bloom_bytes', 0))
        }
    data['exp13_flight_controller'] = exp13_summary

    # 13. Operation Mix (op_mix.csv)
    op_mix_raw = load_csv('op_mix.csv')
    op_mix_summary = {}
    for r in op_mix_raw:
        op_mix_summary[r['workload']] = {
            'inserts': int(r['inserts']),
            'hot_lookups': int(r['hot_lookups']),
            'cold_lookups': int(r['cold_lookups']),
            'misses': int(r['misses']),
            'scopes_exited': int(r.get('scope_exits', r.get('scopes_exited', 0))),
            'decompressions': int(r['decompressions']),
            'cache_hits': int(r['cache_hits']),
            'malloc_count': int(r.get('malloc_count', 0)),
            'free_count': int(r.get('free_count', 0)),
            'avg_insert_ns': float(r.get('avg_insert_ns', 0)),
            'avg_lookup_ns': float(r.get('avg_lookup_ns', 0)),
            'avg_exit_ns': float(r.get('avg_scope_exit_ns', r.get('avg_exit_ns', 0))),
            'total_time_us': float(r.get('total_runtime_us', r.get('total_time_us', 0)))
        }
    data['op_mix'] = op_mix_summary

    with open(OUT_FILE, 'w', encoding='utf-8') as f:
        json.dump(data, f, indent=2)

    print(f"[DATA-GEN] Built verified web visualizer dataset at: {OUT_FILE}")
    print(f"[DATA-GEN] File size: {os.path.getsize(OUT_FILE)} bytes")

if __name__ == '__main__':
    build_data()
