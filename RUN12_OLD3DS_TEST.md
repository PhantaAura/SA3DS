# SA3DS WIP9 — exact owner-side Mac / Old 3DS XL validation

This is a **hardware test procedure**, not a claim that it was performed.
WIP9 has NOT been compiled with devkitARM or booted on a physical 3DS.
Use legally owned PC2004 Sonic Adventure DX data locally; never upload or
redistribute the original DLL/PVM, generated SA3C/SA3T, or Sega assets.

## 1. Reproduce host source tests (Mac Terminal)

From the unpacked WIP9 source root:

```sh
shasum -a 256 ../SA3DS_0.0.9.1E-WIP9_FinalFlight_CodeOnly.zip
bash tests/run_sa3ds_current_host.sh
```

Compare the SHA to the accompanying `.sha256` file. The host suite must
complete without failure. It does NOT prove a 3DS build or GPU parity.

## 2. Generate local Sonic packages (owner data stays on Mac/SD)

```sh
python3 tools/prepare_3ds_owner_assets.py \
  "/absolute/path/to/CHRMODELS_orig.dll" \
  "/absolute/path/to/SONIC.PVM" \
  --runtime runtime_0_0_9_0G
```

The converter checks the expected 2004 PC source hashes. If they differ,
STOP and record the hashes; do not patch the verification to force a match.
This writes two local packages under `runtime_0_0_9_0G/romfs/sa3ds/`.

Compile and run the portable visual and ACTION validators before copying
anything to SD (see `tools/README_owner_visual_preflight.md` and
`tools/check_sonic_actions.c`). If either rejects original generated data,
record the exact section/slot/key and stop the hardware test.

## 3. Prepare the code-only `.3dsx` on a Mac with the 3DS toolchain

Verify `DEVKITPRO`, `DEVKITARM`, libctru, Citro3D, `picasso`, and `3dsxtool`
are installed and available. Then:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM="$DEVKITPRO/devkitARM"
./build_0_0_9_0G.sh --sd
```

`--sd` selects **romfs_sd**, an isolated README-only directory. It must NOT
embed the owner packages left in the normal `romfs/` directory. The intended
output is `runtime_0_0_9_0G/sa3ds_0_0_9_0G_white_zone_validation.3dsx`.
Verify the built RomFS contains no `sonic.sa3c`, `sonic.sa3t`, original
DLL/PVM, or other Sega-derived data. If the compiler/linker/shader step
fails, capture its FIRST error and complete build log before modifying code.

## 4. Install owner data privately on the Old 3DS XL SD card

Copy the `.3dsx` into the SD card's `/3ds/` homebrew application folder.
Copy ONLY the locally generated package pair into:

```
/3ds/SA3DS/assets/sonic.sa3c
/3ds/SA3DS/assets/sonic.sa3t
```

The code-only `.3dsx` may be shared separately; the two owner-generated
files must remain private. Do not mix SA3C/SA3T from different conversions.

## 5. Real hardware observation checklist

1. Launch through Homebrew Launcher on **Old 3DS/Old 3DS XL**. Photograph
   any crash screen or record the exact first console error.
2. Bottom screen must report `Assets:SD`. If it says `ROMFS`, inspect the
   build package; the SD build should have no owner assets in RomFS.
3. Top screen: confirm Sonic's full model appears, correct orientation and
   scale, recognizable texture mapping, UV seams, black material regions,
   transparent areas and limbs; record screenshots/photos.
4. Circle Pad movement, A jump, B/X action; verify pose selection follows
   gameplay, landing does not disappear within a tick, and animations do
   not visibly alter the movement state. Retail timing is still unverified.
5. Y camera recenter, L/R manual yaw, SELECT+L/R test-zone change,
   SELECT+Y reset. SELECT+A pauses/resumes the simulation while rendering
   remains active. Check no huge physics jump after resuming.
6. Record HUD `60Hz` ticks and `Drop` count. Over a measured 10-second
   period with stable input, expect ~600 simulation ticks if no catch-up
   steps are dropped. This is an acceptance **measurement**, not a promise
   that Old 3DS can maintain 60Hz simulation and desired render FPS.
7. Use SELECT+X to toggle trace. Inspect the private SD file
   `/3ds/SA3DS/traces/white_zone_trace.csv` for monotonically increasing
   simulation frames and exactly one trace record per completed physics tick.
8. Repeat startup/zone-reset/pause for several minutes; record linear-memory
   free display, CPU frame-submit time, crashes and visual corruption.

## 6. Pass/fail gate

Mark **0.0.9.1E PASS** only after a real owner-generated SA3C/SA3T pair
passes validators, the `.3dsx` compiles, and an Old 3DS XL boots and renders
Sonic correctly with reproducible texture/geometry/animation evidence.
Any measured mismatch is a new error-ledger entry, not a reason to skip to
Emerald Coast. Even then, global 1:1 physics and retail camera behavior
require separate PC2004 trace comparisons.
