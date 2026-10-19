# SA3DS 0.0.9.1E-WIP9 TextureSyncFix — hardware follow-up

## Actual Old 3DS XL evidence (2026-10-16)

BootTrace1 B: blue top, `GPU-ONLY: frame submitted` bottom (photo).
BootTrace1 X: `ASSET PREFLIGHT: PASS` (photo).
BootTrace1 A: blank top/bottom (photo); last **durable** SD log checkpoint is:

    014 ENTER Uploading Sonic GPU textures

The log does not tell which instruction stalled. Source inspection exposed a
concrete deadlock risk: `runtime_0_0_9_0G/source/sonic_gpu.c` called
`gspWaitForPPF()` **after** `C3D_SyncDisplayTransfer()` outside a frame.
Citro3D's sync function already waits for PPF in this situation. An extra wait
can wait for a second event that never occurs. This is the **leading suspected
cause**, not a hardware-confirmed fix until the next owner's 3DS run.

Reference: devkitPro/citro3d `source/renderqueue.c`, implementation of
`C3D_SyncDisplayTransfer`, see:
https://github.com/devkitPro/citro3d/blob/master/source/renderqueue.c

## Changed source (only current runtime and regression contract)

- `runtime_0_0_9_0G/source/sonic_gpu.c`: remove extra `gspWaitForPPF`, keep
  3DS tiled RGBA upload, add optional phase reports for each texture and the
  following vertex budget/linear buffer allocations.
- `runtime_0_0_9_0G/include/sonic_gpu.h`: declare optional traced init; keep
  old `sa3ds_sonic_gpu_init` available for compatibility.
- `runtime_0_0_9_0G/source/main.c`: invoke traced init, emit `TEX` lines into
  existing `/3ds/SA3DS/wip9_boottrace.log`, change display title to
  TextureSyncFix. B/X/A diagnostic choices remain present.
- `runtime_0_0_9_0G/Makefile`: display title only.
- `tests/test_0g_visual_reintegration.py`: explicitly permit exactly the
  uploader and initializer changes, while checking retained WIP1–6 guards and
  exact original draw/cleanup paths.
- `tests/test_texture_sync_fix.py` and test runner: source guard against
  recurrence of the unsynchronized second wait.

The 60 Hz physics, retail parity work, ACTION mapping, shader, camera and
White Zone geometry were not edited. The archive contains **code only**, no
Sega model files, textures, sound, retail binaries, or .3dsx.

## Owner hardware instructions

Use the companion `SA3DS_WIP9_TextureSyncFix_MacBuild.sh` to build on Mac.
Keep the original converted Sonic pair at:

    sdmc:/3ds/SA3DS/assets/sonic.sa3c
    sdmc:/3ds/SA3DS/assets/sonic.sa3t

Install `.3dsx` as `sd:/3ds/SA3DS_WIP9_TextureSyncFix.3dsx`.
Launch and press **A** directly. If Sonic appears, report rendering, movement,
frame stability, model/texture correctness and whether START exits. If it stalls
or black screens, return the *new* `wip9_boottrace.log`. The new lines now tell
us which texture and which stage: C3D_TexInit, linearAlloc, transfer, or
post-texture vertex buffer setup. B and X remain available to re-test basics.

The old log is append-only and is not overwritten. Distinguish the new run by
`000 BEGIN WIP9 TextureSyncFix app launched`.

## Validation limit

Source host checks: PASS on authoring machine. Native devkitARM compilation and
Old 3DS XL runtime: **not yet run**; owner testing is required.
