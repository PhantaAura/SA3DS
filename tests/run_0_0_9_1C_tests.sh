#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."
EXE="${1:-${SA3DS_PC2004_EXE:-}}"
SYSTEM_SOURCE="${2:-${SA3DS_SYSTEM_SOURCE:-}}"
if [ -z "$EXE" ] || [ -z "$SYSTEM_SOURCE" ]; then
  echo 'usage: tests/run_0_0_9_1C_tests.sh EXACT_2004_SONIC_EXE SYSTEM_DIR_OR_ZIP' >&2
  exit 2
fi
B="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds_1C_XXXXXXXX")"
trap 'rm -rf "$B"' EXIT INT TERM

echo '== inherited 0.0.9.1B regression =='
./tests/run_0_0_9_1B_tests.sh "$EXE" "$SYSTEM_SOURCE"
echo '== SET synthetic and owner-data decode =='
python3 tests/test_pc2004_set.py "$SYSTEM_SOURCE" "$EXE"
echo '== owner-side embedded SET + EC visual packages =='
python3 tools/build_pc2004_emerald_coast_visual.py "$EXE" "$SYSTEM_SOURCE" "$B/EC" --with-set --report "$B/ec.json"
for p in "$B"/EC/*.sa3dspak; do python3 tools/sa3dspak.py validate "$p"; done
echo '== whole-game SET catalog (placement only) =='
python3 tools/build_pc2004_set_packages.py "$SYSTEM_SOURCE" "$B/ALL" --report "$B/full_set_report.json"
for p in "$B"/ALL/*.sa3dspak; do python3 tools/sa3dspak.py validate "$p" >/dev/null; done
printf '%s\n' '0.0.9.1C SET + object-placement suite: PASS'
