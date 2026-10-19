#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
echo "== frozen 0.0.8.8.9 suite =="
sh tests/run_host_tests.sh
echo "== 0.0.9.0A real-source recon =="
python3 tests/test_pc2004_character_loader_recon.py
python3 tests/test_pc2004_character_gate.py
