# PC2004 dash panel reconstruction

Reference is the owner's exact executable SHA256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Private executable/decompiled text and converted models remain outside Git.
The portable task/direct-boost module and native external-asset integration
are implemented. Complete Emerald Coast gameplay is still unproven.

The task is7A4450 (SET type3, list3), direct-speed helper441490, primary
form7-versus-form6 collision41B110 and capsule axis419470. The owner-data-free
module accepts the previous CCL pass's already-arbitrated first player hit.
It does not replace that first hit with the nearest player or search past a
nonplayer hit. Normal activation/culling remains caller-owned. The release
profile4F88A0 returns zero, excluding the debug property-edit branches.

Initialization defaults nonpositive SET Scale.x to14 speed and Scale.y to60
frames, remembers player13, and returns without drawing/enqueuing. Active ticks
draw and decrement positive lockout before inspecting the previous hit. Only
player ground status bit0 permits boosting; bit1 alone does not. A positive
remaining lockout blocks the remembered player, while another player can boost
and replace that lockout immediately. Duration is ceil(Scale.y), converted to
int32; the same value's signed low16 enters Sonic's control timer. Out-of-range
int32 durations are explicitly unsupported instead of depending on a host cast.

441490 writes local(speed,0,0), task angles(SETX,4000-SETY,SETZ), motion yaw,
world-transformed speed and control timer immediately. It does not issue a
spring command, change action, select animation or clear ground status. The
port bridge changes its internal support anchor when Up changes to retain the
same source task position. Existing player timer filtering applies next tick.

The original panel record has zero center, capsule radius10, half-segment4.44,
and record rotationX4000. Its segment axis is SET Rz*Rx*Ry applied to localZ,
also the recovered player physical-forward column. The narrow-phase predicate
uses Sonic's queued primary center/radius, tests the infinite-line distance,
then strict endpoint-sphere bounds outside the segment. No collision impulse
or secondary attack collider is invented. This is source-equation recovery,
not a proof of x87 bit-exact boundary arithmetic or complete CCL ordering.

The display7A4360 uses original ACTION8B8BC4: three hierarchy nodes, eight frames,
one rotation channel/two keys. It draws with SETY+8000 and advances one frame
per display update, wrapping at8, with the source pause gates. The owner converter now exports the three Basic components once, plus all
integer local poses; the reader builds the animated hierarchy using every TRS
component despite static UNIT/BREAK flags. Raw angle interpolation preserves
the long rotation0->57343, rather than substituting a signed shortest arc.
The external package is39008bytes,62 triangles and7 component textures.
Geometry is uploaded once per component through the preserved CPU path.

Native activation retains SET clip flags and360000/360100 squared distances,
insertion order within panel tasks, prior-hit timing and category4's256-slot
limit shared with spring tasks. Panel collisions use the primary center queued
in Sonic's phase before list2 spring writes; hit consumption uses current ground
bit0. Panel object phase follows springs, with ring tasks still handled by the
existing separate ring scheduler. Complete ordering among all list3 object
classes, secondary collider interactions and source loading timing remain open.
The native camera's prior rendered frustum currently supplies the display's
30-unit visibility test, so exact original camera/culling timing is unproven.

The original-placement fixture is explicit development setup, not natural
course completion. A first chosen original row3 had no collision region beneath
it and failed setup. Owner-terrain inspection identified row9 as supported.
That placement boosted Sonic at tick3 withspeed10/control69/lockout69 and world
velocity(9.992,.055,-.386). An initial observation gate required all8 phases in
actual rendered frames; it failed because the preview advanced several60Hz
ticks between submissions. Measurements now report simulated phases and
rendered phases separately, requiring all8 simulated phases and actual native
component submissions. This does not change playback timing or game behavior.
The follow-on inclined/wall movement remains part of the unfinished collision
and loop parity work; no full traversal or original camera result is claimed.

Focused checks cover source defaults, ceil/low16 distinctions, first-frame
behavior, grounded/per-player gating, expiry order, full-frame direct speed,
retained action/animation/task position and capsule side/end boundaries.
`tools/verify_pc2004_dash_panel.py OWNER_SONIC_EXE` checks exact owner identity,
inspected function digests, calls, branch operands and collision/action records.
These checks do not establish a retail runtime trace, course completion or
hardware success.


