# SA3DS 0.0.9.1E-WIP3 — Animation Presentation Timeline Stability

**Run:** October 13, 2026, 01:00 EDT, overall flight run 6 / recovery pass 3. **Parent:** WIP2 ZIP SHA-256 `7de3fe4dbfa146171cd9b8e6e688dd21312083de0a6d966df0790299c3c1c584`, ZIP integrity PASS. Canonical 0.0.9.1D ancestry unchanged.

## Source changes
- `src/game/sonic_animation_sync.{h,c}`: observer memoization for repeated completed simulation frame; backward frame invalidates prior timeline.
- `src/game/sonic_animation_runtime.{h,c}`: stable selection cache, preserving one-tick landing across repeated draw/HUD queries and resetting on rewind.
- `runtime_0_0_9_0F/source/main.c`: immediate presentation refresh on selected-zone reset, including paused path; updated version label.
- `tests/test_animation_tick_stability.c` and `tests/run_animation_tick_stability.sh`: adversarial landing/repeat/rewind/reset checks with 128 deterministic replay loops, 10 duplicate queries per tick, physics non-mutation assertion and native reset source contract.
- `tests/run_sa3ds_current_host.sh`: repeatable owner-data-free inherited regression gate.

## Tests actually run
- `sha256sum` WIP2 ZIP and `unzip -t`: PASS, parent matches recorded SHA.
- `bash tests/run_animation_tick_stability.sh`: PASS.
- `bash tests/run_sa3ds_current_host.sh`: PASS (0G 12 deterministic movement protocols and HUD/trace, 1D CAM synthetic, WIP1 visual source, WIP2 GPU batch, WIP3 animation timeline).
- `cc ... tests/test_sonic_animation_sync.c`: PASS.
- `cc ... tests/test_animation_observer_h4.c`: PASS (1024 physics-state non-mutation stress).
- `python3 tests/test_0_0_9_0F_runtime_source.py`: PASS.
- `bash tests/run_white_zone_visual_preflight.sh`: NOT RUN; requires user-owned original CHRMODELS.DLL and SONIC.PVM, unavailable in this runtime.
- Native `.3dsx` build: NOT RUN; devkitARM/libctru/Citro3D toolchain unavailable.
- Old 3DS XL: NOT TESTED.

## Parity / remaining work
Original source action slot mappings remain SOURCE-BACKED PARTIAL. This fix is a host-verified presentation determinism and reset invariant, not proof of retail transition logic. The one-tick landing and 0.5 motion-frame/tick playback policy are still PROVISIONAL. WIP1 texture and WIP2 GPU frame changes remain inherited. No Sega binaries, models, textures, or audio are distributed. Checkpoint E is not complete.
