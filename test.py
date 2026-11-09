#!/usr/bin/env python3

import subprocess

def run_cmd(cmd, log=True, stdout=True, stderr=True) -> int:
  if log:
    print(f"Command: { ' '.join(cmd) }")
  try:
    stdout_param = None if stdout else subprocess.DEVNULL
    stderr_param = None if stderr else subprocess.DEVNULL
    subprocess.run(cmd, check=True, stdout=stdout_param, stderr=stderr_param)
  except subprocess.CalledProcessError as e:
    if log:
      print(f"ERROR: Command failed with {e.returncode}")
    return e.returncode
  return 0

input_files = ["main.c", "lorem.txt"]
algs = ["brute", "kmp", "hash_chain"]
binary = "./build/lzss"
trash_folder = "./trash"

for input_file in input_files:
  for alg in algs:
    run_cmd(["perf", "stat", "-r", "10", binary, "--encode", "--algorithm", alg, input_file, "-o", f"{trash_folder}/{alg}.enc"])
    run_cmd([f"./build/lzss", "--decode", f"{trash_folder}/{alg}.enc", "-o", f"{trash_folder}/{alg}.dec"])
    if run_cmd(["diff", input_file, f"{trash_folder}/{alg}.dec"], stdout=False, stderr=False) != 0:
      print(f"ERROR: {alg} is failing for file '{input_file}'")
