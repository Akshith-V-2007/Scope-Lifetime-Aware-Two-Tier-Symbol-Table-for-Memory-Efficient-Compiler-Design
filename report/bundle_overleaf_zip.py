#!/usr/bin/env python3
"""
Bundle Overleaf ZIP for Manuscript Submission
Packages main.tex, figures/, references, and compilation instructions.
"""

import os
import zipfile
import shutil

REPORT_DIR = os.path.dirname(os.path.abspath(__file__))
ZIP_PATH = os.path.join(REPORT_DIR, "manuscript_overleaf.zip")
SRC_TEX = os.path.join(REPORT_DIR, "MANUSCRIPT.tex")
FIGURES_DIR = os.path.join(REPORT_DIR, "figures")

README_CONTENT = """# Scope-Lifetime-Aware Two-Tier Symbol Table - Overleaf Compilation Guide

## Files Included
- `main.tex`: Full research manuscript and architectural monograph.
- `figures/`: Publication-quality IEEE vector (PDF) and 300 DPI raster (PNG) charts.
- `references.bib`: BibTeX citations for dragon book, Knuth TAOCP, Bloom filters, etc.

## How to Compile on Overleaf
1. Log in to [Overleaf](https://www.overleaf.com/).
2. Click **New Project** -> **Upload Project**.
3. Select this `manuscript_overleaf.zip` archive.
4. Set compiler to **pdfLaTeX** or **LaTeX** (default is pdfLaTeX, TeX Live 2024+).
5. Click **Recompile**.

## Local Compilation
```bash
pdflatex main.tex
bibtex main || true
pdflatex main.tex
pdflatex main.tex
```
"""

BIB_CONTENT = r"""@book{dragon,
  author    = {Alfred V. Aho and Monica S. Lam and Ravi Sethi and Jeffrey D. Ullman},
  title     = {Compilers: Principles, Techniques, and Tools},
  edition   = {2nd},
  publisher = {Addison-Wesley},
  year      = {2006}
}

@book{knuth,
  author    = {Donald E. Knuth},
  title     = {The Art of Computer Programming, Volume 3: Sorting and Searching},
  edition   = {2nd},
  publisher = {Addison-Wesley},
  year      = {1998}
}

@article{bloom,
  author    = {Burton H. Bloom},
  title     = {Space/Time Trade-offs in Hash Coding with Allowable Errors},
  journal   = {Communications of the ACM},
  volume    = {13},
  number    = {7},
  pages     = {422--426},
  year      = {1970}
}

@book{navarro,
  author    = {Gonzalo Navarro},
  title     = {Compact Data Structures: A Practical Approach},
  publisher = {Cambridge University Press},
  year      = {2016}
}

@inproceedings{llvm,
  author    = {Chris Lattner and Vikram Adve},
  title     = {{LLVM}: A Compilation Framework for Lifelong Program Analysis \& Transformation},
  booktitle = {Proceedings of the International Symposium on Code Generation and Optimization (CGO)},
  pages     = {75--86},
  year      = {2004}
}
"""

def create_zip():
    with zipfile.ZipFile(ZIP_PATH, 'w', zipfile.ZIP_DEFLATED) as zf:
        # 1. main.tex at root
        zf.write(SRC_TEX, arcname="main.tex")
        # 2. references.bib
        zf.writestr("references.bib", BIB_CONTENT)
        # 3. README.md
        zf.writestr("README.md", README_CONTENT)

        # 4. Figures
        if os.path.exists(FIGURES_DIR):
            for fname in os.listdir(FIGURES_DIR):
                fpath = os.path.join(FIGURES_DIR, fname)
                if os.path.isfile(fpath) and (fname.endswith('.png') or fname.endswith('.pdf')):
                    zf.write(fpath, arcname=f"figures/{fname}")

    print(f"[ZIP] Successfully generated Overleaf submission archive at: {ZIP_PATH}")
    print(f"[ZIP] Size: {os.path.getsize(ZIP_PATH)} bytes")

if __name__ == '__main__':
    create_zip()
