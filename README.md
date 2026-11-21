# Sonic Adventure 3DS

SA3DS is a native, offline 3DS reimplementation of the owner's PC2004 Sonic
Adventure DX. Old 3DS is the hardware baseline. The original executable and data
remain private; the app loads packages generated from the owner's installation.

**Next release milestone: 1.0 — normal boot → Emerald Coast clear → results.**
This milestone is not complete. The Act 1 dash-panel/loop route still breaks,
and an uninterrupted course clear has not been demonstrated. Nonblocking visual
and behavioral differences remain **CANDIDATE** while the playable route is finished.
The full fidelity goal continues after this first playable release.

Read [RESUME.md](RESUME.md) for the next work and
[PARITY_STATUS.json](PARITY_STATUS.json) for current evidence. Git holds previous
versions, test reports and checkpoint history. There is one active `runtime/`.
Technical format and original-behavior evidence remains in `docs/`.

## Roadmap and release gates

| Milestone | Work mapped from the original | Acceptance before release |
| --- | --- | --- |
| 1.0: Emerald Coast clear | Follow original starts, SET movement objects, terrain route, required events, act handoffs, goal and results. | One uninterrupted normal-start native run reaches results; death/checkpoint recovery works; known deviations are listed. |
| Fidelity pass | Match controls and animation, loop/wall/landing collision, stage camera, objects and whale behavior against the exact original. | Matched-state comparisons and focused native checks cover each changed behavior; remaining candidates stay explicit. |
| Old 3DS acceptance | Resolve movement freezes and validate renderer, audio and external SD replacements on the baseline hardware. | A physical Old 3DS completes the route with recorded performance and no route-breaking failures. |

The nine continuing work areas are controls/facing, animation, complete stage
geometry, terrain/camera, interactive objects, scripted events/whale, gameplay
progression/audio, renderer fidelity and external SD asset replacements. Reaching
1.0 does not establish full PC2004 parity or complete those nine areas.

Before implementing a route behavior, identify its original task, placement,
activation condition and handoff. Existing owner SET/CAM/terrain importers and
[route-blocker evidence](docs/PC2004_DASH_PANEL.md) supply the starting map.
Use the controllable original-game reference for uncertain behavior; video
seconds and game-update counts are different clocks.

## Build and run

```sh
sh tools/build_3ds.sh
```

Use `dist/SA3DS.3dsx`. The script stages the code in a path without spaces and
builds the external-SD mode. Keep the CPU texture-upload path, owner-pair loader
and boot diagnostics. Owner-generated assets belong on external storage, never
in this repository. Follow [the owner-pose workflow](docs/PASS11B_SOURCE_POSE_ORACLE.md)
for its exact private input requirements; consult each converter's `--help` for
external output paths.

## Verification and contribution

Run `bash tests/run_sa3ds_current_host.sh` for owner-data-free changes. Pose or
import changes also require `tests/run_pass11b_source_pose.sh` with the exact
owner inputs. Use `tools/build_3ds.sh` for native changes. Add only focused checks
that distinguish the intended original behavior; do not rerun unrelated native
scenarios without a reason.

Finish one behavior, check it, update the handoff/status and commit it. Push
source-only commits to `PhantaAura/SA3DS main`. Make a release when a milestone's
acceptance run actually succeeds; do not publish a numbered success for a build
alone. Never commit original game files, converted assets, private traces or
compiled releases.