## WCA26 measured result

Final code-only native build379344bytes, SHA256
`c99a67a8c0daef4d6f0f3e6e5584af92800fe997bb31a1baf1fca9903480f5b8`.
Exact owner panel asset39008bytes, SHA256
`a0f4780f37034e10c80591bbc6538709ccf6251749d9ff8f67fab22856f8b9c0`.
The independent raw-owner ACTION check passed24 poses/384 matrix components,
maxerror2.69950239e-7. Required cumulative host and owner pose/import gates
passed; source instruction/data verifier passed. Native code built through the
space-safe build script. No hardware result is implied.

Azahar2126.0 Old3DS/Vulkan original row9 fixture PASS32ticks,1 boost,24 component
submissions, simulated phase maskff, rendered maskc7. Several game ticks passed
between displayed frames, explaining the sampled render mask without changing
animation timing. The source-model parts were actually submitted; not all8
phases were independently captured as raster images. Boost tick3 speed10,
lockout/control69 and velocity(9.992,.055,-.386) matched the bounded source path.
Follow-on movement entered the existing inclined/wall solver; that solver and
complete panel-assisted traversal are not accepted as retail parity.

Generated packages are installed in external SD `assets/`; an earlier pair of
spring files copied one directory too high was moved to the loader's actual
asset directory after byte comparison, with private backups. Debug fixture
flags are removed after saving evidence. No owner package or binary enters Git.


## Act 1 chained-boost terrain diagnostic

The owner collision and movement packages reproduce a reset in the isolated
host replay when SET row 0 chains into row 12 on tick 6. Sonic becomes airborne
on tick 13. The resident floor-below query finds a surface under the first
descending sample (tick 52), then reports no floor below Sonic on tick 57.
The replay reaches its reset boundary on tick 137. Starting on row 12 alone
instead lands and survives the 240-tick window.

The floor probe is a vertical landtable query and uses its normal surface
filters. It does not certify the player's swept contact or include DynamicCOL,
native SET scheduling, or the original game's response. The evidence isolates
a chained approach worth matching against PC2004, and distinguishes a missing
floor hit from an out-of-region query. It does not establish a gameplay fix.

The optional SET WALL experiment now reports the source row and response
for each applied correction. In the row 0 fixture, the experiment applies
hard corrections from rows 191, 283 and 533 on the first tick, then soft
corrections from row 533. It leaves the pad chain before row 12 and resets
on tick 47. WALL response can materially change this approach. That experiment
uses SET order and lacks the original CCL scheduling and activation gates,
so its result does not justify enabling those corrections in the native game.

Separately, the static sweep reached the ramp segment in source collision
instance 426. At the loss-of-support tick the tested plane crossings either
fell outside the finite triangles or were on the plane the controller was
already leaving. Removing the sweep's descending-Y gate experimentally left
the failing trajectory unchanged. The gate remains in the runtime.

The chained replay also compares its first lost-ground tick against the
PC2004 per-polygon sphere sweep, evaluated from the port's pose and velocity
at the normal Sonic body radius. On the row 0 → 12 route, the sweep sees five
faces of the same ramp on tick 13, including faces with normal alignment
above 0.96. The imported ramp is present and the per-polygon sweep can hit
it. The host replay still resets. This comparison does not run the retail
candidate scheduler or apply the original response, which remain necessary
to reproduce the loop without changing collision tolerances blindly.

The source ground-side gate at 43CCE0/43D180 compares the new normal to
the actor's prior Up angle (through 438410). It accepts angles through
0x2C00 BAMS, or accepts a larger angle when the candidate normal points
more against gravity. The 43D360 response has an additional object flag
exception. Applied to the host replay's first detachment with ordinary
gravity, the gate accepts all five reported ramp candidates. This rules
out the source angle gate as the reason those particular candidates
would be discarded. Candidate ordering and native response remain open.

On the same replay, the port's endpoint support query returns no contact
at tick 13. Applying the 453820 per-object selection rule to the static
sweep hits selects face 426/9 (final plane height 3.0437). The port's
support center is 0.831 units below that face, beyond the point query's
0.35-unit penetration limit. A private host experiment increasing that
limit to 1.0 keeps support through tick 13 but loses it at tick 16, when
the next selected face is 1.657 units above the point. The tolerance change
was discarded. The source trace identifies a sweep/endpoint mismatch.
The next step is to reproduce the selected face's pushback and support
update in the collision step, including the step's object arbitration.

