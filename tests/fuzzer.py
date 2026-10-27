#!/usr/bin/env python3
import random
import subprocess
import os
import sys

SAMPLE_VARS = ["a", "b", "c", "x", "y", "z", "counter", "sum", "val", "index", "buffer", "state"]
TYPES = ["int", "float", "char"]

def generate_random_block(depth, max_depth, declared_stack):
    lines = []
    lines.append("    " * depth + "{")
    
    current_scope = set()
    declared_stack.append(current_scope)
    
    num_stmts = random.randint(2, 6)
    for _ in range(num_stmts):
        choice = random.random()
        indent = "    " * (depth + 1)
        
        if choice < 0.4:
            # Variable declaration
            vname = random.choice(SAMPLE_VARS)
            tname = random.choice(TYPES)
            val = random.randint(1, 100)
            if vname not in current_scope:
                current_scope.add(vname)
                lines.append(f"{indent}{tname} {vname} = {val};")
        elif choice < 0.7:
            # Assignment to an accessible variable
            all_visible = [v for s in declared_stack for v in s]
            if all_visible:
                vname = random.choice(all_visible)
                val = random.randint(1, 50)
                lines.append(f"{indent}{vname} = {val};")
        elif choice < 0.9 and depth < max_depth:
            # Nested inner block
            lines.extend(generate_random_block(depth + 1, max_depth, declared_stack))
        else:
            # Expression evaluation / lookup
            all_visible = [v for s in declared_stack for v in s]
            if all_visible:
                v1 = random.choice(all_visible)
                lines.append(f"{indent}{v1} = {v1} + 1;")

    declared_stack.pop()
    lines.append("    " * depth + "}")
    return lines

def generate_random_program():
    lines = []
    lines.append("// Randomly Generated Mini-C Fuzz Program")
    lines.append("int g_sys_mode = 1;")
    lines.append("float g_calib_factor = 2.5;")
    lines.append("")
    
    # Generate 1-2 helper functions
    num_funcs = random.randint(1, 3)
    for f in range(num_funcs):
        lines.append(f"int func_worker_{f}(int arg0) {{")
        declared_stack = [{"g_sys_mode", "g_calib_factor", "arg0"}]
        body = generate_random_block(1, random.randint(3, 8), declared_stack)
        lines.extend(body)
        lines.append("    return arg0;")
        lines.append("}")
        lines.append("")
        
    lines.append("int main() {")
    declared_stack = [{"g_sys_mode", "g_calib_factor"}]
    main_body = generate_random_block(1, random.randint(4, 10), declared_stack)
    lines.extend(main_body)
    lines.append("    return 0;")
    lines.append("}")
    return "\n".join(lines)

def run_fuzz_tests(iterations=25):
    print(f"=== Starting Mini-C Symbol Table Differential Fuzzing ({iterations} rounds) ===")
    os.makedirs("tmp", exist_ok=True)
    
    exe_path = "./minicc.exe" if os.path.exists("./minicc.exe") else "./minicc"
    for i in range(1, iterations + 1):
        code = generate_random_program()
        test_file = f"tmp/fuzz_{i}.mc"
        with open(test_file, "w") as f:
            f.write(code)
            
        # Run Baseline with --dump-resolutions
        cmd_base = [exe_path, "--symtab", "baseline", "--dump-resolutions", test_file]
        res_base = subprocess.run(cmd_base, capture_output=True, text=True)
        
        # Run Two-Tier with --dump-resolutions
        cmd_two = [exe_path, "--symtab", "twotier", "--dump-resolutions", test_file]
        res_two = subprocess.run(cmd_two, capture_output=True, text=True)
        
        # Verify identical exit codes
        assert res_base.returncode == res_two.returncode, \
            f"Fuzz test {i} returncode mismatch: baseline={res_base.returncode}, twotier={res_two.returncode}\nBase out: {res_base.stderr}\nTwo out: {res_two.stderr}"
            
        # Verify bitwise identical symbol resolution traces (type, offset, depth, scope)
        assert res_base.stdout == res_two.stdout, \
            f"Fuzz test {i} resolution trace mismatch!\nBaseline:\n{res_base.stdout}\nTwo-Tier:\n{res_two.stdout}"
            
        print(f"  [Round {i:2d}/{iterations}] OK: {test_file} (exit={res_base.returncode}, resolutions matched 100%)")
        
    print("\n[SUCCESS] All fuzzing iterations passed with 100% differential consistency!\n")
    import shutil
    shutil.rmtree("tmp", ignore_errors=True)

if __name__ == "__main__":
    run_fuzz_tests(30)
