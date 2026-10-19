# SA3DS mod log

## 2026-10-17 — Pass11B source-motion audit

- Imported the supplied Pass10A code-only source as the working base and preserved Pass11A under `audits/pass11a/`.
- Verified the exact owner sources: original `CHRMODELS_orig.dll` SHA-256 `76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5`; `SONIC.PVM` SHA-256 `f8e30dfbebeb901a0ac210a61929c45fbcefcc1a2758cba400f959989b93ecbe`.
- Cumulative owner-data-free host gate passed.
- Owner visual preflight passed.
- Pass11A matched owner Pass10 hardware metrics with maximum absolute bound error `1e-6`; all selected `sonic.exe.c` hierarchy/action markers passed.
- Added Pass11B independent raw-DLL ACTION slot 1 sampler. It does not import the project decoder/writer and compares the source directly with the C SA3C runtime.
- Pass11B passed for 223 nodes over 7 frames and 14,049 scalar comparisons; maximum absolute error was `4.91638197e-09`.
- Found build-system path defect: devkitPro's template breaks when the source path contains spaces. Added `tools/build_3ds.sh`, which stages code in a private no-space directory and emits code-only artifacts into `dist/`.
- The cumulative owner-data-free host gate passed after the changes. The staged devkitARM/Citro3D build passed and produced the Pass11B-audited `.3dsx`, `.elf`, `.smdh`, and `.map` artifacts.
- No Sega executable, decompile, converted model, texture, or game data is stored in this project or embedded in the SD-mode build.

## Next

1. Run Pass11B oracle and cumulative host tests after every pose/runtime change.
2. Build the SD-mode `.3dsx` and test on the owner's Old 3DS.
3. Keep the CPU texture upload and boot trace until hardware proves a replacement.
4. Advance Emerald Coast only after the current Sonic render/pose gate remains stable.
