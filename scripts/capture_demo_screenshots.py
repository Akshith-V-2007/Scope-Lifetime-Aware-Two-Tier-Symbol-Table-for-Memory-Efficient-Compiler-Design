#!/usr/bin/env python3
"""
Automated Chapter Screenshot Capture for Guided Demo
Serves web/ locally and invokes Chrome in headless 1080p mode to capture
each chapter snapshot into demo/screenshots/.
"""

import os
import sys
import time
import subprocess
import threading
from http.server import HTTPServer, SimpleHTTPRequestHandler

PORT = 8765
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WEB_DIR = os.path.join(BASE_DIR, "web")
OUT_DIR = os.path.join(BASE_DIR, "demo", "screenshots")
os.makedirs(OUT_DIR, exist_ok=True)

CHROME_PATH = r"C:\Program Files\Google\Chrome\Application\chrome.exe"
if not os.path.exists(CHROME_PATH):
    CHROME_PATH = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"

CHAPTER_NAMES = [
    ("chapter_01_problem_baseline.png", 1),
    ("chapter_02_twotier_overview.png", 2),
    ("chapter_03_packed_64bit_record.png", 3),
    ("chapter_04_live_demotion_varint.png", 4),
    ("chapter_05_multitier_lookup.png", 5),
    ("chapter_06_scope_exit_arena.png", 6),
    ("chapter_07_memory_trace_comparison.png", 7),
    ("chapter_08_flight_controller_results.png", 8),
    ("chapter_09_limitations_conclusion.png", 9),
]

class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def log_message(self, format, *args):
        pass # Quiet server logs

def start_server():
    server = HTTPServer(("127.0.0.1", PORT), Handler)
    server.serve_forever()

def main():
    print(f"[SCREENSHOT] Starting background server at http://127.0.0.1:{PORT} serving {WEB_DIR}...")
    server_thread = threading.Thread(target=start_server, daemon=True)
    server_thread.start()
    time.sleep(1.0)

    for filename, ch_num in CHAPTER_NAMES:
        out_path = os.path.join(OUT_DIR, filename)
        url = f"http://127.0.0.1:{PORT}/index.html?ch={ch_num}"
        cmd = [
            CHROME_PATH,
            "--headless=new",
            "--no-sandbox",
            "--disable-gpu",
            "--hide-scrollbars",
            "--window-size=1920,1080",
            f"--screenshot={out_path}",
            url
        ]
        print(f"[SCREENSHOT] Capturing Chapter {ch_num} -> {filename}...")
        try:
            subprocess.run(cmd, check=True, timeout=15)
            if os.path.exists(out_path):
                print(f"  [OK] Saved ({os.path.getsize(out_path)} bytes)")
            else:
                print(f"  [FAIL] Failed: file not generated")
        except Exception as e:
            print(f"  [FAIL] Error capturing {filename}: {e}")

    print("====================================================================")
    print(" All chapter screenshots generated in demo/screenshots/!")
    print("====================================================================")

if __name__ == '__main__':
    main()
