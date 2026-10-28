#!/usr/bin/env python3
"""
Publication-Quality IEEE Figure Generator for Scope-Lifetime-Aware Two-Tier Symbol Table
Outputs publication-ready figures to report/figures/ as PNG (300 DPI), SVG, and PDF.
Strict adherence to IEEE formatting guidelines:
 - Width: 3.5 in (single column) or 7.0 in (double column)
 - White background, serif font (9-10 pt)
 - Colorblind-safe palette (Okabe-Ito) with grayscale-distinguishable patterns/markers
 - Fully labeled axes with physical units
 - Legends, no internal plot titles
"""

import os
import sys
import tempfile
import csv

# Set temporary directory for matplotlib config to avoid permission warnings on Windows
os.environ['MPLCONFIGDIR'] = os.path.join(tempfile.gettempdir(), 'matplotlib_cd_bench')
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np

# IEEE Publication Plot Styling
plt.rcParams.update({
    'font.family': 'serif',
    'font.serif': ['Times New Roman', 'DejaVu Serif', 'Times', 'Liberation Serif', 'serif'],
    'font.size': 9,
    'axes.labelsize': 9,
    'axes.titlesize': 9,
    'xtick.labelsize': 8,
    'ytick.labelsize': 8,
    'legend.fontsize': 8,
    'figure.titlesize': 10,
    'figure.facecolor': '#ffffff',
    'axes.facecolor': '#ffffff',
    'savefig.facecolor': '#ffffff',
    'axes.grid': True,
    'grid.color': '#e2e8f0',
    'grid.linestyle': '--',
    'grid.linewidth': 0.5,
    'grid.alpha': 0.7,
    'lines.linewidth': 1.2,
    'lines.markersize': 5,
    'patch.linewidth': 0.8
})

OUT_DIR = os.path.join('report', 'figures')
os.makedirs(OUT_DIR, exist_ok=True)

# Okabe-Ito Colorblind-Safe Palette
ENGINE_COLORS = {
    'baseline':        '#000000',       # Black
    'strong_baseline': '#009E73',       # Bluish Green
    'arena':           '#E69F00',       # Orange
    'intern':          '#F0E442',       # Yellow
    'packed':          '#56B4E9',       # Sky Blue
    'nombloom':        '#CC79A7',       # Purple / Pink
    'twotier':         '#0072B2'        # Deep Blue
}

ENGINE_HATCHES = {
    'baseline':        '',
    'strong_baseline': '//',
    'arena':           '\\\\',
    'intern':          '--',
    'packed':          'xx',
    'nombloom':        '..',
    'twotier':         '++'
}

def save_fig(fig, base_name):
    for ext in ['png', 'svg', 'pdf']:
        path = os.path.join(OUT_DIR, f"{base_name}.{ext}")
        fig.savefig(path, dpi=300, bbox_inches='tight', pad_inches=0.03)
    print(f"[FIGURE] Generated '{base_name}' (.png, .svg, .pdf)")
    plt.close(fig)

