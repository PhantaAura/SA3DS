# WCA38 — native Sonic/enemy contacts, bounce, score and task death

Startingae07955. The native E SARU tasks now consume previous-pass CCL hits,
cache and damage flags through the existing shared source helper. After all
NPC updates, the player/NPC sphere pass clears old reports and produces the
next pass using owner-v2 collider/category records. Source ring ID11 is not
repurposed as an enemy identity. Actor IDs distinguish live native tasks from
SET kind29. Dead tasks leave the active list on the original action12 request.

Both flat and general controllers freeze the selected action/ball state before
terrain contact can rewrite it to a landing/departure state. For the supported
ordinary paths, intervening force helpers do not change those strength inputs;
this preserves49AA48's selected-state values without deriving attack strength
from the completed landing state. This is not a reconstruction of all task
status/player-work flags or exceptional actions. Unknown selector state does
not produce native contacts. Primary center follows the existing source5.4
anchor; broad radius uses queued world speed length plus source player height.

Player transform and broad speed use the queued post-player snapshot, taken
before spring/object commands and bounce can alter the live player. Current
player ball/ground status is separately used by the source bounce gate in the
NPC phase. The source query cursor is shared across callbacks; repeated damage
calls are not deduplicated or forced to award only once. CCL clear happens after
those callbacks, not before they consume previous hits.

## Correct world frame and score semantics

The previous result field was incorrectly named `bounce_local_velocity`.
Exact441280 first writes its arguments to motionwk world velocity at+4/+8/+C,
then calls43EC00 to derive the separate local Speed cache.4CDFE0 therefore asks
for world(0,2,0), even on a tilted task frame. The corrected result field is
`bounce_world_velocity`; the native executor changes world velocity only.
A tilted-frame regression proves that position/yaw/action/ball/frame remain
unchanged and physical-up is not substituted for world-up. The port still
recomputes local speed next tick; the source's independent local-cache tiny-X/Z
snap and x87 precision are not fully represented by this executor.

`game_score.[ch]` implements425C70: first award resets chain amount0, later
awards add with signed500 cap, every award sets120 timer, and total receives the
original award only.426070's active-stage-timer branch tests the OLD timer for
negative before decrement/clamp;0 becomes-1 for one tick. Host boundary checks
preserve this detail. Native active simulation ticks update the timer; full
original HUD/task priority and special timer modes remain unproven. Enemy/task
resets and act handoffs do not reset score; the explicit probe resets it only
for its controlled measurement. Original score HUD rendering remains pending.

51F680 is a one-byte immediate return in this exact executable. It is not a SET
persistence update. Source46C150 clears SET loaded flags and schedules task
removal. The current bounded native adapter retains a dead row until its normal
unload/reset; complete SET scheduler/persistence/camera arbitration is pending.
Death puff/fragments/animal are not replaced with fabricated effects.

## Verification

- Required `bash tests/run_sa3ds_current_host.sh`: PASS. Focused additions cover
  flat/general ball landing, subsequent idle and fresh jump attack snapshots,
  air-dash timeout state, tilted world bounce and score/chain/timer boundaries.
- Exact owner verifier:12 helper identities PASS, including441280/43EC00,
  425C70/426070 and the no-op51F680. Static source identity is not retail runtime.
- `bash tools/build_3ds.sh`: PASS. Final code-only406244B artifact SHA256
  `916e741bd322b0dbc35aee062318e54da2d12584a85d36cf0e9cba8d6d083b14`.
- That exact final artifact ran in Azahar2126.0/private Old3DS/Vulkan profile:
  original act0 SET row26, normal jump input,180ticks;2 contacts,1 award,total10,
  1world bounce,1task death,0hurt contacts,165actor draws,max2 actor submissions,
  actions mask000010e7. The fixture pauses itself. This is bounded native attack
  evidence, not a retail trajectory/visual comparison or physical hardware.
- An earlier406212B/8452e599 artifact passed the same fixture before the final
  score-reset placement correction. The final artifact was rerun after that
  correction; results match. No unchanged pose/import oracle was repeated.
- Final300-frame diagnostic includes paused tail:180ticks/6599ms,submit5845ms,
  max428ms,minLinear26569600,maxCmd.267822. This is not active gameplay FPS proof.

The optional private probe marker contains `26 C`: choose original SET row26,
place Sonic on imported nearby support and issue a normal jump. This marker is
removed after verification and is not required for ordinary gameplay.

## Acceptance still open

Player hurt is detected by CCL but its action/ring loss/invulnerability/death
handler is not connected. The jump fixture had no hurt contact and cannot prove
that path. Bombs/held bomb/projectile collision, sound/dust, death effects,
animals, score HUD, other players/categories/forms and full global CCL task order
remain incomplete. Secondary action72 is unsupported. Native power-up/player
flags4/6 are not yet modeled. Homing still needs its native target registry,
raw-yaw/task-frame bridge and source double invocation on entry. Source floor,
visibility, special world mode, SET lifetime and global RNG order remain open.
Full CP1/CP5/CP7/WCA and physics1to1 are not accepted by this bounded fixture.
