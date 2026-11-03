# WCA39 — native ordinary homing against original E SARU tasks

Startinge91b2d5. The stage's real E SARU tasks now supply homing candidates and
current first-collider poses. Candidate task distances/order come from the
previous completed player/NPC CCL pass, collected independently of physical
body overlap. Outer flag40 and original task ID0 are used; SET kind29 is not a
task ID. Rings remain excluded. The list resets after NPC updates and before
new CCL reports; the next Sonic phase resolves still-live tasks before NPC
removal. Act/reset clears the list. Full original global CCL queue/SET lifetime
is still incomplete, so this is not a proof of every source task ordering case.

`SA1PhysicsWorld.homing` is optional and world-owned. NULL preserves White
Zone's existing empty-target specialization. Native stage binding is refreshed
after collision-region replacement, with the prior CCL cached-other category
for action14 cancellation. Native partner ID is0 because no partner player is
implemented; player-work special flags are not yet connected. The portable
services support these inputs without inventing them. Source quarter-LUT trig
and the MSVC RNG state are shared with implemented native Saru tasks; original
global RNG order, including missing actors/effects, remains unproven.

## Entry and movement phase

The common selector now separates action8->14 entry from its empty-list
specialization.494B80 starts timer-1/age0. A truly empty list/no partner writes
forward5 (10 special) and timer1. Otherwise it invokes492300 immediately,
including that helper's acceleration when no eligible target is found. A
nonzero post-call timer overwrites local forward with5, even for the special
player. Action14 movement then invokes492300 again in the SAME simulation tick.
Tracking bypasses normal gravity/acceleration. Other outcomes execute the
existing44C270/443F50 reconstruction once per helper invocation. Position and
animation are not advanced twice.

Subsequent action14 ticks use one invocation. The prior CCL cached-other
category0/2/3 cancels to action8/timer-1 before movement, rather than searching
for a nearest target to decide contact. The existing timeout/action-cancel/age
logic is retained. A negative entry timer and above-player target still apply
the source upward clamp/timer1 rule; the next invocation consequently decays
instead of resuming pursuit. No new upward-homing gameplay rule was added.

## Physical frame bridge

43EC90 calls Ninja Z, X, then negative Y rotations; the original784E20 element
updates confirm post-multiplication. Physical rotation is Rz(Z)*Rx(X)*Ry(-Y).
Its columns are port forward, physical up, and NEGATIVE port right. Upright
sourceY is4000 minus port heading; passing port yaw directly would choose the
wrong targets/turn direction. Source local sideways velocity also has the
opposite sign to the port's rightward component.

`pc2004_task_frame.[ch]` inverts the canonical principal-X matrix to recover the
raw Y field, then applies a new raw-Y delta while preserving physical up/X/Z.
Normal helper4B8E80 uses principal asin and source normal-pole Z0; that canonical
pole branch is handled separately. Inverse quantization rounds an encoded
integer rotation field; source TARGET bearing still truncates after65536 times
the exact inverse-tau double. These are different operations. Matrix inversion
and delta rotation are host checked, not a reconstruction of all original
rotation history, smoothing or exceptional Euler branches. The inherited
contact-frame/support-origin adapter and float precision remain part of CP4's
open retail parity work. Loop/exceptional-frame trajectories are not accepted
by this fixture.

## Verification

- Required `bash tests/run_sa3ds_current_host.sh`: PASS. Existing regression
  environments retain empty-list behavior. Explicit-link host runners include
  the newly referenced homing/frame modules.
- 119 independent Rz*Rx*Ry frame cases PASS, including tilted/overturned normal
  matrices, raw-Y wrap, delta turns and canonical normal poles. These are
  equations, not original runtime angle-history observations.
- Public-controller handoff PASS: two TRACK calls before position integration,
  correct raw-Y two-step turn/world velocity, one-call continuation, cached
  category cancel, empty-service/legacy equivalence, failed-search double
  acceleration, special nonzero-timer forward5 and upward-entry one-TRACK then
  decay. Kernel collection/ranking/partner/RNG tests also remain passing.
- Exact-owner evidence PASS: collection/pursuit/init/turn/RNG helpers, physical
  local-to-world rotation, normal-angle helper and Ninja Y element updates;
  bearing/negative-roll constants match. This is static evidence, not a retail
  movement trace.
- `bash tools/build_3ds.sh`: PASS. Final412344B code-only artifact SHA256
  `12b1d541ecb49985d9ed2eb8ec3c8ce593b7413a0aadaa790d9ddeaa171c6ced`.
- That exact artifact ran in Azahar2126.0/private Old3DS/Vulkan profile. Original
  act0 SET row26 fixture: hold jump, release one tick, second press at tick16.
  Entry reports2 calls/2 TRACK results,target3,timer0,age0.180ticks PASS with
  15calls/15tracks,2contacts,score10,bounce1,enemy taskdeath1,hurt0,170actor draws,
  max2 actor submissions. Paused end state was visually inspected. This is
  bounded native pursuit/attack evidence, not retail/hardware/full-course proof.
- Initial412344B/326c887d attempt pressed again at tick6 while below the enemy.
  It correctly yielded1 TRACK then timer2 decay, no contact; whole pursuit
  fixture FAIL. Only the input timing changed for the final attempt. Physics
  and the original upward-entry rule were retained. No unchanged pose/import
  oracle was repeated.

Final300-frame diagnostic includes paused tail:180ticks/6632ms,submit5836ms,
max411ms,minLinear26569600,maxCmd.272217. Do not report it as active gameplay FPS.
The optional private marker `emerald_coast_saru_probe.flag` contains `26 H` and
is removed after this check. Natural gameplay uses the same candidate/physics
path without that marker; the fixture is not course progression evidence.

## Remaining acceptance

Full controller/landing/loop/exceptional-frame and power-up/partner behavior,
retail trajectories, effects/trails/sound, complete CCL/SET/task/RNG ordering and
physical hardware remain open. Player hurt, bomb projectiles, death effects,
monitors/checkpoints, whale/events, full HUD/audio and natural course completion
remain incomplete. CP1/CP5/WCA and physics1to1 are not completed by this change.