The ordinary static-support branch at 449EE5 advances the averaged
radius-corrected center by the selected face's tangent step, then
projects it onto the selected plane using 788D40. The body-offset
addition and subtraction cancel before the static task position write.
Applying that position calculation to the tick-13 sweep gives task
position (2204.667, 20.638, 256.112). After converting it to the port's
one-unit support center, the existing port query reports support on the
same face, 426/9. The replay reports this corrected query alongside the
unchanged gameplay trajectory. The calculation assumes the response
branch was entered and does not decide its entry, orientation update,
or write gate.

In a temporary host experiment that reapplied port ground state after
each successful projected query, the first two corrections recovered
support on faces 426/9 and 426/13. The player detached at tick 16 on
face 426/148. That face passes the source ground-side gate, but its
normal Y is 0.1649, below the port point probe's 0.45 cutoff. The
experiment never traverses the loop and uses port frame/velocity
updates, so it is not a candidate for enabling in native gameplay.

The port's wall pass used to re-query support even on frames when it
made no correction. In the temporary face-relative replay, that
fallback silently replaced the descending loop normal (Y = -0.813)
with a near-upright normal (Y = 0.994) on tick 27. The wall pass now
preserves steep support on correction-free frames. Upright support
still goes through the existing ledge check. With the same private
sweep experiment, the actor retains the loop face through tick 27.
Tick 28 still has a real wall correction followed by an incorrect
support handoff. The checked-in build uses the ordinary controller and
still detaches at tick 13. The experimental steep-face changes are
used only to investigate the remaining loop response.

The tick-28 wall correction in the private replay comes from edge 0
of face 416/101 at sweep fraction 0.9311. Its response normal aligns
0.9129 with the actor's Up. After this correction the projected
point on face 426/41 is within the support distance but outside the
finite triangle, including when expanded by the source sweep's four
percent. Suppressing the individual edge merely moves the collision
to tick 29. A real fix needs the source's cross-object response rather
than an exception for that edge. The wall pass now uses the same
ground-departure action and animation transition as the controller
when revalidation finds no support. Its height fallback also handles
dynamic platforms, which have no static triangle index.

The collision step runs two passes over the objects gathered by
450C10. The first pass uses the current travel length. With the
incoming low ground bits set, 43CCE0 can shorten that length for an
opposing non-ground-side face. With those bits clear, 43D180 can
shorten it for a ground-side face. Both use the distance between the
sweep center and a face's radius-corrected point. The last object that
shortens the first-pass displacement leads the response pass. The
rest are traversed in reverse input order for static support and
forward input order for a moving support task. The previous support
identity moves to the end unless it was promoted.

In the Act 1 row 0 → 12 replay, the five hits on object 426 leave the
grounded prepass length at 12.5125 on tick 13. This narrows
the first-detachment investigation to the support response rather
than a length clamp within that object. The host trace propagates length
clamps in resident object order and reports response order for the
objects it hit. It omits the source's object discovery, mutable
velocity side effects and ordered re-sweeps.

For ordinary supported Sonic at or above dash speed, 444640
re-sweeps the candidate with a radius of 5.1. It starts from the
task origin plus the task-frame probe offset, with the first
sweep's averaged radius correction relative to its starting center.
On the row 0 → 12 replay at tick 13, the reconstructed fast probe
records four hits and still selects 426/9. The final speed gate
accepts that face, and projection using the confirmation sweep
gives (2204.307, 20.425, 256.112). The probe takes its frame and
velocity from the port replay. It does not simulate the full
candidate response, special flags, or original runtime scheduling.

The initial sweep also invokes 43C7D0. For this fast grounded
fixture, the nonpositive local vertical speed lowers the center by
Sonic's 2-unit position error. The sphere radius stays 5. Rerunning
tick 13 with the corrected center height of 3 still selects 426/9
and leaves the prepass length unchanged. The confirmation sweep
now records five hits and projects to (2204.408, 20.485, 256.112).
These values supersede the confirmation values above, which used
an unadjusted center on the first sweep. The fast branch is scoped
to the replay's ordinary grounded frame and uses the port
displacement as its estimate of local velocity.

