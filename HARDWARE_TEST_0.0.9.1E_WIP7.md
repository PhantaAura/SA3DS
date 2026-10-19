# SA3DS WIP7 — Mac build and Old 3DS XL validation (NOT YET EXECUTED)

## Source and asset safety

- This checkpoint is code-only. It intentionally does not contain any Sega executable, DLL, PVM, extracted Sonic model, audio, texture, or cooked SA3C/SA3T.
- Locally generate `sonic.sa3c` from your **own** 2004 `CHRMODELS_orig.dll` and `sonic.sa3t` from your **own** `SONIC.PVM` using the project's converters. Their source SHA-256 values are **not** expected to match.
- Place only the generated owner-side packages in `runtime_0_0_9_0G/romfs/sa3ds/` on your local Mac for compilation. Never redistribute those assets or compiled payload-bearing builds.

## Preflight on Mac (owner data stays local)

1. Run `bash tests/run_sa3ds_current_host.sh` from the source root. Expect a PASS, including `0G native visual reintegration`.
2. Build the owner-side CLIs using `cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c -Isrc/game ...` or use the existing scripts `bash tests/run_sonic_owner_visual_cli.sh` and `bash tests/run_sonic_action_preflight.sh` to verify the test fixtures. For real packages, compile `tools/check_sonic_visuals.c` and `tools/check_sonic_actions.c` per their usage headers, then run them on your owner-generated packages. Record actual output.
3. With devkitPro's devkitARM, libctru, Citro3D and shader tooling installed, set `DEVKITPRO` and `DEVKITARM` appropriately, then run `make -C runtime_0_0_9_0G clean` and `make -C runtime_0_0_9_0G -j2`. Keep full compiler/linker logs. Do not claim success until the `.3dsx` actually exists.
4. If compilation fails, preserve the first error and repair source/toolchain compatibility before trying hardware. Do not fall back to the old 0G binary while claiming WIP7 features.

## Old 3DS XL acceptance procedure (only after a successful new build)

1. Back up SD contents, install the new `.3dsx` and its associated owner-side data in the location expected by the build. Charge the device before testing.
2. Launch and photograph top + bottom screens. The bottom-screen title should identify `0.0.9.1E-WIP7`, and the developer HUD should show texture counts, alpha counts, GPU arena used/capacity, animation ID, ACTION slot and frame.
3. In the White Zone, check Sonic from front, side, and back. Inspect blue material, muzzle, eyes, gloves, shoes, UV seams, alpha, depth and culling. Do not judge retail parity from appearance alone; compare to captured 2004 PC reference under matched pose and camera.
4. Reset with SELECT+Y while paused and while moving; ensure the displayed pose resets immediately. Cycle zones with SELECT+L/R; verify HUD zone selection and camera modes. Record whether traces can start/stop with SELECT+X.
5. Check that GPU arena usage never exceeds capacity, that geometry remains stable across multiple draw calls, and that the game does not crash or show corrupted meshes after repeated resets. Capture console error text if any.
6. Record FPS/frame pacing, free linear memory, texture upload startup time, and whether the device survives sleep/resume. These are **measurements pending**, not current claims.

## Gate

Do **not** mark 0.0.9.1E complete until owner package preflight, actual devkitARM compilation, visible original Sonic geometry/material/animation inspection, and Old 3DS XL stability checks have been performed. Physics/retail animation transition parity requires separate measured traces.
