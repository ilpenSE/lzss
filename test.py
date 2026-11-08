#!/usr/bin/env python3

import subprocess

def run_cmd(cmd, log=True, stdout=True, stderr=True):
  if log:
    print(f"Command: { ' '.join(cmd) }")
  try:
    stdout_param = None if stdout else subprocess.DEVNULL
    stderr_param = None if stderr else subprocess.DEVNULL
    subprocess.run(cmd, check=True, stdout=stdout_param, stderr=stderr_param)
  except subprocess.CalledProcessError as e:
    print(f"ERROR: Command failed with {e.returncode}")

input_file = "main.c"

# Measure performance
run_cmd(["perf", "stat", "-r", "10", "./build/lzss_kmp", "-e", input_file, "-o", "kmp.enc"])
run_cmd(["perf", "stat", "-r", "10", "./build/lzss_brute", "-e", input_file, "-o", "brute.enc"])

# Confirm kmp's output
run_cmd(["./build/lzss_kmp", "-d", "kmp.enc", "-o", "kmp.dec"])
run_cmd(["diff", input_file, "kmp.dec"]) # Should not be outputting

# Confirm brute's output
run_cmd(["./build/lzss_brute", "-d", "brute.enc", "-o", "brute.dec"])
run_cmd(["diff", input_file, "brute.dec"]) # Should not be outputting
