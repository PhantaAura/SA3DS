# SA3DS cumulative error and recovery history

## ENV-001 — Scheduled execution ClientError
- Observed: Oct 12, 2026, scheduled runs 1–3 at 17:00, 18:00, 19:00 EDT. Python/container operations returned `ClientError`, preventing source extraction, modification and compilation.
- Earlier unsuccessful approaches: retrying the same Python/shell execution and attempting alternative Python execution without an accessible source worktree. Do not repeat these unchanged indefinitely.
- Oct 12 23:00 EDT recovery: `container.exec` successfully executed shell, extracted and hashed the exact 1D archive, and ran C/Python source tests. Original archive SHA-256 matched `5409a6aa2d5776975389a4ec22ebadfb0acb1c58631ec1904eb0dacff0b67517` and `unzip -t` passed.
- Root cause: **unknown**. Recovery proves execution works in this run; it does not prove scheduled environment reliability for subsequent runs. Check the environment and latest artifact at the start of every run.
- Regression gate: `sha256sum`, `unzip -t`, compile/run a host C test before claiming productive source access.

## VIS-001 — Original renderer silently assumes material black means white and forces material alpha to FF
- Reproduced in `runtime_0_0_9_0F/source/sonic_gpu.c::material_rgba` and host renderer; original code contained `if (r==0 && g==0 && b==0) r=g=b=255` and returned alpha FF unconditionally.
- Owner evidence: current Sonic model materials have alpha FF and no black RGB, so no confirmed retail Sonic visual discrepancy from this code path.
- Fix: source AARRGGBB -> RRGGBBAA conversion preserving all channels, used by GPU and host software rasterizer; effective host alpha now modulates texture and material alpha.
- Regression: `tests/test_sa3c_gpu_preflight.c` synthetic black, semi-alpha and transparent colors; original owner render smoke passes.

## VIS-002 — Missing SA3C/SA3T cross-package validation before GPU uploads
- Potential failure mode: a texture archive with matching slot count but inconsistent dimensions/source formats/global indices could reach `C3D_TexInit` and render incorrectly; invalid PICA200 texture dimensions would fail only during GPU init.
- Fix: host-testable preflight with distinct error classifications and Old 3DS developer HUD statistics, invoked before texture allocation.
- Regression: `tests/test_sa3c_gpu_preflight.c` deliberately mutates metadata dimensions, GPU dimensions, material references and count; all expected errors are detected.
- Owner source check: all 16 exact Sonic textures passed. **No mismatch was found in the original owner assets.**

## TEST-001 — Full 0.0.9.1C owner-suite timeout
- Oct 12 23:00 EDT: `run_0_0_9_1C_tests.sh` with owner 2004 sonic.exe and system.zip exceeded the 90-second execution limit after inherited 1B checks passed and at `SET synthetic and owner-data decode`.
- Status: timeout, **not** a failed correctness assertion and **not** a full pass.
- Avoid: do not repeat the full expensive suite unchanged in every hourly run. Profile/isolate the SET portion only if it is on the current critical path; prioritize Sonic visuals.

## VIS-003 — Shared GPU scratch vertex span overwritten by later queued draw submissions
- Found Oct 13, 2026 00:00 EDT in `runtime_0_0_9_0F/source/sonic_gpu.c` and `lab_gpu.c`. Prior code allocated only max-mesh vertex count and rewrote the same buffer for each Sonic mesh; the lab rewrote the same first six vertices for each zone and then for distance markers. Citro3D draw submission may queue GPU reads until frame submission; this is a concrete source-level buffer lifetime hazard. Actual Old 3DS visual corruption is **not yet demonstrated**.
- Root cause in source: mutable scratch data reused before GPU command completion. The previous per-draw `GSPGPU_FlushDataCache` does not preserve the bytes against subsequent CPU writes.
- Fix: `sa3c_gpu_batch` budgets all model instances and reserves disjoint per-draw vertex ranges within a bounded linear allocation; the White Zone lab reserves distinct zone spans and a separate marker batch. The arena is reset only at the next synchronized frame. Source status: integration complete; hardware pending.
- Regression: `bash tests/run_white_zone_gpu_batch.sh` compiles and executes host C adversarial tests (duplicate model references, disjoint spans, reset, overflow, bad refs, empty, truncation) and checks both native draw paths. Inherited 0G and 1D host tests pass.
- Avoid: do not reintroduce max-mesh scratch allocation, reuse the same GPU vertex pointer for multiple pending draws, or assume cache flush is a GPU completion barrier.
- Pending: actual Citro3D compile and hardware display validation, including potential command queue limits and linear memory budget.

