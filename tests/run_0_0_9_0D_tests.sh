#!/bin/sh
set -eu
# Frozen 0.0.8.8.9 unfortunately contains one historical .pyc; keep test
# execution from rewriting cache bytes that are part of that legacy hash set.
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."

: "${CHRMODELS_PATH:=/mnt/data/CHRMODELS_clean.dll}"
: "${SONIC_PVM_PATH:=/mnt/data/_sa3ds_real_sonic_source/SONIC.PVM}"

build_dir="${TMPDIR:-/tmp}/sa3ds_real_sonic_0_0_9_0D"
rm -rf "$build_dir"
mkdir -p "$build_dir"
cleanup() { rm -rf "$build_dir"; }
trap cleanup EXIT INT TERM

if [ "${SA3DS_SKIP_INHERITED:-0}" != "1" ]; then
  echo "== inherited 0.0.9.0C suite =="
  CHRMODELS_PATH="$CHRMODELS_PATH" SONIC_PVM_PATH="$SONIC_PVM_PATH" sh tests/run_0_0_9_0C_tests.sh
else
  echo "== inherited suite skipped by SA3DS_SKIP_INHERITED=1 =="
fi

echo "== 0.0.9.0D source rendering semantics =="
python3 tests/test_pc2004_sonic_render_semantics.py "$CHRMODELS_PATH"

echo "== generated owner-source packages =="
python3 tools/build_pc2004_sonic_sa3c.py "$CHRMODELS_PATH" "$SONIC_PVM_PATH" "$build_dir/sonic.sa3c" --report "$build_dir/sa3c.json" >/dev/null
python3 tools/build_pc2004_sonic_sa3t.py "$SONIC_PVM_PATH" "$build_dir/sonic.sa3t" --report "$build_dir/sa3t.json" >/dev/null

echo "== native SA3C pose/draw submission =="
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_render.c \
  tests/test_real_sonic_white_space.c -lm -o "$build_dir/test_real_sonic_white_space"
"$build_dir/test_real_sonic_white_space" "$build_dir/sonic.sa3c"

echo "== native SA3T texture package =="
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3t_texture.c tests/test_sa3t_texture.c \
  -o "$build_dir/test_sa3t_texture"
"$build_dir/test_sa3t_texture" "$build_dir/sonic.sa3t"

echo "== host textured white-space raster smoke =="
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_render.c src/sa3c/sa3t_texture.c \
  tools/host_render_sa3c_white_space.c -lm -o "$build_dir/host_render"
"$build_dir/host_render" "$build_dir/sonic.sa3c" "$build_dir/sonic.sa3t" "$build_dir/action0.ppm" 0 0
"$build_dir/host_render" "$build_dir/sonic.sa3c" "$build_dir/sonic.sa3t" "$build_dir/walk0.ppm" 9 0
"$build_dir/host_render" "$build_dir/sonic.sa3c" "$build_dir/sonic.sa3t" "$build_dir/walk5.ppm" 9 5
if cmp -s "$build_dir/walk0.ppm" "$build_dir/walk5.ppm"; then
  echo "real animation raster did not change between walk frames" >&2
  exit 1
fi

# No generated Sega-derived package or raster leaves the temporary directory.
echo "0.0.9.0D suite: PASS"
