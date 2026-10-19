# Latest: SA3DS Pass11B — audited PC2004 Sonic pose runtime

This tree now continues the supplied Pass10A CPU-upload/Pose Lab baseline. Pass11A reproduced the owner's hardware geometry metrics and checked hierarchy/action control flow in the private `sonic.exe.c` export. Pass11B independently samples ACTION slot 1 from the exact original `CHRMODELS_orig.dll` and compares it with the C runtime: 223 nodes, 7 frames, 14,049 scalar comparisons, maximum error approximately `4.9e-9`.

Start with `MODDING_PLAN.md`, `docs/PASS11B_SOURCE_POSE_ORACLE.md`, and `docs/HANDOFF.md`. Build the code-only SD-mode app with `DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM sh tools/build_3ds.sh`. The resulting hardware-test package is under `dist/`; a new Old 3DS run is still required.

The older CAM and Emerald Coast reconstruction notes below remain future integration inputs; they are not yet a complete playable stage.

# Sonic Adventure 3DS — 0.0.9.1A

**Current milestone: Emerald Coast LandTable + Collision Decode**

SA3DS remains a native 3DS port/reimplementation project: **1 P, no Rs — PORT.
No remake. No remaster. Just a port with QoL.**  The 2004 PC/SADX lineage is the
engineering and behavior source; Dreamcast SA1 remains the presentation/fidelity
reference; Old 3DS / Old 3DS XL remains the baseline hardware target.

0.0.9.1A advances directly from the validated White Zone codebase into the first
real Emerald Coast source pass.  The exact 2004 executable now yields all three
Emerald Coast LandTables, 848 collision instances, 426 unique Basic models and
46,903 triangles.  The new owner-side converter emits pointer-free collision
data into `.sa3dspak` without placing Sega payloads in this checkpoint.

Start with `0.0.9.1A_README.md` and
`docs/EMERALD_COAST_LANDTABLE_RECON_0.0.9.1A.md`.

## Test

```sh
./tests/run_0_0_9_1_tests.sh /path/to/sonic.exe /path/to/system-or-system.zip
```

## Owner-side collision package

```sh
python3 tools/build_pc2004_emerald_coast_collision.py \
  /path/to/sonic.exe /tmp/emerald_coast.sa3dspak
```

Do not redistribute the generated stage package; it contains data reconstructed
from the owner's Sonic Adventure files.

## 0.0.9.1B — Emerald Coast Visual / Material / PVM Decode

The current checkpoint extends the exact Emerald Coast LandTable decode into
pointer-free renderable geometry, exact Basic material records, signed16/255
UVs, and strict BEACH01/02/03 PVM texture bindings.  A generic exact-source PVM
parser also validates OBJ_BEACH, BEACH_SEA, and BG_BEACH.  Owner-side generated
visual packages are excluded from the source archive.

See `0.0.9.1B_README.md` and `docs/EMERALD_COAST_VISUAL_FORMAT_0.0.9.1B.md`.

## 0.0.9.1C — owner SET/object placement decode

See `0.0.9.1C_README.md`, `SET_INVENTORY_0.0.9.1C.json`,
`tools/importers/pc2004_set.py` and
`tools/build_pc2004_set_packages.py`. The standalone
`--with-set` option on the Emerald Coast visual builder embeds placement banks
while leaving legacy 1B output unchanged. This adds no object gameplay logic.