## ANIM-001 — Repeated same-tick selector queries consume landing presentation
- Found Oct 13 2026 01:00 EDT in `src/game/sonic_animation_runtime.c`: landing is selected on airborne->grounded transition and `previous_grounded` is immediately updated. Calling selection again in the same `sim_frame` would switch to idle, even though no new physics tick occurred. Observer also recomputed the same frame, and non-monotonic trace replay could inherit old intent/clip timing.
- Source-level reproduction: `tests/test_animation_tick_stability.c` replays jump->landing, queries every frame 11 times, rewinds, and resets (128 deterministic replays). Prior source fails repeated landing idempotence.
- Fix: cache immutable observer/selector snapshots keyed by completed `sim_frame`; reset timeline on backward `sim_frame`; preserve physics-state non-mutation. Explicit zone/player resets still clear both structures.
- Regression: `bash tests/run_animation_tick_stability.sh` and `bash tests/run_sa3ds_current_host.sh` pass.
- Fidelity note: landing duration and 0.5 source-frames/tick remain PROVISIONAL, not claimed retail behavior.

## ANIM-002 — Reset-selected-zone could render stale previous-zone Sonic pose
- Found Oct 13 2026 01:00 EDT in `runtime_0_0_9_0F/source/main.c`: `reset_selected_zone()` reset player/animation state but did not refresh `g_observation` and `g_selection` until next physics tick. The draw path samples `g_selection` each render.
- Fix: refresh presentation snapshot immediately after zone/player reset, including paused cases.
- Regression: `tests/run_animation_tick_stability.sh` checks reset pose source integration; full 3DS runtime test pending.
- Avoid: do not rely on a future physics step to refresh render state after an immediate scene reset.

## ANIM-003 — Selected ACTION may reach runtime without complete pose sampling validation
- Found Oct 13 2026 02:00 EDT: the native loader checked SA3C references and texture metadata, but did not preflight the exact 17 runtime-ID-to-ACTION slots at multiple frames. An empty action, unsupported interpolation, non-finite key, or a non-finite sample-frame argument could reach the draw path and produce a generic White Zone pose/draw failure (or invalid GPU coordinates). This is a **source-level coverage gap**, not evidence that original Sonic assets are malformed.
- Fix: new `sonic_action_preflight.c/.h` tests each mapped action at start, midpoint and last frame, checks slot/empty/interpolation/finite local+world matrices, reports exact runtime ID, slot and frame on failure, and runs during owner package load. The action sampler now rejects NaN/Inf frame inputs before modifying pose output. Enumerating the existing binding table avoids duplicating mappings.
- Regression: `bash tests/run_sonic_action_preflight.sh` tests synthetic valid action, out-of-range/empty slots, unsupported interpolation, NaN key and invalid frame; ASAN/UBSAN host run passed. `bash tests/run_sa3ds_current_host.sh` inherited suite passed.
- Known limits: **No owner-generated SA3C was available in this run**; original 17 clips and actual Old 3DS rendering remain unverified. Preflight deliberately does not change animation timing or action selection. A native `.3dsx` has not been compiled. On failure, owner package loader returns an actionable diagnostic rather than silently changing the animation.

## TEST-002 — Initial synthetic preflight fixture had cyclic node links
- During WIP4 test creation, `tests/test_sonic_action_preflight.c` initially zero-initialized the synthetic NODE record. In SA3C, `0xFFFF` (not zero) denotes absent model/child/sibling. The zero child/sibling values formed a cycle and the first positive preflight assertion failed with a hierarchy error.
- Corrected the fixture by explicitly writing `0xFFFF` for the absent model, child and sibling. The production source and original owner assets were not implicated.
- Regression: the corrected fixture passes with `-Wall -Wextra -Werror` and ASAN/UBSAN, and the full cumulative owner-free suite passes. Avoid creating zero-initialized SA3C hierarchy fixtures without explicit invalid-reference sentinels.