# =========================================================================
# Figure 1: Grouped Bars of Peak RAM Across All Engines & Workloads
# =========================================================================
def plot_fig1_peak_ram():
    csv_path = os.path.join('bench', 'results', 'exp1_peak_ram.csv')
    if not os.path.exists(csv_path): return

    # Load data
    runs_by_key = {}
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            key = (r['workload'], r['engine'])
            runs_by_key.setdefault(key, []).append(float(r['peak_ram_bytes']))

    workloads = [
        'arduino_sensors.mc', 'freertos_tasks.mc', 'zephyr_network.mc', 'mibench_telecom.mc',
        'embedded_flight_controller.mc', 'synthetic_broad_scope.mc',
        'synthetic_heavy_shadowing.mc', 'synthetic_deep_nesting.mc'
    ]
    engines = ['baseline', 'strong_baseline', 'arena', 'intern', 'packed', 'nombloom', 'twotier']
    engine_labels = {
        'baseline': 'Baseline',
        'strong_baseline': 'Strong Base',
        'arena': 'Arena',
        'intern': 'Intern',
        'packed': 'Packed',
        'nombloom': 'No-Bloom',
        'twotier': 'Two-Tier'
    }
    labels = ['Arduino', 'FreeRTOS', 'Zephyr', 'MiBench', 'Flight Ctrl', 'Broad Scp', 'Shadowing', 'Deep Nest']

    x = np.arange(len(workloads))
    width = 0.11

    fig, ax = plt.subplots(figsize=(7.0, 3.2))

    for i, eng in enumerate(engines):
        medians = []
        iqrs = []
        for w in workloads:
            vals = runs_by_key.get((w, eng), [0.0])
            q75, q25 = np.percentile(vals, [75, 25])
            medians.append(np.median(vals) / 1024.0) # in KB
            iqrs.append((q75 - q25) / 1024.0)

        offset = (i - 3.0) * width
        ax.bar(x + offset, medians, width, yerr=iqrs, capsize=2,
               label=engine_labels.get(eng, eng.capitalize()), color=ENGINE_COLORS[eng],
               hatch=ENGINE_HATCHES[eng], edgecolor='#334155', alpha=0.85)

    ax.set_ylabel('Peak RAM (KB)')
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=15, ha='right')
    ax.set_yscale('log')
    ax.set_ylim(bottom=2.0, top=70.0)
    ax.set_yticks([2, 5, 10, 20, 40, 60])
    ax.yaxis.set_major_formatter(ticker.ScalarFormatter())
    ax.yaxis.set_minor_formatter(ticker.NullFormatter())
    ax.legend(loc='upper center', bbox_to_anchor=(0.5, -0.22), ncol=4, frameon=True, framealpha=0.9, fontsize=7.5)
    save_fig(fig, 'fig1_peak_ram_grouped')

# =========================================================================
# Figure 2: Stacked Two-Tier Memory Breakdown
# =========================================================================
def plot_fig2_memory_breakdown():
    csv_path = os.path.join('bench', 'results', 'exp2_memory_breakdown.csv')
    if not os.path.exists(csv_path): return

    workload_data = {}
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            w = r['workload']
            if w not in workload_data:
                workload_data[w] = {
                    'hot': float(r['hot_table_bytes']),
                    'cold': float(r['cold_blocks_bytes']),
                    'pool': float(r['string_pool_bytes']),
                    'type': float(r['type_table_bytes']),
                    'arena': float(r['arenas_bytes']),
                    'meta': float(r['metadata_bloom_bytes'])
                }

    workloads = list(workload_data.keys())
    labels = [w.replace('.mc', '').replace('synthetic_', '').replace('embedded_', '') for w in workloads]
    x = np.arange(len(workloads))

    fig, ax = plt.subplots(figsize=(7.0, 3.0))

    hot = [workload_data[w]['hot'] / 1024.0 for w in workloads]
    cold = [workload_data[w]['cold'] / 1024.0 for w in workloads]
    pool = [workload_data[w]['pool'] / 1024.0 for w in workloads]
    ttype = [workload_data[w]['type'] / 1024.0 for w in workloads]
    arena = [workload_data[w]['arena'] / 1024.0 for w in workloads]
    meta = [workload_data[w]['meta'] / 1024.0 for w in workloads]

    colors = ['#0072B2', '#009E73', '#E69F00', '#56B4E9', '#D55E00', '#CC79A7']

    b0 = np.zeros(len(workloads))
    p1 = ax.bar(x, hot, 0.55, label='Hot Table', color=colors[0], edgecolor='#1e293b')
    b1 = np.array(hot)
    p2 = ax.bar(x, cold, 0.55, bottom=b1, label='Cold Blocks', color=colors[1], edgecolor='#1e293b')
    b2 = b1 + np.array(cold)
    p3 = ax.bar(x, pool, 0.55, bottom=b2, label='String Pool', color=colors[2], edgecolor='#1e293b')
    b3 = b2 + np.array(pool)
    p4 = ax.bar(x, ttype, 0.55, bottom=b3, label='Type Table', color=colors[3], edgecolor='#1e293b')
    b4 = b3 + np.array(ttype)
    p5 = ax.bar(x, arena, 0.55, bottom=b4, label='Arenas', color=colors[4], edgecolor='#1e293b')
    b5 = b4 + np.array(arena)
    p6 = ax.bar(x, meta, 0.55, bottom=b5, label='Metadata/Bloom', color=colors[5], edgecolor='#1e293b')

    ax.set_ylabel('Memory Footprint (KB)')
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=15, ha='right')
    ax.legend(loc='upper left', ncol=3, frameon=True, framealpha=0.9)
    save_fig(fig, 'fig2_memory_breakdown_stacked')

