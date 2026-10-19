# Pass 10A — Frozen Sonic Pose Lab / Geometry Diagnostics

## Scope and provenance

Built directly on WIP9 CPUUpload2, which the owner has photographed running real Sonic on Old 3DS XL after a successful CPU texture upload. The renderer, owner-generated SA3C/SA3T asset pipeline, physics, and standard White Zone mode are kept. This is an **independent diagnostic mode**, not a visual- or physics-parity certification. Dreamcast/DX asset selection and PC mod compatibility are later phases.

## Launch

Copy this code-only 3dsx to `SD:/3ds/SA3DS_Pass10A_PoseLab.3dsx` after building on Mac (not compiled here). Keep owner-converted `sonic.sa3c` and `sonic.sa3t` under `SD:/3ds/SA3DS/assets/`. At the new start menu, press **L** for `Pass10A Pose Lab`; **A** still launches regular White Zone, **Y** keeps the texture-only test, **X** validates assets, **B** performs the GPU-only test. The source files, not Sega assets, are packaged.

Pose Lab starts in **Stand ACTION slot 1, frame 0**, with simulation ticks disabled. It draws the original owner-extracted character through the existing CPU-swirled texture and Citro3D pipeline against a pale-blue diagnosis background (normal White Zone remains white). The camera center is set once using the initial source-frame rendered bounding box, and then stays stable across animation frames.

Controls inside Pose Lab:

| Input | Effect |
| --- | --- |
| **L / R** | Rotate view -45° / +45° (yaw) |
| **D-Pad Left / Right** | Source frame -1 / +1 (no automatic playback) |
| **X / B** | Previous / next source ACTION slot in a small verified set |
| **A** | Toggle between motion's frame and the source model's bind transforms |
| **Y** | Reset to Stand ACTION, frame 0, view 0°, standard zoom |
| **D-Pad Up / Down** | Zoom in / out |
| **START** | Exit |

The bottom screen reports selected action slot, source frame, bind state, fixed camera orientation/distance, root and transformed min/max X/Y/Z bounds. Logs record only numeric measurements and do not export model vertices:

- `SD:/3ds/SA3DS/pass10_pose_metrics.csv` — overwritten on each Pose Lab launch, updates when user changes pose/view.
- `SD:/3ds/SA3DS/pass10_boot.log` — appended startup checkpoints.

## First owner hardware protocol

1. Open **L Pose Lab**, don't touch the controls; photograph BOTH screens. Confirm `Stand Slot:1 Frame:0.0 Mode:ACTION` and Sonic's full standing silhouette.
2. Press **R twice** (90 degrees); photograph both screens. Press **R twice** more (180 degrees) and photograph.
3. Reset **Y**, press **A** to compare bind pose; photograph.
4. Press **B** (Walk1) then step 1–3 frames using **Right**; note whether the model becomes distorted only after motion keys are applied.
5. Exit, launch **A** (ordinary White Zone), ensure moving Sonic still works and the bottom physics HUD is unchanged.
6. Send the photos and `pass10_pose_metrics.csv` if possible.

**Interpretation:** A deformed Stand frame 0 and bind pose point toward import/hierarchy/presentation errors; a good stand but distorted walking pose points toward animation sampling/transitions; a changing shape depending on camera angle points toward projection, culling, or texture effects. This diagnostic alone does not prove any specific hypothesis.

## Host measurement evidence

With the user-provided PC2004 data in a private runtime, `report_sonic_pose_metrics` successfully read the generated SA3C and measured Stand slot 1 at frame 0: 1,993 triangles, 5,979 drawn triangle vertices, root node 72, extents about X -4.241..4.472, Y -5.267..4.589, Z -2.155..2.158. Frame 2 also decoded. These are SOURCE-PROCESSING measurements, not proof that the original game looks right. Neither owner's SA3C/SA3T nor exported mesh geometry is present in the distributed archive.

## Remaining gaps

- Native 3DS compilation: requires owner's devkitARM (not present on the assistant's host).
- Old 3DS hardware fidelity: untested for this new diagnostic.
- True PC2004 reference images/pose matrix comparison and retail action timing: not yet captured.
- SADX/Dreamcast toggling: planned, not implemented.
- Mod shop: planned later; the existing SD external-asset loader should remain compatible.

## Optional standalone Mac-side geometry report

From the unpacked project root, with `cc` installed and owner-generated `sonic.sa3c` already available:

```sh
cc -std=c11 -O2 -Wall -Wextra -Isrc/sa3c -Isrc/game \
 src/sa3c/sa3c_character.c src/sa3c/sa3c_pose.c src/sa3c/sa3c_render.c \
 src/game/sonic_pose_metrics.c tools/report_sonic_pose_metrics.c -lm -o report_sonic_pose_metrics
./report_sonic_pose_metrics /path/to/sonic.sa3c > pose_report.csv
```

Do not share the underlying SA3C asset publicly; only the numerical summary if you choose.
