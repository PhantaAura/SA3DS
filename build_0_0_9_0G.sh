#!/bin/sh
set -eu
cd "$(dirname "$0")"
if [ "$#" -eq 1 ] && [ "$1" = "--sd" ]; then
  exec make -C runtime_0_0_9_0G ASSET_SOURCE=sd
elif [ "$#" -eq 2 ]; then
  python3 tools/prepare_3ds_owner_assets.py "$1" "$2" --runtime runtime_0_0_9_0G
elif [ "$#" -ne 0 ]; then
  echo "usage: $0 [CHRMODELS.DLL SONIC.PVM | --sd]" >&2
  exit 2
fi
exec make -C runtime_0_0_9_0G
