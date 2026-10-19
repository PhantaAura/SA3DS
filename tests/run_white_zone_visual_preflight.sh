#!/bin/sh
set -eu
export PYTHONDONTWRITEBYTECODE=1
cd "$(dirname "$0")/.."
: "${CHRMODELS_PATH:?Set CHRMODELS_PATH to user-owned original CHRMODELS.DLL}"
: "${SONIC_PVM_PATH:?Set SONIC_PVM_PATH to user-owned SONIC.PVM}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
python3 tools/prepare_3ds_owner_assets.py "$CHRMODELS_PATH" "$SONIC_PVM_PATH" --runtime "$TMP" >/dev/null
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
    src/sa3c/sa3c_character.c src/sa3c/sa3t_texture.c \
    src/sa3c/sa3c_gpu_preflight.c tests/test_sa3c_gpu_preflight.c \
    -o "$TMP/test_visual"
"$TMP/test_visual" "$TMP/romfs/sa3ds/sonic.sa3c" "$TMP/romfs/sa3ds/sonic.sa3t"
python3 tests/test_0_0_9_0F_runtime_source.py
python3 tests/test_white_zone_visual_source.py
