# SA3DS 0.0.9.1E-WIP1 — White Zone Visual Guard (NOT E completion)

**Parent:** SA3DS 0.0.9.1D CAM Decode, SHA-256 `5409a6aa2d5776975389a4ec22ebadfb0acb1c58631ec1904eb0dacff0b67517`.

**Run:** October 12, 2026, 23:00 EDT (recovery overall run 4). Prior three scheduled runs encountered ENV-001 and produced no integrated code. This run recovered working container access, independently verified the parent ZIP hash and integrity, extracted the source and compiled/executed real inherited tests.

**New code (source-only):** A host-testable SA3C/SA3T/PICA200 visual preflight validates owner-generated texture count, source metadata linkage, supported power-of-two dimensions, material texture references, RGBA byte counts and alpha distribution before GPU uploads. The native White Zone renderer invokes it before allocating textures; runtime prints actionable error and developer overlay statistics. Material diffuse AARRGGBB is converted to RRGGBBAA without forcing legitimate black to white or discarding source alpha. Host software raster smoke applies the same black/alpha behavior. New C adversarial tests and Python integration contract check guard against regression.

**Actual owner evidence:** Exact original PC2004 CHRMODELS.DLL and SONIC.PVM from the user-provided system archives passed their known SHA-256 profiles. Sonic has 16 RGBA textures (232,192 bytes), 13 referenced texture slots; 125 model material instances in the audited source sample, all source material alpha bytes FF and none with black RGB. All source texture dimensions fit the 3DS PICA200 upload constraints. Thus no real owner texture mismatch has been found; the new guard prevents silent future mismatches and improves diagnostics. Material alpha/black changes have no expected visible effect on this exact Sonic source, but remove unsupported assumptions for future source profiles.

**Tests executed and passed:**
- `bash tests/run_0_0_9_0G_tests.sh`: White Zone lab geometry, HUD, 1024-state observer non-mutation, 12 deterministic protocols, strict comparator and runtime source contract.
- `bash tests/run_0_0_9_0E_tests.sh` with exact owner CHRMODELS/PVM: inherited character decode, SA3C pose/animation, texture conversion and host render smoke.
- `bash tests/run_0_0_9_1D_tests.sh`: CAM synthetic/NaN/CRC/SHA/duplicate tests.
- `sh tests/run_white_zone_visual_preflight.sh` with owner inputs: C preflight, invalid dimension/metadata/material reference/count/alpha fixtures, native source integration contract.
- `SA3DS_SKIP_INHERITED=1 sh tests/run_0_0_9_0D_tests.sh` with owner inputs: 3 host textured white-space renders pass, 1,993 triangles for normal Sonic.
- Full `run_0_0_9_1C_tests.sh` was started with owner EXE/system.zip. Inherited 1B stage visual tests passed, but the run exceeded the execution timeout in the SET owner decode; full 1C suite **NOT COMPLETED** and not counted as passing.

**Unverified / pending:** This run has no devkitARM/libctru or Old 3DS runtime, so native C source changes are not yet 3DS-compiled or hardware-tested. Original Sonic visual parity remains source-backed partial, not verified 1:1. Retail animation switching, material flag semantics, camera, physics and Old 3DS performance remain pending. No Emerald Coast playable runtime was built.

**Next:** Finish 0.0.9.1E original Sonic GPU visual integration and real animation-state validation. Prioritize compiling on an actual 3DS SDK environment, checking texture appearance, alpha/culling/UV orientation and action transitions on Old 3DS; isolate full 1C timeout only when it becomes relevant. Do not skip to 0.0.9.1F just because the hourly slot advanced.

**Copyright:** Code-only. The owner-generated `.sa3c`, `.sa3t`, CHRMODELS.DLL, SONIC.PVM and all Sega content are excluded from this archive.