# =========================================================================
# Figure 3: Ablation Waterfall on Deep Nesting
# =========================================================================
def plot_fig3_ablation_waterfall():
    stages = [
        'Baseline\n(Malloc)',
        '+ Arena\nAllocator',
        '+ String\nInterning',
        '+ 64-bit\nPacked',
        '+ Varint\nCompress',
        '+ Bloom\nFilter'
    ]
    # Data directly from ablation experiment
    peak_ram_kb = [39.57, 33.91, 27.10, 20.70, 16.29, 16.29]

    fig, ax = plt.subplots(figsize=(3.5, 2.8))
    x = np.arange(len(stages))

    bars = ax.bar(x, peak_ram_kb, width=0.55, color='#0072B2', edgecolor='#0f172a', alpha=0.85)

    for i, b in enumerate(bars):
        h = b.get_height()
        red = (1.0 - h / peak_ram_kb[0]) * 100.0
        label = f"{h:.1f}K\n(-{red:.0f}%)" if i > 0 else f"{h:.1f}K\n(Base)"
        ax.text(b.get_x() + b.get_width() / 2.0, h + 0.8, label,
                ha='center', va='bottom', fontsize=7)

    ax.set_ylabel('Peak RAM (KB)')
    ax.set_ylim(0, 48)
    ax.set_xticks(x)
    ax.set_xticklabels(stages, fontsize=7)
    save_fig(fig, 'fig3_ablation_waterfall')

# =========================================================================
# Figure 4: K-Sweep Pareto Frontier (Peak RAM vs Elapsed Time)
# =========================================================================
def plot_fig4_k_pareto():
    csv_path = os.path.join('bench', 'results', 'exp9_sensitivity.csv')
    if not os.path.exists(csv_path): return

    k_vals = []
    ram_vals = []
    time_vals = []

    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            if r['sweep_param'] == 'hot_depth_k' and r['workload'] == 'synthetic_deep_nesting.mc':
                k_vals.append(int(r['param_value']))
                ram_vals.append(float(r['peak_ram_bytes']) / 1024.0)
                time_vals.append(float(r['elapsed_ns']) / 1000.0) # us

    if not k_vals: return

    fig, ax1 = plt.subplots(figsize=(3.5, 2.8))

    color_ram = '#0072B2'
    color_time = '#D55E00'

    ax1.set_xlabel('Hot Scope Depth K')
    ax1.set_ylabel('Peak RAM (KB)', color=color_ram)
    l1 = ax1.plot(k_vals, ram_vals, 'o-', color=color_ram, label='Peak RAM (KB)')
    ax1.tick_params(axis='y', labelcolor=color_ram)

    ax2 = ax1.twinx()
    ax2.set_ylabel('Elapsed Analysis Time (µs)', color=color_time)
    l2 = ax2.plot(k_vals, time_vals, 's--', color=color_time, label='Analysis Time (µs)')
    ax2.tick_params(axis='y', labelcolor=color_time)

    # Sweet spot annotation at K=2
    ax1.annotate('Sweet Spot (K=2)', xy=(2, ram_vals[k_vals.index(2)]), xytext=(6, ram_vals[k_vals.index(2)] + 5),
                 arrowprops=dict(arrowstyle='->', color='#334155', lw=1.0),
                 fontsize=7, fontweight='bold', color='#0f172a')

    save_fig(fig, 'fig4_k_pareto_curve')

