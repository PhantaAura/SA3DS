#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."
EXE="${1:-${SA3DS_PC2004_EXE:-}}"
SYSTEM_SOURCE="${2:-${SA3DS_SYSTEM_SOURCE:-}}"
if [ -z "$EXE" ] || [ -z "$SYSTEM_SOURCE" ]; then
  echo 'usage: tests/run_0_0_9_1B_tests.sh EXACT_2004_SONIC_EXE SYSTEM_DIR_OR_ZIP' >&2
  exit 2
fi
B="${TMPDIR:-/tmp}/sa3ds_0_0_9_1B_tests"
rm -rf "$B";mkdir -p "$B"
trap 'rm -rf "$B"' EXIT INT TERM

echo '== inherited 0.0.9.1A regression =='
./tests/run_0_0_9_1_tests.sh "$EXE" "$SYSTEM_SOURCE"

echo '== EC visual/material/PVM exact-source decode =='
python3 tests/test_pc2004_emerald_coast_visual.py "$EXE" "$SYSTEM_SOURCE"

echo '== EC visual evidence report =='
python3 tools/audit_emerald_coast_visual.py "$EXE" "$SYSTEM_SOURCE" --json "$B/visual.json" >/dev/null

echo '== owner-side EC visual package builder =='
python3 tools/build_pc2004_emerald_coast_visual.py "$EXE" "$SYSTEM_SOURCE" "$B/out" --report "$B/build.json"
for p in "$B"/out/*.sa3dspak;do python3 tools/sa3dspak.py validate "$p";done

echo '0.0.9.1B Emerald Coast visual/material/PVM suite: PASS'
