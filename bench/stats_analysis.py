#!/usr/bin/env python3
"""
Statistical Significance & Bootstrap Confidence Interval Analysis
Uses Wilcoxon signed-rank test (scipy.stats.wilcoxon) and 10,000 bootstrap resamples
to rigorously quantify latency ratios, memory reductions, and scope lifecycle performance.
"""

import os
import csv
import numpy as np
try:
    from scipy.stats import wilcoxon
    HAS_SCIPY = True
except ImportError:
    HAS_SCIPY = False

def bootstrap_ci(sample_a, sample_b, num_resamples=10000, ci=0.95):
    """
    Computes bootstrap confidence interval for ratio sample_b / sample_a.
    """
    sample_a = np.array(sample_a, dtype=float)
    sample_b = np.array(sample_b, dtype=float)
    if len(sample_a) == 0 or len(sample_b) == 0:
        return 1.0, 1.0, 1.0

    n = min(len(sample_a), len(sample_b))
    ratios = []
    rng = np.random.default_rng(seed=42)
    for _ in range(num_resamples):
        idx_a = rng.choice(len(sample_a), size=n, replace=True)
        idx_b = rng.choice(len(sample_b), size=n, replace=True)
        med_a = np.median(sample_a[idx_a])
        med_b = np.median(sample_b[idx_b])
        if med_a > 0:
            ratios.append(med_b / med_a)

    if not ratios:
        return 1.0, 1.0, 1.0

    ratios = np.sort(ratios)
    low_idx = int(((1.0 - ci) / 2.0) * len(ratios))
    high_idx = int(((1.0 + ci) / 2.0) * len(ratios))
    high_idx = min(high_idx, len(ratios) - 1)
    observed_ratio = np.median(sample_b) / np.median(sample_a) if np.median(sample_a) > 0 else 1.0
    return observed_ratio, ratios[low_idx], ratios[high_idx]

def compute_wilcoxon(sample_a, sample_b):
    """
    Computes Wilcoxon signed-rank test statistic and p-value.
    """
    sample_a = np.array(sample_a, dtype=float)
    sample_b = np.array(sample_b, dtype=float)
    n = min(len(sample_a), len(sample_b))
    sample_a = sample_a[:n]
    sample_b = sample_b[:n]

    diff = sample_b - sample_a
    if np.all(diff == 0):
        return 0.0, 1.0

    if HAS_SCIPY:
        try:
            res = wilcoxon(sample_a, sample_b)
            return float(res.statistic), float(res.pvalue)
        except Exception:
            pass

    # Exact fallback rank-sum calculation
    abs_diff = np.abs(diff)
    nonzero_mask = abs_diff > 0
    if not np.any(nonzero_mask):
        return 0.0, 1.0
    nonzero_diff = diff[nonzero_mask]
    nonzero_abs = abs_diff[nonzero_mask]
    ranks = np.argsort(np.argsort(nonzero_abs)) + 1.0
    w_plus = np.sum(ranks[nonzero_diff > 0])
    w_minus = np.sum(ranks[nonzero_diff < 0])
    stat = min(w_plus, w_minus)
    n_nonzero = len(nonzero_diff)
    # Asymptotic normal approximation for p-value
    mean_w = n_nonzero * (n_nonzero + 1) / 4.0
    sigma_w = np.sqrt(n_nonzero * (n_nonzero + 1) * (2 * n_nonzero + 1) / 24.0)
    z = (stat - mean_w) / sigma_w if sigma_w > 0 else 0.0
    import math
    p_val = 2.0 * 0.5 * math.erfc(abs(z) / math.sqrt(2))
    return stat, min(max(p_val, 0.0), 1.0)