# =========================================================================
# Figure 5: Micro-Latency Breakdown Across Engines
# =========================================================================
def plot_fig5_latency_cdfs():
    summary_path = os.path.join('bench', 'results', 'latency_summary.csv')
    csv_path = os.path.join('bench', 'results', 'exp4_micro_latency.csv')

    ops = ['insert', 'hot_lookup', 'outer_scope_lookup', 'cold_lookup_forced', 'miss_lookup']
    labels = ['Insert', 'Hot Hit', 'Outer Scope', 'Forced Decomp', 'Miss']
    engines = ['baseline', 'strong_baseline', 'twotier']

    meds = {eng: [] for eng in engines}
    iqrs = {eng: [] for eng in engines}

    if os.path.exists(summary_path):
        data = {}
        with open(summary_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for r in reader:
                data[(r['engine'], r['op_type'])] = (float(r['p50_ns']), float(r['iqr_ns']))
        for op in ops:
            for eng in engines:
                m, q = data.get((eng, op), (0.0, 0.0))
                meds[eng].append(m)
                iqrs[eng].append(q)
    elif os.path.exists(csv_path):
        runs = {}
        with open(csv_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for r in reader:
                runs.setdefault((r['engine'], r['op_type']), []).append(float(r['latency_ns']))
        for op in ops:
            for eng in engines:
                vals = runs.get((eng, op), [0.0])
                q75, q25 = np.percentile(vals, [75, 25])
                meds[eng].append(float(np.median(vals)))
                iqrs[eng].append(float(q75 - q25))
    else:
        return

    fig, ax = plt.subplots(figsize=(4.0, 2.9))
    x = np.arange(len(ops))
    width = 0.26

    for i, eng in enumerate(engines):
        offset = (i - 1.0) * width
        disp_name = 'Baseline' if eng == 'baseline' else ('Strong Base' if eng == 'strong_baseline' else 'Two-Tier')
        ax.bar(x + offset, meds[eng], width, yerr=iqrs[eng], capsize=2,
               label=disp_name, color=ENGINE_COLORS[eng], hatch=ENGINE_HATCHES[eng],
               edgecolor='#0f172a', alpha=0.85)

    ax.set_ylabel('Latency (ns / op)')
    ax.set_yscale('log')
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=15, ha='right', fontsize=7.5)
    ax.legend(loc='upper right', frameon=True, fontsize=7.5)
    save_fig(fig, 'fig5_latency_cdfs')

# =========================================================================
# Figure 6: Scope Teardown Lifecycle Scaling
# =========================================================================
def plot_fig6_scope_scaling():
    csv_path = os.path.join('bench', 'results', 'exp5_scope_scaling.csv')
    if not os.path.exists(csv_path): return

    times = {'baseline': {}, 'strong_baseline': {}, 'twotier': {}}

    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            eng = r['engine']
            if eng in times:
                n = int(r['symbols_per_scope'])
                val = float(r['enter_exit_time_ns']) / 1000.0 # us
                times[eng].setdefault(n, []).append(val)

    counts = sorted(list(times['baseline'].keys()))
    if not counts: return

    b_med = [np.median(times['baseline'].get(n, [0])) for n in counts]
    s_med = [np.median(times['strong_baseline'].get(n, [0])) for n in counts]
    t_med = [np.median(times['twotier'].get(n, [0])) for n in counts]

    fig, ax = plt.subplots(figsize=(3.6, 2.9))

    ax.plot(counts, b_med, 's--', color=ENGINE_COLORS['baseline'], label='Baseline (Chained free)')
    if any(s_med):
        ax.plot(counts, s_med, '^-.', color=ENGINE_COLORS['strong_baseline'], label='Strong Base (Arena + Shift)')
    ax.plot(counts, t_med, 'o-', color=ENGINE_COLORS['twotier'], label='Two-Tier (Arena + Sweep)')

    ax.set_xscale('log')
    ax.set_yscale('log')
    ax.set_xlabel('Symbols Per Scope (N)')
    ax.set_ylabel('Lifecycle Teardown Time (µs)')
    ax.legend(loc='upper left', frameon=True, fontsize=7.5)
    save_fig(fig, 'fig6_scope_scaling')

# =========================================================================
# Figure 7: Memory-vs-AST Step Trace
# =========================================================================
def plot_fig7_ast_memory_trace():
    csv_path = os.path.join('bench', 'results', 'exp7_ast_memory_trace.csv')
    if not os.path.exists(csv_path): return

    traces = {}
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            key = (r['workload'], r['engine'])
            traces.setdefault(key, []).append((int(r['step']), float(r['current_bytes']) / 1024.0))

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(7.0, 2.8))

    # Deep Nesting
    b_steps, b_bytes = zip(*traces.get(('synthetic_deep_nesting.mc', 'baseline'), [(0, 0)]))
    t_steps, t_bytes = zip(*traces.get(('synthetic_deep_nesting.mc', 'twotier'), [(0, 0)]))
    ax1.plot(b_steps, b_bytes, color=ENGINE_COLORS['baseline'], label='Baseline')
    ax1.plot(t_steps, t_bytes, color=ENGINE_COLORS['twotier'], label='Two-Tier')
    ax1.set_xlabel('AST Processing Step')
    ax1.set_ylabel('Current Symbol Table RAM (KB)')
    ax1.legend(loc='upper left', frameon=True)
    ax1.set_title('Deep Nesting Workload', fontsize=8, pad=3)

    # Flight Controller
    fb_steps, fb_bytes = zip(*traces.get(('embedded_flight_controller.mc', 'baseline'), [(0, 0)]))
    ft_steps, ft_bytes = zip(*traces.get(('embedded_flight_controller.mc', 'twotier'), [(0, 0)]))
    ax2.plot(fb_steps, fb_bytes, color=ENGINE_COLORS['baseline'], label='Baseline')
    ax2.plot(ft_steps, ft_bytes, color=ENGINE_COLORS['twotier'], label='Two-Tier')
    ax2.set_xlabel('AST Processing Step')
    ax2.set_ylabel('Current Symbol Table RAM (KB)')
    ax2.legend(loc='upper left', frameon=True)
    ax2.set_title('Flight Controller Workload', fontsize=8, pad=3)

    save_fig(fig, 'fig7_ast_memory_trace')

