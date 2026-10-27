#!/usr/bin/env python3
import argparse
import random

BASE_NAMES = [
    "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta", "theta",
    "iota", "kappa", "lambda", "mu", "nu", "xi", "omicron", "pi",
    "rho", "sigma", "tau", "upsilon", "phi", "chi", "psi", "omega",
    "counter", "index", "buffer", "pointer", "temp", "status", "flags",
    "total", "result", "sensor_val", "cache_line", "timer_reg", "baud_rate"
]

def generate_synthetic(depth, symbols_per_scope, shadowing_rate, output_path):
    lines = []
    lines.append("// Synthetically Generated Benchmark Suite")
    lines.append(f"// Parameters: depth={depth}, symbols_per_scope={symbols_per_scope}, shadowing_rate={shadowing_rate:.2f}")
    lines.append("int g_root_state = 1;")
    lines.append("int g_benchmark_epoch = 2026;")
    lines.append("")
    lines.append("int benchmark_workload(int input_arg) {")
    lines.append("    int workload_accum = input_arg;")

    active_symbols_by_depth = []

    # Build nested blocks to target depth
    for d in range(1, depth + 1):
        indent = "    " * d
        lines.append(f"{indent}{{")
        current_scope_names = []

        for s in range(symbols_per_scope):
            # Check shadowing
            is_shadow = (random.random() < shadowing_rate) and any(active_symbols_by_depth)
            if is_shadow:
                # Pick a name from an ancestor depth
                ancestor_names = [name for scope in active_symbols_by_depth for name in scope]
                if ancestor_names:
                    chosen_name = random.choice(ancestor_names)
                else:
                    chosen_name = f"var_d{d}_s{s}"
            else:
                chosen_name = f"var_d{d}_s{s}"

            # Ensure unique in current scope
            if chosen_name in current_scope_names:
                chosen_name = f"var_d{d}_s{s}_{len(current_scope_names)}"

            current_scope_names.append(chosen_name)
            val = (d * 100) + s
            lines.append(f"{indent}    int {chosen_name} = {val};")

        # Perform lookups
        all_visible = [name for scope in active_symbols_by_depth for name in scope] + current_scope_names
        num_lookups = min(5, len(all_visible))
        for _ in range(num_lookups):
            target = random.choice(all_visible)
            lines.append(f"{indent}    workload_accum = workload_accum + {target};")

        active_symbols_by_depth.append(current_scope_names)

    # Close nested blocks
    for d in range(depth, 0, -1):
        indent = "    " * d
        lines.append(f"{indent}}}")

    lines.append("    return workload_accum;")
    lines.append("}")
    lines.append("")
    lines.append("int main() {")
    lines.append("    int res = benchmark_workload(10);")
    lines.append("    return res;")
    lines.append("}")

    code = "\n".join(lines)
    with open(output_path, "w") as f:
        f.write(code)
    print(f"[SYNTHETIC] Generated '{output_path}' (depth={depth}, symbols={symbols_per_scope}, shadowing={shadowing_rate})")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--depth", type=int, default=32, help="Nesting depth")
    parser.add_argument("--symbols", type=int, default=8, help="Symbols per scope")
    parser.add_argument("--shadowing", type=float, default=0.25, help="Shadowing rate [0.0 - 1.0]")
    parser.add_argument("--output", type=str, default="data/synthetic_custom.mc", help="Output file path")
    args = parser.parse_args()

    generate_synthetic(args.depth, args.symbols, args.shadowing, args.output)
