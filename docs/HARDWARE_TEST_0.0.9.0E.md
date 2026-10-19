# SA3DS 0.0.9.0E Hardware Test — Old 3DS / Old 3DS XL

## 1. Build

From the extracted checkpoint root on the Mac with devkitARM/devkitPro installed:

```sh
./build_0_0_9_0E.sh \
  "/path/to/system/CHRMODELS.DLL" \
  "/path/to/system/SONIC.PVM"
```

Expected build product:

```text
runtime_0_0_9_0E/sa3ds_0_0_9_0E_real_sonic.3dsx
```

If the owner files do not match the exact supported PC2004 hashes, the preparation script must stop instead of generating a package from an unknown profile.

## 2. Put on SD card

Copy the resulting `.3dsx` into a Homebrew Launcher-visible folder, for example:

```text
/3ds/sa3ds_0_0_9_0E/sa3ds_0_0_9_0E_real_sonic.3dsx
```

The generated RomFS is embedded in the `.3dsx` by the normal devkitARM build; the raw owner-source files do not need to be copied to the 3DS.

## 3. First boot checks

The top screen should show textured real PC2004 Sonic in the white-space presentation test.

The bottom screen should show:

- checkpoint/version
- semantic animation name
- dense runtime animation ID
- CHRMODELS ACTION slot
- current animation frame
- node/draw/triangle counts
- submitted vertices
- texture count
- culling/debug V state
- CPU submission time
- free linear memory

## 4. Cycle all eight presets

Use L/R and verify:

1. Stand
2. Walk1
3. Run1
4. JumpOrSpin
5. Roll
6. Fall
7. Land
8. Brake

Expected structural counts:

- Stand/Walk1/Run1/Fall/Land/Brake: **42 draws / 1,993 triangles**
- JumpOrSpin/Roll: **41 draws / 1,001 triangles**

## 5. Visual diagnosis controls

- `X`: cycle none/back/front culling. If geometry disappears with one cull direction, record which mode is correct.
- `Y`: toggle debug V inversion. If textures are vertically flipped in the default state, record that the toggled state is correct.
- D-pad left/right: rotate Sonic and inspect all sides.
- D-pad up/down: adjust camera distance.
- `A`: pause to inspect a pose.
- `B`: reset frame.

## 6. Report back

The most useful result is:

- whether it compiled cleanly;
- whether it booted;
- a photo/screenshot of top + bottom screens;
- which culling mode looks correct;
- whether default or toggled V orientation looks correct;
- displayed CPU submit time and free linear memory;
- whether animation cycling stays smooth.

Do not call 0.0.9.0E hardware-verified until these physical-device checks pass.
