#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."
EXE="${1:-${SA3DS_PC2004_EXE:-}}"
SYSTEM_SOURCE="${2:-${SA3DS_SYSTEM_SOURCE:-}}"
if [ -z "$EXE" ]; then
  echo 'usage: tests/run_0_0_9_1_tests.sh EXACT_2004_SONIC_EXE [SYSTEM_DIR_OR_ZIP]' >&2
  exit 2
fi
B="${TMPDIR:-/tmp}/sa3ds_0_0_9_1_tests"
rm -rf "$B"; mkdir -p "$B"
trap 'rm -rf "$B"' EXIT INT TERM

echo '== inherited 0.0.9.0G regression =='
./tests/run_0_0_9_0G_tests.sh

echo '== exact PC2004 Emerald Coast root/code witnesses =='
python3 tools/verify_pc2004_emerald_coast_roots.py "$EXE" --json "$B/roots.json" >/dev/null

echo '== exact PC2004 Emerald Coast structural decode =='
python3 tests/test_pc2004_emerald_coast_decode.py "$EXE"

echo '== source-neutral collision chunk =='
python3 tests/test_ec_collision_chunk.py "$EXE"
python3 tools/build_pc2004_emerald_coast_collision.py "$EXE" "$B/emerald_coast.sa3dspak" --report "$B/ec.json" >/dev/null
python3 tools/sa3dspak.py validate "$B/emerald_coast.sa3dspak"
python3 tools/sa3dspak.py inspect "$B/emerald_coast.sa3dspak" >/dev/null

if [ -n "$SYSTEM_SOURCE" ]; then
  echo '== Emerald Coast system companion inventory =='
  python3 tools/audit_emerald_coast_system.py "$SYSTEM_SOURCE" --json "$B/system.json" >/dev/null
fi

echo '0.0.9.1 Emerald Coast source-decode suite: PASS'
