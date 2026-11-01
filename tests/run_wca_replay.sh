#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-wca-plan.XXXXXX")"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/game tests/test_wca_replay.c src/game/wca_replay.c -lm -o "$tmp/test"
"$tmp/test"
python3 tests/test_wca_replay_integration.py
python3 tests/test_wca_replay_verifier.py
