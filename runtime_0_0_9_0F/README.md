# SA3DS 0.0.9.0E — Real Sonic 3DS Runtime Gate

This directory is the first **Citro3D/PICA200** real-Sonic hardware gate.

It does not contain Sonic assets. Generate local RomFS packages from your owned
PC2004 files first:

```sh
cd <checkpoint-root>
python3 tools/prepare_3ds_owner_assets.py \
  "/path/to/system/CHRMODELS.DLL" \
  "/path/to/system/SONIC.PVM"
```

Then build with devkitARM/devkitPro:

```sh
make -C runtime_0_0_9_0E
```

Or do both:

```sh
./build_0_0_9_0E.sh "/path/to/CHRMODELS.DLL" "/path/to/SONIC.PVM"
```

Expected output:

`runtime_0_0_9_0E/sa3ds_0_0_9_0E_real_sonic.3dsx`

## Hardware controls

- L/R: previous/next test animation
- A: pause
- B: reset frame
- X: cycle culling (none/back/front) for hardware diagnosis
- Y: toggle debug V inversion
- D-pad left/right: rotate Sonic
- D-pad up/down: zoom
- START: exit

The eight test animation semantics use the recovered 2004 runtime remap, not an
assumption that dense animation IDs equal CHRMODELS ACTION slots.

## Intentional 0E implementation choice

0E CPU-expands each mesh's indexed triangles into a small dynamic linear VBO and
lets PICA200 perform projection, depth testing, alpha testing, and texture
sampling. Normal Sonic is only 1,993 triangles, so this favors correctness and
observability over premature optimization. After hardware validation, 0F/0G can
move to persistent indexed VBOs and tighter draw sorting if profiling warrants it.