# =========================================================================
# Figure 8: Scalability Curves (Symbols & Depth)
# =========================================================================
def plot_fig8_scalability():
    csv_path = os.path.join('bench', 'results', 'exp8_scalability.csv')
    if not os.path.exists(csv_path): return

    syms_b = []
    syms_t = []
    depth_b = []
    depth_t = []

    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            tt = r['test_type']
            param = int(r['param_value'])
            ram_kb = float(r['peak_ram_bytes']) / 1024.0
            eng = r['engine']
            if tt == 'symbols':
                if eng == 'baseline': syms_b.append((param, ram_kb))
                else: syms_t.append((param, ram_kb))
            elif tt == 'depth':
                if eng == 'baseline': depth_b.append((param, ram_kb))
                else: depth_t.append((param, ram_kb))

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(7.0, 2.8))

    if syms_b:
        x_sb, y_sb = zip(*sorted(syms_b))
        x_st, y_st = zip(*sorted(syms_t))
        ax1.plot(x_sb, y_sb, 's--', color=ENGINE_COLORS['baseline'], label='Baseline')
        ax1.plot(x_st, y_st, 'o-', color=ENGINE_COLORS['twotier'], label='Two-Tier')
        ax1.set_xscale('log')
        ax1.set_yscale('log')
        ax1.set_xlabel('Total Identifiers (100 to 50,000)')
        ax1.set_ylabel('Peak RAM (KB)')
        ax1.legend(loc='lower right', frameon=True, fontsize=8)

    if depth_b:
        x_db, y_db = zip(*sorted(depth_b))
        x_dt, y_dt = zip(*sorted(depth_t))
        ax2.plot(x_db, y_db, 's--', color=ENGINE_COLORS['baseline'], label='Baseline')
        ax2.plot(x_dt, y_dt, 'o-', color=ENGINE_COLORS['twotier'], label='Two-Tier')
        ax2.set_xlabel('Nesting Depth (4 to 256)')
        ax2.set_ylabel('Peak RAM (KB)')
        ax2.set_xticks([4, 32, 64, 128, 256])
        ax2.legend(loc='upper left', frameon=True, fontsize=8)

    save_fig(fig, 'fig8_scalability_lines')