def analyze_all():
    results = []

    # 1. Analyze Peak RAM from exp1_peak_ram.csv
    exp1_path = os.path.join('bench', 'results', 'exp1_peak_ram.csv')
    if os.path.exists(exp1_path):
        data = {}
        with open(exp1_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for r in reader:
                k = (r['workload'], r['engine'])
                data.setdefault(k, []).append(float(r['peak_ram_bytes']))

        workloads = sorted(list({k[0] for k in data.keys()}))
        for w in workloads:
            base = data.get((w, 'baseline'), [])
            strong = data.get((w, 'strong_baseline'), [])
            two = data.get((w, 'twotier'), [])

            if base and two:
                ratio, ci_l, ci_h = bootstrap_ci(base, two)
                w_stat, p_val = compute_wilcoxon(base, two)
                results.append({
                    'category': 'peak_ram_vs_baseline',
                    'workload': w,
                    'metric': 'RAM (Bytes)',
                    'ratio': ratio,
                    'ci_95_low': ci_l,
                    'ci_95_high': ci_h,
                    'wilcoxon_stat': w_stat,
                    'p_value': p_val,
                    'significant': p_val < 0.05
                })

            if strong and two:
                ratio, ci_l, ci_h = bootstrap_ci(strong, two)
                w_stat, p_val = compute_wilcoxon(strong, two)
                results.append({
                    'category': 'peak_ram_vs_strong_baseline',
                    'workload': w,
                    'metric': 'RAM (Bytes)',
                    'ratio': ratio,
                    'ci_95_low': ci_l,
                    'ci_95_high': ci_h,
                    'wilcoxon_stat': w_stat,
                    'p_value': p_val,
                    'significant': p_val < 0.05
                })

    # 2. Analyze End-to-End Compiler Wall Time from exp11_compiler_e2e.csv
    exp11_path = os.path.join('bench', 'results', 'exp11_compiler_e2e.csv')
    if os.path.exists(exp11_path):
        data = {}
        with open(exp11_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for r in reader:
                k = (r['workload'], r['engine'])
                data.setdefault(k, []).append(float(r['wall_time_us']))

        workloads = sorted(list({k[0] for k in data.keys()}))
        for w in workloads:
            base = data.get((w, 'baseline'), [])
            strong = data.get((w, 'strong_baseline'), [])
            two = data.get((w, 'twotier'), [])

            if base and two:
                ratio, ci_l, ci_h = bootstrap_ci(base, two)
                w_stat, p_val = compute_wilcoxon(base, two)
                results.append({
                    'category': 'e2e_time_vs_baseline',
                    'workload': w,
                    'metric': 'Wall Time (us)',
                    'ratio': ratio,
                    'ci_95_low': ci_l,
                    'ci_95_high': ci_h,
                    'wilcoxon_stat': w_stat,
                    'p_value': p_val,
                    'significant': p_val < 0.05
                })

            if strong and two:
                ratio, ci_l, ci_h = bootstrap_ci(strong, two)
                w_stat, p_val = compute_wilcoxon(strong, two)
                results.append({
                    'category': 'e2e_time_vs_strong_baseline',
                    'workload': w,
                    'metric': 'Wall Time (us)',
                    'ratio': ratio,
                    'ci_95_low': ci_l,
                    'ci_95_high': ci_h,
                    'wilcoxon_stat': w_stat,
                    'p_value': p_val,
                    'significant': p_val < 0.05
                })

    # 3. Analyze Micro-Latency Breakdown from exp4_micro_latency.csv
    exp4_path = os.path.join('bench', 'results', 'exp4_micro_latency.csv')
    if os.path.exists(exp4_path):
        data = {}
        with open(exp4_path, 'r', encoding='utf-8') as f:
            reader = csv.DictReader(f)
            for r in reader:
                k = (r['op_type'], r['engine'])
                data.setdefault(k, []).append(float(r['latency_ns']))

        ops = sorted(list({k[0] for k in data.keys()}))
        for op in ops:
            base = data.get((op, 'baseline'), [])
            strong = data.get((op, 'strong_baseline'), [])
            two = data.get((op, 'twotier'), [])

            if base and two:
                ratio, ci_l, ci_h = bootstrap_ci(base, two)
                w_stat, p_val = compute_wilcoxon(base, two)
                results.append({
                    'category': 'micro_latency_vs_baseline',
                    'workload': op,
                    'metric': 'Latency (ns/op)',
                    'ratio': ratio,
                    'ci_95_low': ci_l,
                    'ci_95_high': ci_h,
                    'wilcoxon_stat': w_stat,
                    'p_value': p_val,
                    'significant': p_val < 0.05
                })

            if strong and two:
                ratio, ci_l, ci_h = bootstrap_ci(strong, two)
                w_stat, p_val = compute_wilcoxon(strong, two)
                results.append({
                    'category': 'micro_latency_vs_strong_baseline',
                    'workload': op,
                    'metric': 'Latency (ns/op)',
                    'ratio': ratio,
                    'ci_95_low': ci_l,
                    'ci_95_high': ci_h,
                    'wilcoxon_stat': w_stat,
                    'p_value': p_val,
                    'significant': p_val < 0.05
                })

    # Save to CSV
    out_csv = os.path.join('bench', 'results', 'statistical_validation.csv')
    os.makedirs(os.path.dirname(out_csv), exist_ok=True)
    with open(out_csv, 'w', newline='', encoding='utf-8') as f:
        fieldnames = ['category', 'workload', 'metric', 'ratio', 'ci_95_low', 'ci_95_high', 'wilcoxon_stat', 'p_value', 'significant']
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)

    print(f"\n[STATISTICS] Successfully written {len(results)} statistical evaluations to '{out_csv}'")
    print(f"{'Category':<28} | {'Item':<28} | {'Ratio':<7} | {'95% CI':<16} | {'p-value':<10} | {'Sig (p<0.05)':<12}")
    print("-" * 110)
    for r in results[:15]:
        ci_str = f"[{r['ci_95_low']:.2f}, {r['ci_95_high']:.2f}]"
        sig_str = "YES" if r['significant'] else "NO"
        print(f"{r['category']:<28} | {r['workload']:<28} | {r['ratio']:<7.2f} | {ci_str:<16} | {r['p_value']:<10.4e} | {sig_str:<12}")

if __name__ == '__main__':
    analyze_all()