## ANIM-004 — Three-frame ACTION preflight misses hidden corrupt keys
- Found Oct 13 2026, scheduled 03:00 EDT pass: WIP4 only samples first/middle/last ACTION poses. A synthetic five-key track at frames 0,1,2,3,10 with NaN at frame 1 produces finite poses at 0,5,10 because the interpolator never reads that key. Source-level coverage gap, not a retail-data failure.
- Fix: O(total active keys) scan at owner package load for finite values, ordered key times, nonempty enabled tracks and key/channel range. Expose channel/track/key diagnostic; provide a portable read-only owner-SA3C CLI. Retain WIP4's three pose samples.
- Regression: `bash tests/run_sonic_action_preflight.sh` (hidden NaN, descending key, duplicate time allowed, empty track, bad range; synthetic complete SA3C CLI success and hidden-NaN failure), `bash tests/run_sa3ds_current_host.sh` PASS, GCC ASAN/UBSAN PASS.
- Owner-data note: No original SA3C available in this run; cannot conclude any original Sonic key is invalid. Do not impose unmeasured frame upper bounds or ban duplicate timestamps.
- Native 3DS compilation and Old 3DS display remain pending.

## VIS-004 — Unvalidated SA3C vertex floats / expanded triangle lists could reach PICA200
- Found Oct 13 2026 04:00 EDT: the native `sonic_gpu.c` uses `GPU_TRIANGLES` for every converted mesh and reads 8 float32 components from each referenced vertex, but the loader's previous reference/texture/ACTION checks did not validate finite geometry components, expanded triangle-list counts or retained Basic source primitive class.
- Reproduction: synthetic SA3C mesh with 3 indices and a NaN UV / Inf normal; WIP5 loader would accept this structure while WIP6 returns `NONFINITE_VERTEX` with mesh/index/vertex/component. Synthetic two-index mesh is rejected as `TRIANGLE_LIST`.
- Root cause: incomplete geometry preflight coverage, not a confirmed retail asset defect.
- Fix: `sa3ds_geometry_preflight()` at native owner-package load, prior to GPU init. It accepts empty meshes, validates the supported strip/quad source classes and rejects malformed triangle counts, bad index bounds and nonfinite vertex values.
- Regression: `bash tests/run_sonic_geometry_preflight.sh` PASS; `bash tests/run_sa3ds_current_host.sh` PASS; GCC ASAN/UBSAN synthetic geometry test PASS.
- Avoid: treating source primitive class as direct GPU draw mode (owner builder has already triangulated), banning arbitrary UV coordinates, or claiming that synthetic safety tests establish original Sonic visual fidelity.
- Pending: owner-generated SA3C preflight, devkitARM/libctru build and actual Old 3DS display.

## VIS-005 — Incorrect assumption that SA3C/SA3T origin hashes must match (avoided before release)
- During 04:00 EDT owner visual CLI development, an initial implementation compared the two source SHA-256 fields as if both archives came from one binary.
- Converter inspection disproved the hypothesis: SA3C is sourced from CHRMODELS_orig.dll, SA3T from SONIC.PVM. Their hashes are expected to differ. This check would reject valid game assets.
- Fix before checkpoint: removed cross-package hash equality requirement, retained SA3C TEXS ↔ SA3T metadata compatibility validation.
- Regression: `tests/run_sonic_owner_visual_cli.sh` explicitly accepts synthetic owner packages with different source hashes; rejects malformed metadata, NaN UV, bad index and missing texture archive.
- Do not reintroduce SHA equality without a provenance schema that links actual related sources. No original owner package was inspected this run.