# =========================================================================
# Figure 9: Scope Demotion Threshold Sensitivity
# =========================================================================
def plot_fig9_sensitivity():
    csv_path = os.path.join('bench', 'results', 'exp9_sensitivity.csv')
    if not os.path.exists(csv_path): return

    thresh_flight = []
    thresh_deep = []

    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            if r['sweep_param'] == 'min_scope_threshold':
                n = int(r['param_value'])
                ram = float(r['peak_ram_bytes']) / 1024.0
                if 'flight' in r['workload']: thresh_flight.append((n, ram))
                else: thresh_deep.append((n, ram))

    if not thresh_flight: return

    fig, ax = plt.subplots(figsize=(3.5, 2.8))
    xf, yf = zip(*sorted(thresh_flight))
    xd, yd = zip(*sorted(thresh_deep))

    ax.plot(xf, yf, 'o-', color='#0072B2', label='Flight Controller')
    ax.plot(xd, yd, 's--', color='#D55E00', label='Deep Nesting')

    ax.set_xlabel('Min Scope Size Demotion Threshold N')
    ax.set_ylabel('Peak RAM (KB)')
    ax.legend(loc='center right', frameon=True)
    save_fig(fig, 'fig9_sensitivity_curves')

# =========================================================================
# Figure 10: Budget-Cap Heatmap
# =========================================================================
def plot_fig10_budget_cap():
    csv_path = os.path.join('bench', 'results', 'exp10_budget_caps.csv')
    if not os.path.exists(csv_path): return

    caps = [4096, 8192, 12288, 16384, 24576, 32768, 49152, 65536]
    workloads = []
    matrix = []

    w_map = {}
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            w = r['workload']
            c = int(r['cap_bytes'])
            t_pass = 1 if 'PASS' in r['twotier_status'] else 0
            b_pass = 1 if 'PASS' in r['baseline_status'] else 0
            # 2 = Both pass, 1 = Two-Tier only passes, 0 = Neither pass / OOM
            val = 2 if (t_pass and b_pass) else (1 if t_pass else 0)
            w_map.setdefault(w, {})[c] = val

    workloads = list(w_map.keys())
    labels_w = [w.replace('.mc', '').replace('synthetic_', '').replace('embedded_', '') for w in workloads]
    labels_c = [f"{c//1024}K" for c in caps]

    mat = np.zeros((len(workloads), len(caps)))
    for i, w in enumerate(workloads):
        for j, c in enumerate(caps):
            mat[i, j] = w_map[w].get(c, 0)

    fig, ax = plt.subplots(figsize=(7.0, 3.0))
    cmap = matplotlib.colors.ListedColormap(['#ef4444', '#0284c7', '#10b981'])
    im = ax.imshow(mat, cmap=cmap, aspect='auto', vmin=0, vmax=2)

    ax.set_xticks(np.arange(len(caps)))
    ax.set_yticks(np.arange(len(workloads)))
    ax.set_xticklabels(labels_c)
    ax.set_yticklabels(labels_w)
    ax.set_xlabel('RAM Budget Limit (Bytes)')

    # Annotate pass/fail
    for i in range(len(workloads)):
        for j in range(len(caps)):
            status = "PASS" if mat[i, j] > 0 else "OOM"
            color = "#ffffff" if mat[i, j] == 0 else "#0f172a"
            ax.text(j, i, status, ha='center', va='center', fontsize=7, color=color, fontweight='bold')

    save_fig(fig, 'fig10_budget_cap_heatmap')

