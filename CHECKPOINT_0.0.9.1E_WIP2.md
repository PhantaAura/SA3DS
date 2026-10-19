# SA3DS 0.0.9.1E-WIP2 — GPU Frame Vertex Isolation (NOT E Completion)

**Run:** October 13, 2026, 00:00 EDT; overall run 5 / recovery pass 2. **Parent:** exact verified WIP1 ZIP SHA-256 `94f9c681b8b04512c0c26e472f7f7beb071dbc041a4e314267f25a47fb295eb2` (unzip integrity verified), itself descended from 0.0.9.1D.

## Engineering change
The native Sonic GPU renderer previously used a single scratch vertex span sized for the largest mesh, overwriting that span for every mesh before the GPU necessarily consumed queued draw commands. White Zone lab zone draws and distance markers also reused the same vertex memory. This is a source-level rendering hazard; actual hardware corruption is not claimed. `src/sa3c/sa3c_gpu_batch.{h,c}` now computes a conservative upper bound over every model instance referenced by a node (including repeated models), rejects invalid references and oversize allocations, and reserves a non-overlapping vertex span for each draw. The Sonic draw callback binds/flushed its own unique vertex range; the frame arena resets at the next synchronized `C3D_FrameBegin(C3D_FRAME_SYNCDRAW)` call. The lab similarly assigns separate per-zone spans and appends marker geometry after them. HUD reports arena usage and last allocator error. Original animation and physics semantics unchanged.

## Test evidence
- `sha256sum SA3DS_0.0.9.1E-WIP1_WhiteZoneVisualGuard_CodeOnly.zip`: matched WIP1 parent hash.
- `unzip -t` parent: PASS.
- `bash tests/run_white_zone_gpu_batch.sh`: PASS (compiled C adversarial arena fixtures and native source contract).
- `bash tests/run_0_0_9_0G_tests.sh`: PASS (White Zone geometry, HUD/camera, animation non-mutation stress, 12 deterministic protocols, strict comparator, runtime contract).
- `bash tests/run_0_0_9_1D_tests.sh`: PASS (synthetic CAM suite).
- `python3 tests/test_white_zone_visual_source.py`: PASS (inherited WIP1 native visual integration contract).
- `python3 tests/test_0_0_9_0F_runtime_source.py`: PASS.
- **Not run:** owner original model/texture tests, because original Sega inputs were not mounted here; full SET suite intentionally not repeated after prior TEST-001 timeout; native 3DS build unavailable (`DEVKITARM` not installed); hardware test pending.

## Parity and next gate
Source-backed partial only. GPU frame lifetime fix has host verification, not proven retail-equivalent rendering. Original Sonic visual fidelity, action transitions, culling/UV/alpha, camera, Old 3DS boot and performance are unverified. Next run must inherit this exact WIP2 artifact and proceed with animation and material GPU integration, not jump to Emerald Coast or declare E complete. Code-only archive contains no Sega-owned assets.
