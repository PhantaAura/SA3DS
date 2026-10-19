# SA3DS mod log

## 2026-10-19 — PC2004 movement-driven animation-rate pass

- Recovered the property 9/10/11 inputs from the exact owner PC2004 executable
  using its Ghidra 12.1.4 export and raw x86 instructions. The live REA/Ghidra
  bridge was unavailable; Hopper was not used.
- Confirmed property 9 uses signed local forward speed times row speed;
  properties 10/11 use stateful `CharObj2.SpindashSpeed`. Ground locomotion
  sets that scalar to `forward + 20 * ground residual`; the jump transition
  sets it to `5.0`. Property 10 multiplies it by row speed and property 11 uses
  it directly.
- Integrated the recovered formulas and per-tick rate accumulation into the
  existing runtime. The animation reads movement state but does not feed back
  into movement; unrecovered non-flat residual paths remain explicitly
  provisional.
- Added deterministic equation, signed-speed, changing-input, persistent jump
  scalar, fallback, and flat-path residual tests. The cumulative host suite,
  exact-executable AnimData reconstruction, and 14,049-scalar Pass11B pose
  oracle pass.
- The staged devkitARM/Citro3D build passes. The code-only 239,884-byte `.3dsx`
  has SHA-256
  `83a29554b538a6a44fc0417ed1fee2929462b1eea11b560f0f2fb249227ee71a`.
- Azahar and physical Old 3DS testing were not rerun for this checkpoint. The
  CPU-swizzled texture upload path is unchanged.

## 2026-10-19 — PC2004 Sonic AnimData fixed-rate pass

- Verified exact PC2004 executable identity and used REA with Ghidra 12.1.4;
  Hopper was not used.
- Expanded the executable reconstruction tool from ACTION remap-only output to
  all 146 complete 16-byte `SonicAnimData` row definitions.
- Confirmed `PlayAnimation` (`0x0044A800`) uses row `AnimationSpeed` directly
  for fixed properties and uses external runtime multipliers for properties
  9/10/11.
- Integrated exact instance/property/next/transition/speed metadata for the 17
  currently bound Sonic animations and exact fixed-property frame advance.
- Kept dynamic animation rate and retail transition blending explicitly
  provisional; no multiplier or blend behavior was invented.
- Added deterministic metadata/timeline regression tests and exact-executable
  comparison to the Pass11B owner-data oracle.
- Full owner-data-free host suite and Pass11B owner oracle passed; the latter
  compared 14,049 scalars with maximum error `4.91638197e-09`.
- Built the SD-mode Old 3DS application with devkitARM/Citro3D. The resulting
  239,556-byte `.3dsx` has SHA-256
  `5a504f5ecee14b0a0cdafe5364d741cc6268c9fca707e69e90ea21dd7b9dd669`.
- Azahar 2126.0 (Old 3DS mode) loaded local owner-only assets, completed all 16
  CPU-swizzled texture uploads, logged the first submitted frame, and displayed
  Sonic in White Zone at 60 FPS. No physical-hardware run was performed.

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