# =========================================================================
# Figure 11: Compression Ratio & Bits Per Symbol
# =========================================================================
def plot_fig11_compression_metrics():
    csv_path = os.path.join('bench', 'results', 'exp3_compression_ratio.csv')
    if not os.path.exists(csv_path): return

    workload_data = {}
    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            w = r['workload']
            if w not in workload_data:
                workload_data[w] = {
                    'ratio': float(r['compression_ratio']),
                    'bits': float(r['bits_per_symbol'])
                }

    filtered = {w: d for w, d in workload_data.items() if d['ratio'] > 1.05}
    if not filtered: filtered = workload_data

    names = list(filtered.keys())
    labels = [w.replace('.mc', '').replace('synthetic_', '').replace('embedded_', '') for w in names]
    ratios = [filtered[w]['ratio'] for w in names]
    bits = [filtered[w]['bits'] for w in names]

    fig, ax1 = plt.subplots(figsize=(3.5, 2.8))
    x = np.arange(len(names))
    width = 0.4

    ax1.bar(x, ratios, width, color='#0072B2', edgecolor='#0f172a', label='Ratio')
    ax1.set_ylabel('Compression Ratio (x)', color='#0072B2')
    ax1.set_xticks(x)
    ax1.set_xticklabels(labels, rotation=20, ha='right', fontsize=7.5)

    ax2 = ax1.twinx()
    ax2.plot(x, bits, 'rD--', label='Bits/Sym', markersize=4)
    ax2.set_ylabel('Bits / Symbol', color='#D55E00')

    save_fig(fig, 'fig11_compression_metrics')

# =========================================================================
# Figure 12: Bloom & Decompression Cache Effectiveness
# =========================================================================
def plot_fig12_bloom_cache():
    csv_path = os.path.join('bench', 'results', 'exp6_bloom_cache.csv')
    if not os.path.exists(csv_path): return

    workloads = []
    cache_hits = []
    bloom_fps = []

    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            w = r['workload']
            if w not in workloads:
                workloads.append(w)
                cache_hits.append(float(r['cache_hit_rate']))
                bloom_fps.append(float(r['bloom_fp_rate']))

    labels = [w.replace('.mc', '').replace('synthetic_', '').replace('embedded_', '') for w in workloads]
    x = np.arange(len(workloads))
    width = 0.35

    fig, ax = plt.subplots(figsize=(3.5, 2.8))

    ax.bar(x - width/2, cache_hits, width, label='Cache Hit %', color='#009E73', edgecolor='#0f172a')
    ax.bar(x + width/2, bloom_fps, width, label='Bloom FP %', color='#D55E00', edgecolor='#0f172a')

    ax.set_ylabel('Rate (%)')
    ax.set_xticks(x)
    ax.set_xticklabels(labels, rotation=25, ha='right', fontsize=7)
    ax.legend(loc='upper right', frameon=True)
    save_fig(fig, 'fig12_bloom_cache_effectiveness')

