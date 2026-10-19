# SA3DS 0.0.9.0G — White Zone Validation Runtime

This directory is the Old-3DS White Zone measurement runtime. It contains no Sega assets.

Generate the owner-only RomFS payloads from the supported PC2004 files and build:

```sh
./build_0_0_9_0G.sh "/path/to/CHRMODELS.DLL" "/path/to/SONIC.PVM"
```

If the owner-only payloads are already present under `runtime_0_0_9_0G/romfs/sa3ds/`, run:

```sh
make -C runtime_0_0_9_0G
```

Expected target name: `runtime_0_0_9_0G/sa3ds_0_0_9_0G_white_zone_validation.3dsx`.

## White Zone controls

- Circle Pad: move / steer
- A: jump
- B or X: normal contextual action
- L / R: manual camera yaw
- Y: recenter camera
- SELECT: toggle developer overlay
- SELECT + L / R: previous / next selected test zone
- SELECT + Y: reset to selected zone
- SELECT + X: start / stop CSV trace
- SELECT + D-Pad Left: switch FOLLOW / REFERENCE camera
- START: exit

REFERENCE deliberately does not auto-follow Sonic, which makes camera-relative parity recordings reproducible. Animation observation/selection is presentation-only and guarded against physics-state mutation at runtime.

The HUD deliberately displays `PHYSICS_1TO1:NO`; 0G is a measurement gate, not a global parity claim.

## WIP7 Run11 — owner data on SD, code-only `.3dsx` build option

This optional path allows the **same native White Zone renderer** to be built
without bundling Sega assets in the `.3dsx`. It is intended for Old 3DS XL
visual testing when the owner-generated packages can be copied to SD.

On a Mac with devkitARM/libctru/Citro3D and the `picasso` shader tool installed:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM="$DEVKITPRO/devkitARM"
./build_0_0_9_0G.sh --sd
# equivalent: make -C runtime_0_0_9_0G ASSET_SOURCE=sd
```

If the project path contains spaces, use the repository wrapper instead:

```sh
DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM sh tools/build_3ds.sh
```

It stages only source plus the dedicated asset-free `romfs_sd` tree under a
private no-space temporary path and copies the resulting artifacts to `dist/`.

Copy both locally generated files (from your own PC2004 game files) to:

```
sdmc:/3ds/SA3DS/assets/sonic.sa3c
sdmc:/3ds/SA3DS/assets/sonic.sa3t
```

Do not distribute those files. The runtime first checks the original RomFS
pair, then the SD pair; it **never combines one archive from each location**.
When building with `--sd`, the packaged RomFS contains only the code-only
README and SD is used. The bottom-screen HUD reports `Assets:SD` or
`Assets:ROMFS`. The 16 MiB per-file read cap is a defensive Old 3DS limit,
not a claim about the maximum size of retail source assets. If exceeded,
report the actual package sizes before adjusting memory budgets. If a complete
RomFS pair is present but invalid, validation fails rather than silently
switching to an unrelated SD pair.

This is a host-tested **source-side build option**, not proof of a compiled
`.3dsx` or an Old 3DS boot. First hardware pass: confirm loader source on the
bottom screen, inspect Sonic's textures/UVs/alpha and ACTION transitions,
then measure frame time and available linear memory. `0.0.9.1E` gate stays
NOT PASSED until actual hardware and owner data are validated.

## WIP9 final-flight source update (Run 12)

White Zone physics now uses an integer-phase fixed **60Hz simulation clock**
independent of Citro3D render submissions. Rendering can consume zero or
multiple physics ticks; button edges latch until the next simulation tick.
Catch-up is capped at four ticks per rendered frame, with dropped ticks
reported on the developer HUD. This is a bounded scheduler, **not** proof of
60 FPS Old 3DS performance or original PC2004 pacing. SELECT+A toggles
simulation pause. The camera follow update and trace write occur per sim tick.

**SD code-only packaging safety:** `ASSET_SOURCE=sd` now selects `romfs_sd/`
(README only), NOT `romfs/`. This prevents a prior local owner-data conversion
from accidentally embedding proprietary packages in an SD-mode executable.
See `RUN12_OLD3DS_TEST.md` at the repository root for exact testing steps.
