#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."
: "${CHRMODELS_PATH:=/mnt/data/CHRMODELS_clean.dll}"
: "${SONIC_PVM_PATH:=/mnt/data/_sonic_e_test.PVM}"
TMP="${TMPDIR:-/tmp}/sa3ds_0e_test"
rm -rf "$TMP"; mkdir -p "$TMP"
trap 'rm -rf "$TMP"; rm -f runtime_0_0_9_0E/romfs/sa3ds/sonic.sa3c runtime_0_0_9_0E/romfs/sa3ds/sonic.sa3t runtime_0_0_9_0E/romfs/sa3ds/owner_asset_build_report.json' EXIT INT TERM

echo '== inherited 0D gate =='
SA3DS_SKIP_INHERITED=1 CHRMODELS_PATH="$CHRMODELS_PATH" SONIC_PVM_PATH="$SONIC_PVM_PATH" sh tests/run_0_0_9_0D_tests.sh

echo '== owner RomFS generation =='
python3 tools/prepare_3ds_owner_assets.py "$CHRMODELS_PATH" "$SONIC_PVM_PATH" >/dev/null

echo '== core preset sample/draw =='
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_render.c tests/test_0_0_9_0E_presets.c -lm -o "$TMP/presets"
"$TMP/presets" runtime_0_0_9_0E/romfs/sa3ds/sonic.sa3c

echo '== Citro3D source contract =='
# Remove generated packages first so the source-only packaging assertion is real.
rm -f runtime_0_0_9_0E/romfs/sa3ds/sonic.sa3c runtime_0_0_9_0E/romfs/sa3ds/sonic.sa3t runtime_0_0_9_0E/romfs/sa3ds/owner_asset_build_report.json
python3 tests/test_0_0_9_0E_runtime_source.py

echo '0.0.9.0E suite: PASS'
