#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
${CC:-cc} -std=c11 -Wall -Wextra -Werror -O1 -g -fsanitize=undefined \
  -Iruntime_0_0_9_0G/include \
  runtime_0_0_9_0G/source/owner_assets.c \
  tests/test_owner_pair_loader.c -o "$tmp/test_owner_pair_loader"
"$tmp/test_owner_pair_loader"
python3 tests/test_0g_external_asset_integration.py
sh tests/test_owner_asset_build_mode.sh