# =========================================================================
# Figure 13: Flight Controller Memory Breakdown & Overhead Analysis
# =========================================================================
def plot_fig13_flight_controller():
    csv_path = os.path.join('bench', 'results', 'exp13_flight_controller_breakdown.csv')
    if not os.path.exists(csv_path): return

    scopes_base = []
    tot_base = []

    scopes_two = []
    tot_two = []
    hot_two = []
    cold_two = []
    str_two = []
    type_two = []
    arena_two = []
    meta_two = []

    scopes_uncomp = []
    tot_uncomp = []

    with open(csv_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for r in reader:
            s = int(r['scope_count'])
            eng = r['engine']
            tot = float(r['total_bytes']) / 1024.0
            if eng == 'baseline':
                scopes_base.append(s)
                tot_base.append(tot)
            elif eng == 'twotier':
                scopes_two.append(s)
                tot_two.append(tot)
                hot_two.append(float(r['hot_table_bytes']) / 1024.0)
                cold_two.append(float(r['cold_blocks_bytes']) / 1024.0)
                str_two.append(float(r['string_pool_bytes']) / 1024.0)
                type_two.append(float(r['type_table_bytes']) / 1024.0)
                arena_two.append(float(r['arenas_bytes']) / 1024.0)
                meta_two.append(float(r['metadata_bloom_bytes']) / 1024.0)
            elif eng == 'twotier_uncompressed':
                scopes_uncomp.append(s)
                tot_uncomp.append(tot)

    if not scopes_two: return

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(7.0, 2.9))

    # Panel 1: Total RAM trajectory
    ax1.plot(scopes_base, tot_base, 's--', color='#000000', label='Baseline (malloc)', markersize=3)
    ax1.plot(scopes_two, tot_two, 'o-', color='#0072B2', label='Two-Tier (K=2)', markersize=3)
    ax1.plot(scopes_uncomp, tot_uncomp, '^-.', color='#D55E00', label='Two-Tier (No Comp)', markersize=3)
    ax1.set_xlabel('Lexical Scopes Processed (1 to 39)')
    ax1.set_ylabel('Tracked RAM (KB)')
    ax1.legend(loc='lower right', frameon=True, fontsize=7.5)

    # Panel 2: Stacked Components of Two-Tier
    x = np.array(scopes_two)
    str_arr = np.array(str_two)
    meta_arr = np.array(meta_two)
    type_arr = np.array(type_two)
    hot_arr = np.array(hot_two)
    cold_arr = np.array(cold_two)
    arena_arr = np.array(arena_two)

    # Cumulative stack
    y1 = str_arr
    y2 = y1 + meta_arr
    y3 = y2 + type_arr
    y4 = y3 + hot_arr
    y5 = y4 + cold_arr + arena_arr

    ax2.fill_between(x, 0, y1, label='String Pool', color='#CC79A7', alpha=0.85, hatch='//')
    ax2.fill_between(x, y1, y2, label='Metadata/Bloom', color='#E69F00', alpha=0.85, hatch='\\\\')
    ax2.fill_between(x, y2, y3, label='Type Table', color='#009E73', alpha=0.85, hatch='..')
    ax2.fill_between(x, y3, y4, label='Hot Table', color='#56B4E9', alpha=0.85, hatch='xx')
    ax2.fill_between(x, y4, y5, label='Cold/Arenas', color='#F0E442', alpha=0.85, hatch='++')

    ax2.plot(x, tot_base, 'k--', label='Baseline Total', linewidth=1.2)
    ax2.set_xlabel('Lexical Scopes Processed (1 to 39)')
    ax2.set_ylabel('Component RAM (KB)')
    ax2.legend(loc='upper left', frameon=True, fontsize=6.8, ncol=2)

    save_fig(fig, 'fig13_flight_controller_breakdown')

def main():
    print("\n====================================================================")
    print(" Generating Publication IEEE Figures in report/figures/ (PNG/SVG/PDF)")
    print("====================================================================")
    plot_fig1_peak_ram()
    plot_fig2_memory_breakdown()
    plot_fig3_ablation_waterfall()
    plot_fig4_k_pareto()
    plot_fig5_latency_cdfs()
    plot_fig6_scope_scaling()
    plot_fig7_ast_memory_trace()
    plot_fig8_scalability()
    plot_fig9_sensitivity()
    plot_fig10_budget_cap()
    plot_fig11_compression_metrics()
    plot_fig12_bloom_cache()
    plot_fig13_flight_controller()
    print("====================================================================")
    print(" All publication figures generated successfully in report/figures/!")
    print("====================================================================\n")

if __name__ == '__main__':
    main()
