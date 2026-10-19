# Sonic Adventure DX 2004 to Nintendo 3DS port plan

- Install: `/Users/emeraldhunter/Library/Application Support/Steam/steamapps/common/Sonic Adventure DX` (Steam conversion of the 2004 PC port).
- Source executable: exact owner binary `sonic.exe`, SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
- Private reverse-engineering reference: owner Ghidra C export `/Users/emeraldhunter/sonic.exe.c`, SHA-256 `21be26ca2a6261d337211d9a65f3d9f6c11be6b3443a809255f964f59aa63d18`.
- Engine/code: native 32-bit Windows/Ninja-era SADX code and data; no anti-cheat; offline/single-player target.
- Chosen route: clean native reimplementation for libctru/Citro3D that reads packages generated locally from the owner's installed PC data. Do not redistribute the executable, decompile, or Sega assets.
- Existing base: Pass10A CPU-upload/Pose Lab on the 0.0.9.1E-WIP9 movement and White Zone stack, plus Pass11A hierarchy/Ghidra audit.
- Current vertical slice: render and pose owner-converted PC2004 Sonic in the 3DS White Zone, with 60 Hz movement and independent source-motion verification.
- Build: `DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM sh tools/build_3ds.sh`.
- Private owner-data test: set `CHRMODELS_PATH` to original `CHRMODELS_orig.dll`, set `SONIC_PVM_PATH`, then run `sh tests/run_pass11b_source_pose.sh`.
- Hardware test: copy `dist/SA3DS_Pass11B_Audited.3dsx` and the locally generated `sonic.sa3c`/`sonic.sa3t` pair to the existing SD paths. Old 3DS hardware remains the rendering/performance oracle.
- Next dependency milestone: owner-hardware screenshots/log for Pass11B-audited build, then Emerald Coast collision/visual streaming integration.