To apply that isolated response in the host replay, run
`host_dash_panel_path COLLISION_PAK MOVEMENT_PAK --chain --apply-static-support`.
On a lost-support frame it accepts a confirmed ground-side face
only when one object hit and the prepass left travel unchanged.
The confirmation position then feeds the port's usual ground-frame
update and wall pass. This mode is independent of the SET WALL
experiment and is off by default.

For rows 0 → 12, the replay applies the correction at ticks 13,
14, 16 and 17. At tick 18 the confirmation still finds the loop
but the port displacement produces +0.006071 local vertical
speed. This rejects the grounded position response. The replay
has not reconstructed the original player's local velocity at
that point, so using the displacement sign is a source of drift
to resolve before attempting to enable this mode in gameplay.
A private trace of the controller before position integration measures
+0.006035388 in its current local frame on the same tick. This confirms
the positive sign comes from the port's movement state at that point,
rather than from subtracting two rounded world positions.

The ordinary running controller now applies the 44C270 steep-surface
force branches. The primary branch checks the absolute world-Y component
of the cross product between collision-work Up and incoming motion.
When it fires, the local gravity contribution becomes (0, -weight, 0).
The other steep branches subtract their respective weight multiples
from the transformed normal component. Drag is added after that step.
The port maps the source inputs to its retained task frame and current
world velocity for this ordinary path.

The opt-in replay now accepts the confirmed face at tick 18 and keeps
support through tick 27. It detaches at tick 28 and later resets. The
ordinary replay still needs integration of the swept support response
to traverse the entrance.

The opt-in replay also applies the ordinary grounded velocity writeback
from PResetPosition (43EE70). It clears local normal speed and snaps
forward speed strictly inside ±0.01 and side speed inside or at
±(0.9 × run_accel), with the source work-flag bypass for the two
tangential tests. The side comparison retains the product precision.
The replay still loses support at tick 28, and this time the trace
shows zero per-polygon hits before the wall pass. That trajectory's
failure occurs earlier than the edge correction seen with the
previous experimental position updates.

A private trace of the tick-28 miss shows the sweep samples are within
five units of faces 426/39–44 but outside the expanded polygons. For
the last support face, 426/41, the first sample is almost on a side
edge and the later samples move farther past it. The height slab
therefore does not explain the loss of support on this trajectory.

The host replay's `--chain --apply-static-task-angles` variant keeps
canonical task yaw when applying confirmed terrain pitch and roll
instead of projecting the old forward vector. This is a sensitivity
check using the source rotation convention. It does not reproduce
449500's angle selection or 444F10's capped angle updates. This variant
reduces lateral drift enough that it no longer reaches the steep-force
cross-product threshold on tick 18. It detaches there. The prior
`--apply-static-support` replay is unchanged.

The task-angle replay now also exercises the ordinary supported
444F10 angle step. The code chooses immediate writeback for a
more upward face. Otherwise it computes the shortest turn from
the current frame to the face normal and applies the jog, run,
rush and dash speed limits to X and Z while retaining yaw.
This check uses canonical port angles and an already selected
target. The source's target history and special status paths
remain outside the replay.

With the angle step enabled, the row 0 → 12 replay gets a valid
confirmation at tick 16 but loses that support during the wall
pass on the same tick. The existing position-only experiment
remains byte-for-byte unchanged. The source tracks the accepted
polygon normal separately from the smoothed orientation. The
current port contact frame still conflates those inputs.

The wall pass now recognizes a previously selected steep static face
by its triangle index. It excludes that face from wall collisions and
uses its plane and finite extent when another wall correction requires
support revalidation. The body sweep keeps the smoothed task frame.
With this change, the task-angle replay retains face 426/148 at tick
16 and proceeds through tick 17. Tick 18 still fails the confirmation
speed gate on this alternate trajectory. The native movement step has
yet to adopt the experimental swept support response.

The running path now stores the confirmed support normal's vertical
component separately from the task rotation. This follows 43EE70:
the accepted face sets player-work Up, while 44C270 still uses the
task frame for transforming world forces and for its cross product.
Both the steep-force and the ground acceleration/speed thresholds
use the stored Up when available. A focused fixture covers a steep
contact with the actor still tilted up. In the task-angle replay,
this separation alone leaves the tick-18 loss unchanged.
