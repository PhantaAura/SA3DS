#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

: "${CHRMODELS_PATH:=/mnt/data/CHRMODELS_clean.dll}"
: "${SONIC_PVM_PATH:=/mnt/data/_sa3ds_real_sonic_source/SONIC.PVM}"

echo "== inherited 0.0.9.0B suite =="
sh tests/run_0_0_9_0B_tests.sh

echo "== 0.0.9.0C exact real Sonic source decode =="
if [ ! -f "$CHRMODELS_PATH" ]; then
  echo "missing exact CHRMODELS source: $CHRMODELS_PATH" >&2
  exit 1
fi
if [ ! -f "$SONIC_PVM_PATH" ]; then
  echo "missing exact SONIC.PVM source: $SONIC_PVM_PATH" >&2
  exit 1
fi
python3 tests/test_pc2004_chrmodels_decode.py "$CHRMODELS_PATH"
python3 tests/test_pc2004_sonic_pvm.py "$SONIC_PVM_PATH"
python3 tests/test_pc2004_sonic_material_texture_binding.py "$CHRMODELS_PATH" "$SONIC_PVM_PATH"
python3 tests/test_pc2004_sonic_sa3c_build.py "$CHRMODELS_PATH" "$SONIC_PVM_PATH"

build_dir="${TMPDIR:-/tmp}/sa3ds_real_sonic_0_0_9_0C"
mkdir -p "$build_dir"
python3 tools/build_pc2004_sonic_sa3c.py "$CHRMODELS_PATH" "$SONIC_PVM_PATH" "$build_dir/sonic.sa3c" --report "$build_dir/sonic_sa3c_report.json" >/dev/null
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c src/sa3c/sa3c_character.c tests/test_sa3c_character.c -o "$build_dir/test_sa3c_character"
"$build_dir/test_sa3c_character" "$build_dir/sonic.sa3c"
rm -f "$build_dir/sonic.sa3c"

echo "0.0.9.0C suite: PASS"
