# SA3DS handoff

Authoritative branch: `PhantaAura/SA3DS main`. The current working checkout is
`/private/tmp/sa3ds-package-check.C7aBCm`; the older Documents workspace is not
this checkout. Current behavior commit: `b790297` (ground braking/turn-around).

Next milestone is **1.0: normal boot → Emerald Coast clear → results**.
It remains incomplete. Finish the playable route before polishing nonblocking
systems; mark deviations CANDIDATE. The full nine-part fidelity goal in the
README remains active after 1.0.

## Next work

1. Implement the original automatic-loop path entry and held-input steering,
   including status0x2000's control-timer exception and source normal-force
   addend. The current port lacks this behavior; terrain-only fixes do not
   establish a valid loop clear. See `docs/PC2004_DASH_PANEL.md`.
2. Walk each required act handoff, spring/pad chain and event from the original
   stage map; finish the whale/progression pieces that block reaching the goal.
3. Run normal start → goal → results and death → checkpoint recovery on native
   software; publish 1.0 only after that acceptance succeeds.
4. Then refine remaining animation/physics/visual differences and verify the
   reported movement freezes on physical Old 3DS.

## Current evidence and limits

- Ground braking now keeps facing, enters action13/request20 and resumes through
  turn-around30; focused checks, the cumulative host gate and native build pass.
- Four owner-recorded braking-force updates match within 5.96e-7 with the actual
  tilted task frame; this is component evidence, not a matched terrain replay.
- The original reference recorded 1,800 Sonic updates; the playback component
  matched 1,552 checked updates across eight seeded spans (240 excluded).
- Consolidation also passes the cumulative host gate and required native build.
- Native play of the braking candidate is pending; no new emulator or physical
  hardware result is claimed. The loop remains unresolved, and freezes are not
  proven fixed. Unsupported task-controlled reversal and exceptional contacts
  remain outside the ordinary braking path.

Private reference material stays outside Git. The isolated Mac reference uses
640×480, a Wine virtual desktop, the tested D3D8-to-D3D9 wrapper and a single
OpenGL command thread. See `tools/pc2004_reference_macos/` and
`tools/pc2004_retail_capture/native/`. Capture only Sonic's window and game-owned
controller state; the owner uses their Mac concurrently. Do not use the replaced
Windows music as the music oracle. Use the owner's local music files.

Use `runtime/` and `tools/build_3ds.sh`; output is `dist/SA3DS.3dsx`.
Run the required host gate once per finished source behavior, the exact-owner
pose/import gate when applicable, and only the necessary native checks. One
behavior per commit; update this file and `PARITY_STATUS.json` instead of making
new checkpoint documents. Earlier baselines and evidence reports remain in Git.

The controllable original now starts directly in Emerald Coast in the isolated
small Wine window. A source-neutral physics/animation scenario at the owner pad
placement, with active logical forward held, records360 updates: path mode enters
at17 and exits grounded at96. The identical neutral-input scenario falls, so the
old coasting fixture is not a loop-clear acceptance test. Neither is a full-course
normal-start acceptance run. A straight-line scripted approach from the original
spawn stops at shoreline geometry; it is not a verified course route.

Original active analog is3B0E7A0/4, read by43BDC0. Recorder version1's filtered
fields came from unused3B0E368; do not use them for control parity. Version2 fixes
that timing/source and explicitly labels legacy captures. Existing playback
component evidence does not depend on those input fields. Inject only after a
ready player task exists; injection during Wine startup can stall initialization.
Use one capture per fresh reference process until repeated-capture behavior is
established. Private scenario files/recordings remain under `/private/tmp/sa3ds-ec-reference-driver`.
Recovered loop task pseudocode is private at `/private/tmp/sa3ds-loop-task-recovery/output`;
4BB1F0 is the automatic loop constructor/poller, not a terrain response.