## VIS-006 — Later 0G native runtime did not inherit WIP1-WIP6 0F renderer/loader fixes
- Found Oct 13 2026 05:00 EDT, overall Run 10: source comparison of `runtime_0_0_9_0G` against guarded `runtime_0_0_9_0F` showed the 0G Sonic renderer still reused one scratch span for queued GPU draws, coerced black material RGB to white, did not run GPU visual/geometry/ACTION preflight, and failed to refresh animation selection immediately on zone reset. Its lab geometry also reused queued zone vertices.
- Cause: earlier WIP1-WIP6 modifications targeted the 0F runtime while 0G was retained as the White Zone parity-HUD/trace/camera candidate. A later native build from 0G could silently omit validated improvements. This is a source integration gap, NOT proof of actual Old 3DS visual corruption.
- Fix: port `sonic_gpu.c`, `sonic_gpu.h`, `lab_gpu.c` from guarded 0F into 0G; merge 0F's preflight/diagnostic/reset fixes into 0G `main.c` without replacing 0G parity HUD, camera and trace behavior.
- Regression: `python3 tests/test_0g_visual_reintegration.py` checks renderer parity, preflight order, pose refresh and 0G behavior preservation; `bash tests/run_sa3ds_current_host.sh` PASS including existing host GPU batch/geometry/ACTION/physics tests. No devkitARM/libctru or real hardware test available.
- Avoid: copying the entire 0F main over 0G (would discard 0G trace/camera/HUD); falsely calling source-contract checks native compilation.

## TEST-003 — Original 0G title assertion rejected legitimate WIP7 banner
- Initial post-merge host gate failed `tests/test_0_0_9_0G_runtime_source.py` solely because it required the exact 0G console title.
- Fix: accept the original 0G or explicit WIP7 White Zone banner; retain all existing parity camera/trace/physics non-mutation assertions. Post-fix full host gate PASS.

## ASSET-001 — RomFS-only build gate prevents code-only native hardware candidate
- Found Oct 13 2026 06:00 EDT, overall Run 11: 0G Makefile required
  `romfs/sa3ds/sonic.sa3c` and `.sa3t` at compile time, and runtime only read
  the RomFS pair. A developer with devkitARM but owner packages on SD could
  not build a code-only `.3dsx` for a real visual test.
- Root cause: original packaging path coupled owner-data availability to
  compilation. This is a build/integration limitation, not a confirmed Sonic
  rendering defect or an observed native build failure.
- Fix: portable atomic pair loader with RomFS precedence, SD fallback,
  per-file 16MiB cap, exact-length reads, and cleanup. `ASSET_SOURCE=sd`
  bypasses only compile-time owner-file presence checks. Native loader still
  validates SA3C/SA3T and reports selected source in HUD.
- Regression: `bash tests/run_owner_pair_loader.sh` PASS including 3 synthetic
  pairing scenarios, invalid/oversized files, fallback and make-mode checks;
  ASAN/UBSAN C fixture PASS; full cumulative host suite PASS.
- Avoid: mixing RomFS SA3C with SD SA3T, distributing owner packages in a
  checkpoint, assuming that a source-contract test is a native `.3dsx` build.
- Pending: real SD service/paths, native compile, owner data and Old 3DS test.

## TEST-004 — New host fixture initially rejected by GCC -Wformat-truncation
- Reproduction: first `bash tests/run_owner_pair_loader.sh` failed under
  `-Wall -Wextra -Werror` because synthetic `snprintf(big,512,"%s/oversized",s)`
  could theoretically truncate for a 512-byte directory.
- Fix: use a 1024-byte destination in the test fixture; repeat test PASS.
- No production loader source or Sega data was implicated.

## SIM-001 — White Zone physics tied to rendering cadence (Run 12)
- Found 2026-10-13 07:00 EDT by inspecting 0G `main.c`: one call to
  `sa1_physics_step_steam_sonic_flat()` per `aptMainLoop` iteration, rather
  than per 1/60-second simulation interval. At 30 render FPS, physics would
  advance 30 ticks/second; at >60 render FPS, it could advance too quickly.
  This is a confirmed SOURCE scheduling defect, not a measured Old 3DS FPS.
- Fix: portable integer-phase 60Hz scheduler in `src/game/white_zone_tick.[ch]`;
  the 0G main loop executes 0..4 physics ticks per rendered frame. Input
  button edges are latched until a tick, consumed once on the first catch-up
  step; held/analog inputs remain available on all steps. Camera follow and
  trace writing now occur per simulation tick. SELECT+A toggles pause;
  zone reset/pause transition clears accumulated simulation time.
- Conservative stall policy: clamp elapsed wall time to 250ms, cap catch-up
  at 4 ticks/frame, record skipped catch-up ticks in the HUD. This is a
  defensive runtime policy, NOT a verified retail catch-up behavior.
