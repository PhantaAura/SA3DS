#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
    src/sa3c/sa3c_gpu_batch.c tests/test_sa3c_gpu_batch.c \
    -o "$TMP/test_sa3c_gpu_batch"
"$TMP/test_sa3c_gpu_batch"
python3 tests/test_white_zone_gpu_batch_source.py
