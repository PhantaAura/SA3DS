#!/bin/sh
set -eu

# The devkitPro make templates tokenize absolute paths containing spaces.
# Stage only code into a private no-space path, build there, then copy the
# resulting code-only artifacts back into dist/.
root="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
runtime="${1:-runtime_0_0_9_0G}"
case "$runtime" in
  runtime_0_0_9_0G) ;;
  *) echo "unsupported runtime directory: $runtime" >&2; exit 2 ;;
esac
: "${DEVKITPRO:=/opt/devkitpro}"
: "${DEVKITARM:=$DEVKITPRO/devkitARM}"
if [ ! -f "$DEVKITARM/3ds_rules" ]; then
  echo "missing devkitARM 3ds_rules under $DEVKITARM" >&2
  exit 2
fi

stage="$(mktemp -d "${TMPDIR:-/tmp}/sa3ds-build.XXXXXX")"
trap 'rm -rf "$stage"' EXIT HUP INT TERM
mkdir -p "$stage/project/$runtime" "$root/dist"
cp -R "$root/src" "$stage/project/src"
# Copy only build inputs used by ASSET_SOURCE=sd. In particular, never stage
# the normal RomFS tree, stale build products, or locally converted assets.
cp "$root/$runtime/Makefile" "$stage/project/$runtime/Makefile"
cp -R "$root/$runtime/include" "$stage/project/$runtime/include"
cp -R "$root/$runtime/source" "$stage/project/$runtime/source"
cp -R "$root/$runtime/romfs_sd" "$stage/project/$runtime/romfs_sd"
make -C "$stage/project/$runtime" ASSET_SOURCE=sd \
  DEVKITPRO="$DEVKITPRO" DEVKITARM="$DEVKITARM"

target="sa3ds_0_0_9_0G_white_zone_validation"
for ext in 3dsx elf smdh map; do
  source_file="$stage/project/$runtime/$target.$ext"
  if [ "$ext" = map ] && [ ! -f "$source_file" ]; then
    source_file="$stage/project/$runtime/build/$target.map"
  fi
  if [ -f "$source_file" ]; then
    cp "$source_file" "$root/dist/SA3DS_Pass11B_Audited.$ext"
  fi
done
echo "Built code-only artifacts in $root/dist"