- Regression: `bash tests/run_white_zone_tick.sh` and full
  `bash tests/run_sa3ds_current_host.sh` PASS. Host UBSan compiled/executed
  PASS. Native devkitARM build and actual 3DS frame pacing remain PENDING.
- Avoid: adding one physics step per draw, delivering a button press once
  per render frame (would lose input at >60 FPS), or claiming 60 FPS hardware
  rendering because the simulation scheduler is 60Hz.

## PKG-001 — SD build could accidentally embed local owner assets (Run 12)
- Found 2026-10-13 07:00 EDT in 0G Makefile: WIP8 `ASSET_SOURCE=sd`
  skipped the RomFS asset-presence check, but `_3DSXFLAGS` still used
  `--romfs=.../romfs`. If a previous local owner conversion had written
  `sonic.sa3c`/`sonic.sa3t` there, an SD-mode `.3dsx` would STILL embed those
  Sega-derived files. Source inspection proves the packaging path hazard;
  no actual `.3dsx` containing owner data was observed.
- Fix: select a separate `romfs_sd/` tree with only a code-only README for
  `ASSET_SOURCE=sd`; keep the existing owner-data RomFS build path intact.
- Regression: `tests/test_owner_asset_build_mode.sh` resolves the Makefile
  ROMFS variable through GNU Make in both modes and checks the SD tree for
  owner package extensions. Full inherited host suite PASS. Native packaging
  must be inspected on a Mac with devkitARM before claiming final binary
  isolation.
- Avoid: relying solely on `check-assets` or the absence of owner files in
  this particular ZIP to prove future SD-mode builds are code-only.


## MAC-001 — macOS Clang does not declare mkdtemp() in host test
- Report: owner's macOS Clang `-std=c11 -Werror` host test failed at `tests/test_owner_pair_loader.c:37` with `call to undeclared function 'mkdtemp'`. This is a host regression test portability issue, not evidence of a Sonic gameplay defect.
- Source cause: existing `_XOPEN_SOURCE 700` narrows Darwin SDK's declaration visibility; `mkdtemp` may be hidden unless `_DARWIN_C_SOURCE` is defined before system headers. The test already includes `<stdlib.h>` and `<unistd.h>`.
- Fix: request `_DARWIN_C_SOURCE` only on Apple builds while preserving POSIX `_XOPEN_SOURCE 700` and the original runtime/test semantics elsewhere. No 3DS code, asset loader code, or game physics changed.
- Verification: rerun `bash tests/run_owner_pair_loader.sh` and `bash tests/run_sa3ds_current_host.sh` with Mac Clang. Linux host/synthetic Apple-preprocessor checks can verify the code path but are NOT macOS SDK compilation proof.
- Avoid: disabling `-Werror`, adding a guessed declaration of `mkdtemp`, or replacing the secure directory creator with `mktemp`.
- Native `.3dsx` and Old 3DS boot still pending.

## BUILD-002 — Malformed native HUD `printf` during WIP9 integration (macOS compiler)
- Observed: 2026-10-15 on the owner's Mac, `runtime_0_0_9_0G/source/main.c:176-180`, native devkitARM compiler emitted missing terminating `"`, stray integer suffix `1HAssets`, and `printf` receiving an `int` in place of `const char *`. This is a real source syntax error, not an asset/model defect.
- Root cause: two HUD `printf` calls were interleaved during WIP9 source integration, leaving a raw newline inside the first string and the `1HAssets` suffix after the second call. Prior host test suites only checked presence of specific tokens, not global C lexical integrity; they did not catch the malformed native source.
- Fix (MacFix2): restore two separately terminated formatted statements, one `Assets` at row 27 and one `60Hz/Drop/pause` at row 28. No gameplay values or simulation behavior changed.
- Prevention: add `tests/test_0g_native_hud_lexical.py` to cumulative suite to detect raw line breaks inside native C strings, unbalanced delimiters, and exact HUD call integrity. Verify that the old MacFix1 source fails this test.
- Verified here: lexical regression and cumulative host suites; native devkitARM build still needs owner's Mac rerun. Do not claim `.3dsx` success until observed.
