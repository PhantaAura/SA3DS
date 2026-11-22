# SA3DS handoff

Authoritative branch: `PhantaAura/SA3DS main`. The current working checkout is
`/Users/emeraldhunter/Documents/ChatGPT/Sonic Adventure 3DS/work/SA3DS`.
The temporary checkout disappeared; this persistent checkout was recovered from
pushed9726ecd. The older Desktop/Documents baselines are not this checkout.

Next milestone is **1.0: normal boot → Emerald Coast clear → results**.
It remains incomplete. Finish the playable route before polishing nonblocking
systems; mark deviations CANDIDATE. The full nine-part fidelity goal in the
README remains active after 1.0.

## Next work

1. Finish the automatic-loop collision response using the regenerated incoming-state trace.
   Owner paths, deferred command26, input-timer exception and held-input steering
   now exist as an opt-in CANDIDATE. Steep support is still unresolved; the native
   candidate is off by default. See `docs/PC2004_DASH_PANEL.md`.
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

## Automatic-loop candidate and workspace recovery

The converter `tools/build_pc2004_loop_paths.py` reads five supported4BB1F0 paths
(405 points) across both acts from the exact executable. Other path callbacks
are explicitly omitted in package metadata; this is not complete path support.
The player command retains action, defers activation to the next player update,
and bypasses the control timer only while status2000 is active. Held input
advances path distance, turns the task frame and adds the original-.8 normal
force; neutral input does not advance the path. Commands share priority with
spring commands, and resets clear borrowed path references.

Native experiments require BOTH external `assets/ec_loop_paths.sa3dspak` and
`ec_loop_candidate.txt` containing1 under the SA3DS SD folder. The flag remains
necessary because the isolated candidate still loses steep support; no completed
loop, normal-start course, emulator play or physical hardware check is claimed.
The unaccepted generic contact-tolerance experiment was discarded.

The previous temporary checkout and private recordings disappeared before the
pending path-distance capture could be inspected. Source was recovered from Git,
the candidate was restored here, and required checks were rerun here. The reference was then regenerated in the persistent external lab; missing older
files are not acceptance evidence. Keep long-lived reference artifacts outside
this tree.

## Restored controllable original and matched loop steering

Persistent private lab:
`/Users/emeraldhunter/Documents/Codex/2026-11-22/sa3ds-reference-lab`.
The clean private reference uses the owner's CHRMODELS_orig.dll in place of the
installed Mod Manager proxy, without modifying the owner install. This fixes
its startup dialog about missing mods/.modloader/Profiles. The game now renders
Emerald Coast in a474x360 captured window and accepts game-local logical control;
no OS keyboard/mouse injection is used.

`held-forward-state-v2/frames.csv` has360 original updates, zero capture errors,
and the92-byte incoming-state sidecar. Path mode is active17..95 and exits at96
while grounded. `capture-tools-v2/check-loop` with `compare_loop.py` checks all79
active updates in the portable C43C130 component: zero failures, exact distance,
maximum heading error1 BAMS. This is component evidence, not full physics parity.

The incoming-state fields resolve an important replay error: source world tasks
move Sonic between main updates. For example, before17 the actual X is2114.154,
while update16's post-main X was2125.607. Do not use the previous post-main
position as the next input state. A row0/12 boost replay omitting world tasks is
not a matched full-stage loop acceptance run.

Next compare the actual native loop candidate with the proper world-task order
and replace the incomplete steep-contact response. Force integration, terrain
response, normal start-to-goal/results and Old3DS freezes remain unverified.
The candidate stays off by default. Build/checker sources and private scenario
preparation are in tools/pc2004_retail_capture/native; fresh capture requires a
fresh reference process and the expected-game binding. Old capture formats
remain supported; all original data, DLLs, screenshots and traces stay external.
