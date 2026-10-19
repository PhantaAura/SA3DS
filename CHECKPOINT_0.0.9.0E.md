# Checkpoint 0.0.9.0E — Real Sonic 3DS Runtime Gate

Status: **PASS (source/build gate); HARDWARE VERIFICATION PENDING**

## Proven in this checkpoint

- Frozen 0.0.8.8.9 baseline remains byte-identical: **108/108** hashes PASS.
- Inherited 0D real-Sonic source/render semantics PASS.
- Real owner-source SA3C/SA3T generation PASS with exact source hash guards.
- All eight hardware-test Sonic animation presets sample and emit draw commands successfully.
- Normal root: 42 draws / 1,993 triangles.
- ACTION 14 alternate Jump/Roll root: 41 draws / 1,001 triangles.
- Citro3D runtime source contract PASS.
- Runtime consumes owner-generated RomFS `sonic.sa3c` and `sonic.sa3t`.
- CPU hierarchy/animation transform path remains presentation-only and does not write physics state.
- PICA200 path includes depth test, alpha test, material-color modulation and hardware texture sampling.
- SA3T RGBA8 upload is explicitly tiled for PICA200 rather than assuming row-major `C3D_TexUpload` input is directly sampleable.
- Bottom-screen hardware diagnostics expose animation/remap state, source draw counts, submitted vertices, CPU submission time and free linear memory.

## Not claimed until the user runs it on hardware

- successful devkitARM compilation on the user's installed toolchain;
- successful boot on Old 3DS/Old 3DS XL;
- correct texture orientation/channel order on physical PICA200 output;
- stable 60 FPS on Old 3DS;
- final CPU/GPU/linear-memory performance;
- final cull state and material parity;
- physics-driven animation selection (planned for 0.0.9.0F).

## Exact source profiles

- PC2004 `sonic.exe`: `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`
- PC2004 `CHRMODELS.DLL`: `76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5`
- `SONIC.PVM`: `f8e30dfbebeb901a0ac210a61929c45fbcefcc1a2758cba400f959989b93ecbe`
