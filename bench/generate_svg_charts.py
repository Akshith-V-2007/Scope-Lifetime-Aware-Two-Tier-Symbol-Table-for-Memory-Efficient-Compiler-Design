#!/usr/bin/env python3
"""
generate_svg_charts.py
Generates publication-quality SVG charts for the Technical Report from bench/benchmark_results.csv.
"""

import os
import csv

CSV_PATH = "bench/benchmark_results.csv"

def load_csv_data():
    if not os.path.exists(CSV_PATH):
        raise FileNotFoundError(f"Cannot find {CSV_PATH}. Run bench_runner first.")
    
    rows = []
    with open(CSV_PATH, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for r in reader:
            rows.append(r)
    return rows

def generate_peak_ram_chart(rows):
    # Filter exp1
    exp1_base = {}
    exp1_two = {}
    workload_names = {
        "arduino_sensors.mc": "Arduino Sensors",
        "freertos_tasks.mc": "FreeRTOS Tasks",
        "zephyr_network.mc": "Zephyr Network",
        "mibench_telecom.mc": "MiBench Telecom",
        "embedded_flight_controller.mc": "Flight Controller",
        "synthetic_broad_scope.mc": "Broad Scopes",
        "synthetic_heavy_shadowing.mc": "Heavy Shadowing",
        "synthetic_deep_nesting.mc": "Deep Nesting",
    }

    for r in rows:
        if r["experiment"] == "exp1":
            wl = r["workload"]
            eng = r["engine"]
            ram = int(r["peak_ram_bytes"])
            if eng == "baseline":
                exp1_base[wl] = ram
            elif eng == "twotier":
                exp1_two[wl] = ram

    data = []
    for wl, display in workload_names.items():
        if wl in exp1_base and wl in exp1_two:
            data.append((display, exp1_base[wl], exp1_two[wl]))

    width = 840
    height = 520
    margin_left = 180
    margin_bottom = 60
    margin_top = 60
    margin_right = 50

    chart_w = width - margin_left - margin_right
    chart_h = height - margin_top - margin_bottom

    max_val = 42000

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="100%" height="100%" style="background:#0f172a; font-family:system-ui, -apple-system, sans-serif; border-radius:12px;">')
    
    # Title
    svg.append(f'<text x="{width/2}" y="34" fill="#f8fafc" font-size="18" font-weight="bold" text-anchor="middle">Peak Symbol Table Memory: Baseline vs Two-Tier (Bytes)</text>')
    
    # Grid lines
    for step in [0, 10000, 20000, 30000, 40000]:
        x = margin_left + (step / max_val) * chart_w
        svg.append(f'<line x1="{x}" y1="{margin_top}" x2="{x}" y2="{margin_top + chart_h}" stroke="#334155" stroke-dasharray="4" />')
        svg.append(f'<text x="{x}" y="{margin_top + chart_h + 18}" fill="#94a3b8" font-size="11" text-anchor="middle">{step:,} B</text>')

    bar_group_h = chart_h / len(data)
    bar_h = bar_group_h * 0.32

    for i, (label, base_val, two_val) in enumerate(data):
        y_base = margin_top + i * bar_group_h + 8
        y_two = y_base + bar_h + 3

        w_base = (base_val / max_val) * chart_w
        w_two = (two_val / max_val) * chart_w

        # Label
        svg.append(f'<text x="{margin_left - 12}" y="{y_base + bar_h}" fill="#e2e8f0" font-size="12" font-weight="500" text-anchor="end">{label}</text>')

        # Baseline bar (coral)
        svg.append(f'<rect x="{margin_left}" y="{y_base}" width="{w_base}" height="{bar_h}" fill="#f43f5e" rx="3" />')
        svg.append(f'<text x="{margin_left + w_base + 8}" y="{y_base + bar_h - 2}" fill="#fca5a5" font-size="11">{base_val:,} B</text>')

        # Two-Tier bar (emerald if saving, amber if higher)
        pct = (1.0 - two_val / base_val) * 100.0
        bar_col = "#10b981" if pct >= 0 else "#f59e0b"
        text_col = "#6ee7b7" if pct >= 0 else "#fcd34d"
        pct_str = f"-{pct:.1f}%" if pct >= 0 else f"+{abs(pct):.1f}%"

        svg.append(f'<rect x="{margin_left}" y="{y_two}" width="{w_two}" height="{bar_h}" fill="{bar_col}" rx="3" />')
        svg.append(f'<text x="{margin_left + w_two + 8}" y="{y_two + bar_h - 2}" fill="{text_col}" font-size="11">{two_val:,} B ({pct_str})</text>')

    # Legend
    svg.append(f'<rect x="250" y="{height - 22}" width="14" height="12" fill="#f43f5e" rx="2" />')
    svg.append(f'<text x="272" y="{height - 12}" fill="#cbd5e1" font-size="11">Baseline (Malloc Stack)</text>')
    svg.append(f'<rect x="460" y="{height - 22}" width="14" height="12" fill="#10b981" rx="2" />')
    svg.append(f'<text x="482" y="{height - 12}" fill="#cbd5e1" font-size="11">Two-Tier Scope-Aware</text>')

    svg.append('</svg>')

    os.makedirs("report", exist_ok=True)
    with open("report/chart_peak_ram.svg", "w", encoding="utf-8") as f:
        f.write("\n".join(svg))
    print("[CHART] Generated 'report/chart_peak_ram.svg'")

def generate_tradeoff_curve(rows):
    # Filter exp3_sweep_k
    pts = []
    for r in rows:
        if r["experiment"] == "exp3_sweep_k":
            k = int(r["workload"])
            ram = int(r["peak_ram_bytes"])
            decomps = int(r["decomps"])
            pts.append((k, ram, decomps))

    width = 750
    height = 400
    ml, mr, mt, mb = 80, 80, 60, 60
    cw = width - ml - mr
    ch = height - mt - mb

    max_ram = 20000
    max_decomps = 85

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="100%" height="100%" style="background:#0f172a; font-family:system-ui, sans-serif; border-radius:12px;">')
    svg.append(f'<text x="{width/2}" y="32" fill="#f8fafc" font-size="17" font-weight="bold" text-anchor="middle">Pareto Trade-off Curve: Hot Depth K vs Peak RAM &amp; Decompressions</text>')

    # Grid
    for step in [0, 5000, 10000, 15000, 20000]:
        y = mt + ch - (step / max_ram) * ch
        svg.append(f'<line x1="{ml}" y1="{y}" x2="{width - mr}" y2="{y}" stroke="#334155" stroke-dasharray="4" />')
        svg.append(f'<text x="{ml - 10}" y="{y + 4}" fill="#38bdf8" font-size="11" text-anchor="end">{step:,} B</text>')

    # Decomp right axis
    for d_step in [0, 20, 40, 60, 80]:
        y = mt + ch - (d_step / max_decomps) * ch
        svg.append(f'<text x="{width - mr + 12}" y="{y + 4}" fill="#fbbf24" font-size="11" text-anchor="start">{d_step}</text>')

    # Axes labels
    svg.append(f'<text x="{ml - 40}" y="{mt - 15}" fill="#38bdf8" font-size="12" font-weight="bold">RAM (B)</text>')
    svg.append(f'<text x="{width - mr + 40}" y="{mt - 15}" fill="#fbbf24" font-size="12" font-weight="bold" text-anchor="end">Decompressions</text>')

    # Generate points & paths
    ram_coords = []
    decomp_coords = []
    num_pts = len(pts)

    for i, (k, ram, decomps) in enumerate(pts):
        x = ml + (i / (num_pts - 1)) * cw
        y_ram = mt + ch - (ram / max_ram) * ch
        y_decomp = mt + ch - (decomps / max_decomps) * ch
        ram_coords.append((x, y_ram, k, ram))
        decomp_coords.append((x, y_decomp, k, decomps))

        # X-axis label
        svg.append(f'<text x="{x}" y="{height - mb + 20}" fill="#94a3b8" font-size="11" text-anchor="middle">K={k}</text>')

    # Draw lines
    path_ram = "M " + " L ".join([f"{x},{y}" for x, y, _, _ in ram_coords])
    path_decomp = "M " + " L ".join([f"{x},{y}" for x, y, _, _ in decomp_coords])

    svg.append(f'<path d="{path_ram}" fill="none" stroke="#38bdf8" stroke-width="3" />')
    svg.append(f'<path d="{path_decomp}" fill="none" stroke="#fbbf24" stroke-width="3" stroke-dasharray="6,4" />')

    # Draw dots
    for x, y, k, val in ram_coords:
        svg.append(f'<circle cx="{x}" cy="{y}" r="5" fill="#38bdf8" />')
        svg.append(f'<text x="{x}" y="{y - 8}" fill="#bae6fd" font-size="10" text-anchor="middle">{val:,} B</text>')

    for x, y, k, val in decomp_coords:
        svg.append(f'<circle cx="{x}" cy="{y}" r="5" fill="#fbbf24" />')
        svg.append(f'<text x="{x}" y="{y + 16}" fill="#fde68a" font-size="10" text-anchor="middle">{val}</text>')

    # Legend
    svg.append(f'<circle cx="270" cy="{height - 15}" r="5" fill="#38bdf8" />')
    svg.append(f'<text x="282" y="{height - 11}" fill="#cbd5e1" font-size="12">Peak RAM (Bytes)</text>')
    svg.append(f'<line x1="430" y1="{height - 15}" x2="455" y2="{height - 15}" stroke="#fbbf24" stroke-width="3" stroke-dasharray="4,2" />')
    svg.append(f'<text x="463" y="{height - 11}" fill="#cbd5e1" font-size="12">Cold Decompressions</text>')

    svg.append('</svg>')

    with open("report/chart_tradeoff_curve.svg", "w", encoding="utf-8") as f:
        f.write("\n".join(svg))
    print("[CHART] Generated 'report/chart_tradeoff_curve.svg'")

def generate_ablation_waterfall(rows):
    # Filter exp2_ablation
    stages = []
    colors = ["#f43f5e", "#fb923c", "#facc15", "#a3e635", "#34d399", "#10b981"]
    
    idx = 0
    for r in rows:
        if r["experiment"] == "exp2_ablation":
            name = r["engine"]
            val = int(r["peak_ram_bytes"])
            col = colors[idx % len(colors)]
            stages.append((name, val, col))
            idx += 1

    width = 750
    height = 380
    ml, mr, mt, mb = 60, 40, 50, 60
    cw = width - ml - mr
    ch = height - mt - mb

    max_val = 42000

    svg = []
    svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" width="100%" height="100%" style="background:#0f172a; font-family:system-ui, sans-serif; border-radius:12px;">')
    svg.append(f'<text x="{width/2}" y="30" fill="#f8fafc" font-size="17" font-weight="bold" text-anchor="middle">Ablation Waterfall: Memory Reduction by Mechanism (Bytes)</text>')

    for step in [0, 10000, 20000, 30000, 40000]:
        y = mt + ch - (step / max_val) * ch
        svg.append(f'<line x1="{ml}" y1="{y}" x2="{width - mr}" y2="{y}" stroke="#334155" stroke-dasharray="4" />')
        svg.append(f'<text x="{ml - 10}" y="{y + 4}" fill="#94a3b8" font-size="11" text-anchor="end">{step:,} B</text>')

    col_w = cw / len(stages)
    bar_w = col_w * 0.65

    for i, (name, val, col) in enumerate(stages):
        x = ml + i * col_w + (col_w - bar_w) / 2
        bar_h = (val / max_val) * ch
        y = mt + ch - bar_h

        svg.append(f'<rect x="{x}" y="{y}" width="{bar_w}" height="{bar_h}" fill="{col}" rx="4" />')
        svg.append(f'<text x="{x + bar_w/2}" y="{y - 8}" fill="#f8fafc" font-size="11" font-weight="bold" text-anchor="middle">{val:,} B</text>')

        # Label
        parts = name.split(". ")
        lbl = parts[1] if len(parts) > 1 else name
        svg.append(f'<text x="{x + bar_w/2}" y="{height - mb + 18}" fill="#94a3b8" font-size="11" text-anchor="middle">{lbl}</text>')

    svg.append('</svg>')

    with open("report/chart_ablation_waterfall.svg", "w", encoding="utf-8") as f:
        f.write("\n".join(svg))
    print("[CHART] Generated 'report/chart_ablation_waterfall.svg'")

if __name__ == "__main__":
    data = load_csv_data()
    generate_peak_ram_chart(data)
    generate_tradeoff_curve(data)
    generate_ablation_waterfall(data)
