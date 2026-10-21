#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_dir="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-ec-gpu-XXXXXX")"
trap 'rm -f "$test_dir/test" "$test_dir/fixture.sa3dspak"; rmdir "$test_dir"' EXIT HUP INT TERM
python3 tests/make_emerald_coast_visual_fixture.py "$test_dir/fixture.sa3dspak"
# Native 32-bit allocation guards are tautological on this 64-bit host.
cc -std=c11 -O2 -Wall -Wextra -Werror -Wno-tautological-constant-out-of-range-compare \
  -Itests/emerald_coast_gpu_stubs -Iruntime_0_0_9_0G/include \
  -Isrc/game -Isrc/sa3dspak -Isrc/sa3c \
  tests/test_emerald_coast_gpu_runtime.c \
  runtime_0_0_9_0G/source/emerald_coast_gpu.c \
  src/game/emerald_coast_visual.c src/sa3dspak/sa3dspak.c \
  src/sa3c/sa3t_texture.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_gpu_preflight.c \
  -lm -o "$test_dir/test"
"$test_dir/test" "$test_dir/fixture.sa3dspak"
