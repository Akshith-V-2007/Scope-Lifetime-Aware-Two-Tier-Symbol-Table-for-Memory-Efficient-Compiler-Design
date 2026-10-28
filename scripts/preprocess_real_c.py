#!/usr/bin/env python3
"""
Preprocess Real C Sources for Mini-C (minicc)
Strips preprocessor directives (#include, #define, #ifdef), macro expansions,
and unsupported GCC-specific attributes while preserving original variable declarations,
lexical scopes, pointer declarators, and control flow.

Usage:
    python scripts/preprocess_real_c.py input_file.c output_file.mc
"""

import sys
import re

def preprocess_c_to_mc(input_path, output_path):
    with open(input_path, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    lines = content.splitlines()
    processed_lines = []
    in_block_comment = False

    for line in lines:
        stripped = line.strip()

        # Handle preprocessor directives
        if stripped.startswith('#'):
            processed_lines.append(f"// [STRIPPED PREPROCESSOR]: {stripped}")
            continue

        # Strip compiler attributes e.g. __attribute__((...))
        line = re.sub(r'__attribute__\s*\(\(.*?\)\)', '', line)
        line = re.sub(r'__declspec\s*\(.*?\)', '', line)
        line = re.sub(r'\bstatic\b', '', line)
        line = re.sub(r'\bconst\b', '', line)
        line = re.sub(r'\bvolatile\b', '', line)
        line = re.sub(r'\bextern\b', '', line)
        line = re.sub(r'\bregister\b', '', line)

        # Replace standard typedef types with primitive equivalents for minicc
        line = re.sub(r'\buint32_t\b|\bint32_t\b|\bsize_t\b|\buint16_t\b|\bint16_t\b|\buint8_t\b|\bint8_t\b', 'int', line)
        line = re.sub(r'\bBaseType_t\b|\bUBaseType_t\b|\bTickType_t\b|\bk_spinlock_key_t\b', 'int', line)
        line = re.sub(r'\bbool\b', 'int', line)

        processed_lines.append(line)

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(f"// Preprocessed from {input_path} for Mini-C compiler\n")
        f.write("// Automatic preprocessor stripping and type normalization applied\n\n")
        f.write("\n".join(processed_lines) + "\n")

    print(f"[PREPROCESS] Processed '{input_path}' -> '{output_path}'")

def main():
    if len(sys.argv) < 3:
        print("Usage: python scripts/preprocess_real_c.py <input.c> <output.mc>")
        sys.exit(1)
    preprocess_c_to_mc(sys.argv[1], sys.argv[2])

if __name__ == '__main__':
    main()
