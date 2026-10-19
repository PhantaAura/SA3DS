# Sonic Adventure 3DS — Pass11A: Original PC2004 Sonic hierarchy audit

**Status:** Host-only audit checkpoint; **not** a new `.3dsx`, **not** the completed Pass11 model/animation fix.

This code-only archive verifies that our existing Sonic pose sampling/render-submit implementation reproduces the existing Pass10A White Zone metrics *for the same imported owner-supplied assets*. It also checks selected control flow in the user's Ghidra `sonic.exe.c` export. It cannot prove retail SADX visual parity.

## Tested finding (2026-10-17)

Using original user-provided `CHRMODELS.DLL` + `SONIC.PVM` privately, the existing owner importer generated `sonic.sa3c`. Host pose traversal and geometry transform checks matched previously captured hardware Pose10 metrics **exactly to six decimal places** for action 1, frame 0, bind=0 and bind=1, yaw=0:

| Metric | Stand ACTION | Stand BIND |
|---|---:|---:|
| Action/root | 1 / 72 | 1 / 72 |
| Draw vertices | 5,979 | 5,979 |
| Triangles | 1,993 | 1,993 |
| min XYZ | (-4.241374, -5.266784, -2.155196) | (-2.595378, -5.335658, -2.324799) |
| max XYZ | (4.472046, 4.589382, 2.157908) | (4.472046, 4.589382, 2.324668) |

Independent source research identifies CHRMODELS motion/action ID 1 as **Stand**; SADX gameplay animation ID 0 points to it, with documented speed 0.50 and looping behavior. Reference: https://sadocs.unreliable.network/wiki/Sonic_Adventure/Character_Animation_Data . This is a runtime animation mapping, not a reason to change Pose10 frozen frame-0 visual snapshots.

All original 223 Sonic nodes have source flags without bit 0x20 (the alternate ZYX flag); the imported 2004 model isn't using the alternate mode. Thus changing the rotation order globally is unsupported by current evidence.

Ghidra `FUN_004050C0` confirms child at object +0x2c, sibling +0x30 and flags 0x40/0x20; Ghidra `FUN_00405260` chooses rotation handler by object flags. The host checker asserts these exact control-flow markers, not a semantic proof of all animation math.

## How to run on a Mac

Requires: command-line C compiler (`cc`), Python 3, locally generated owner asset `sonic.sa3c`.

1. Extract the ZIP.
2. Open Terminal in the extracted `Pass11A_2004PC_SonicAudit` folder.
3. Run:

```bash
bash Pass11A_MacAudit.sh /absolute/path/to/sonic.sa3c /absolute/path/to/pass10_pose_metrics.csv /absolute/path/to/sonic.exe.c
```

The second and third arguments are optional. `Pass11A_results/` will contain action-vs-bind pose CSV, per-node transforms and synthetic test programs. The owner-generated `.sa3c`, Ghidra export and Pose10 metrics are **not** bundled and never edited.

This host-only tool compiles using ordinary C11. It does **not** require devkitARM, a 3DS, or a Ghidra extension. It does not change any game executable, existing `.3dsx`, or SD directory.

## Next: Pass11B

1. Compare **source PC ACTION slot 1's animation frame-data sampling** independently against the importer rather than evaluating the same SA3DS code twice.
2. Inspect mesh hide/culling/material behavior and Pose Lab camera projection if source animation checks pass.
3. Integrate *verified* corrections into latest working **Pass10A CPU-upload** source when supplied.
4. Keep CPU-upload texture path and boot logs intact for Old 3DS hardware testing.

**Still required for 3DS integration:** the exact working Pass10A Pose Lab CodeOnly or All-in-One source ZIP. Screenshots, logs and Ghidra `.c` alone are insufficient to patch that build safely.

## Licensing & provenance

The archive contains independent code-only tools and no Sega game assets, original executables, textures, animations, source Ghidra dump, binaries, or rendered previews. The six-decimal numerical metrics above are metadata only. The `src/sa3c` files are a read-only copy of the project's own WIP9/CPUFix host logic for independent auditing. No claim of complete original-PC physics, visuals, or animation parity.
