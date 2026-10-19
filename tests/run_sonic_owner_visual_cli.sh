#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3t_texture.c \
  src/sa3c/sa3c_gpu_preflight.c tools/check_sonic_visuals.c \
  -lm -o "$TMP/check_sonic_visuals"
python3 tests/make_synthetic_sonic_visuals.py "$TMP/valid"
"$TMP/check_sonic_visuals" "$TMP/valid/sonic.sa3c" "$TMP/valid/sonic.sa3t" > "$TMP/valid.log"
grep -q 'PASS: textures=1.*meshes=1 triangles=1' "$TMP/valid.log"
for scenario in bad-uv bad-metadata bad-index; do
  python3 tests/make_synthetic_sonic_visuals.py --"$scenario" "$TMP/$scenario"
  if "$TMP/check_sonic_visuals" "$TMP/$scenario/sonic.sa3c" \
      "$TMP/$scenario/sonic.sa3t" > "$TMP/$scenario.log" 2>&1; then
    echo "FAIL: accepted $scenario fixture" >&2; exit 1
  fi
done
grep -q 'NONFINITE_VERTEX.*component=6' "$TMP/bad-uv.log"
grep -q 'METADATA_MISMATCH' "$TMP/bad-metadata.log"
grep -q 'SA3C format/references: REFERENCE' "$TMP/bad-index.log"
# A valid SA3C and SA3T are derived from DIFFERENT original files.
# This mismatch is expected and must NOT be treated as an error.
python3 tests/make_synthetic_sonic_visuals.py --mismatch "$TMP/mismatch"
"$TMP/check_sonic_visuals" "$TMP/mismatch/sonic.sa3c" \
    "$TMP/mismatch/sonic.sa3t" > "$TMP/mismatch.log"
grep -q 'PASS: textures=1' "$TMP/mismatch.log"
if "$TMP/check_sonic_visuals" "$TMP/valid/sonic.sa3c" "$TMP/absent.sa3t" \
    > "$TMP/absent.log" 2>&1; then
  echo 'FAIL: accepted missing SA3T' >&2; exit 1
fi
printf 'Owner SA3C/SA3T visual+geometry CLI: PASS (valid, distinct source hashes and 4 negative fixtures)\n'
