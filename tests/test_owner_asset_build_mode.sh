#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
# This is a Makefile mode test using a dummy rules include; not devkitARM.
: > "$tmp/3ds_rules"
make -s -C runtime_0_0_9_0G check-assets DEVKITARM="$tmp" ASSET_SOURCE=sd > "$tmp/sd.log"
grep -q 'External SD owner assets selected' "$tmp/sd.log"
# Resolve Makefile variables using the real GNU Make parser and stub rules.
# SD mode must NOT point to romfs/, which may contain local Sega assets.
make -s -pn -C runtime_0_0_9_0G check-assets DEVKITARM="$tmp" ASSET_SOURCE=sd > "$tmp/sd.vars"
grep -Eq '^ROMFS[[:space:]]*:?=[[:space:]]*romfs_sd$' "$tmp/sd.vars"
make -s -pn -C runtime_0_0_9_0G check-assets DEVKITARM="$tmp" ASSET_SOURCE=sd > "$tmp/sd.vars2"
grep -Eq '^ROMFS[[:space:]]*:?=[[:space:]]*romfs_sd$' "$tmp/sd.vars2"
# The dedicated packaging tree must be code-only.
find runtime_0_0_9_0G/romfs_sd -type f | grep -q 'README.txt'
if find runtime_0_0_9_0G/romfs_sd -type f | grep -Eq '\.(sa3c|sa3t|pvm|dll)$'; then
    echo 'ERROR: SD packaging tree contains owner assets' >&2; exit 1
fi
if make -s -C runtime_0_0_9_0G check-assets DEVKITARM="$tmp" > "$tmp/romfs.log" 2>&1; then
    echo 'ERROR: default RomFS build accepted missing owner assets' >&2
    exit 1
fi
bash -n build_0_0_9_0G.sh
printf '%s\n' 'owner build modes: PASS (SD isolated code-only RomFS, default RomFS guarded; no native compile)'
