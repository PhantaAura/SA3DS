#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SYS="${1:-}"
EXE="${2:-}"
cd "$ROOT"
python3 tests/test_pc2004_cam.py
if [[ -n "$SYS" ]]; then
  if [[ -n "$EXE" ]];then python3 tests/test_pc2004_cam.py "$SYS" "$EXE";
  else python3 tests/test_pc2004_cam.py "$SYS";fi
fi
