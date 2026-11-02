# PC2004 airborne attack reconstruction ledger

Private reference: owner's executable SHA256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
The ordinary targetless attack selector and public movement path are now
connected. Target acquisition/pursuit are still pending. This is not complete
controller or retail acceptance. The private executable and decompiled text
stay outside Git.

## Entry and selector

Sonic action8's selector496F50 first handles commands/exceptional contexts,
landing and action-button cancel492F50; then jump-press helper43BF40 may enter
action14 and call494B80. Helper43BF40 blocks status4000 and requires the pressed
jump mask. It returns1 plus whether the first-CCL context helper4B83F0 returned positive.
**No target still returns1**, so it does not prevent an ordinary air dash. An
earlier reading that treated target absence as attack ineligibility was wrong.
4B83F0 examines a first-hit task with ID8; it does not search the homing
candidate table. Homing candidates are collected separately at418BE0.
Action12 fall does not contain this attack-entry check; it cannot be enabled
there simply because the broad port state says airborne.

494B80 initializes signed-short timer+80=-1 and age+82=0 and requests sound2FA.
With empty homing candidate list and441BF0==-1, it writes local forward speed5
(or10 with the special player flag), sets timer1 and returns. Local normal and
side speed, current animation request and task frame remain retained. Otherwise
it calls492300; if the resulting timer is nonzero it writes forward5 afterward.
Effects/sound delivery and exceptional/special-player contexts need integration.

Action14 selector increments signed-short age after reading its prior value.
Prior age>359 or the first CCL other category0/2/3 sets timer=-1 and action8,
retaining animation/status. Otherwise timer>=6 or pressed action cancel sets
timer0, action12, request18 and clears status500. These tests precede its landing
branches. Landing invokes original braking/idle classification helpers; the
port's current shared static support adapter does not prove those branches.

## Movement and target tracking

492300's positive-timer/empty-list path with441BF0==-1 increments timer, scales
local forward speed by.9, then calls **44C270 PGetAcceleration and443F50
PGetSpeed**, not44B9C0 air acceleration. This movement runs on the same update
that the selector first enters action14. In the targetless ordinary path the
initial timer1 therefore becomes2 on that entry movement; selector timeout at6
must not be delayed until after another decay step. Held-jump acceleration is
an action8 selector operation, not an extra boost during action14.

Target acquisition compares source task yaw against candidate direction with
5000-BAMS inclusive bound and source squared-distance ordering. Candidate
positions come from CCL centers, respecting the source absolute-center flag.
Task rotation advances toward the selected yaw by at most800 BAMS (1000 for
the special player flag). An initially negative timer clamps upward target
displacement to0 and sets timer1; otherwise a successful initial track sets0.
Normalized vertical displacement and current task yaw construct world speed5
(or10 special), then transform that speed back into the complete player frame.
After age180, source random scaling affects pursuit speed. Candidate-list order,
partner-player target441BF0, source random state, full CCL arbitration and those
special contexts must be represented explicitly, not replaced with nearest
Euclidean target or instant facing.

## Shared turning correction (WCA27)

443C50 checks task ground status&3 before querying analog/Up/friction. Without
those ground bits it blends independent f32 coefficients .1 old-local and.9
transformed-local. Grounded no-input uses.05, grounded analog ordinary Up uses
.99, low Up uses.5 and friction multiplies the grounded general weight. Slow
443E60 has its own Up-dependent coefficients and no airborne status branch;
quick443DF0 uses transformed local speed directly.

The WCA23 air helper incorrectly passed ground status into the grounded-only
helper's analog argument, selecting.05/.95 in air. WCA27 adds an explicit
status-aware wrapper and corrects both flat and full-frame air callers. It
preserves the distinct retained-ground first spring movement. The pre-existing
flat air test reproduces the old bug; full-frame/retained-ground assertions now
cover both contexts. That existing air test was missing from the cumulative
current-host gate and is now included, without running the entire legacy
physics matrix. Exact-owner steering witnesses include the status branch and
both independent constants. No bit-exact x87 or matched retail trace is claimed.


## WCA28 ordinary targetless controller path

`pc2004_air_attack.[ch]` implements the empty-target selector/clock with explicit
signed16 wrapping. Player-work special flag8000 is separate from task status;
setting task8000 alone does not select speed10. The normal native bridge uses
speed5, no partner/candidate targets and no first-other CCL category. Pure
selector checks cover category/age priority, but native target CCL is not wired.
Commands still run before movement through the existing player command bridge.

The public flat dispatcher promotes attack entry/action14 to the retained
surface-frame path. It constructs flat axes from current yaw before promotion,
avoiding reset's stale axes. Grounded/charge/ordinary fall do not enter air dash
from a second jump. The full-frame path decays forward before shared44C270 /
443F50 acceleration/integration and uses the status-aware turn helper. Action8
cancel and14 timeout clear ball state and request18;14 entry preserves request
14 or15 and local normal/side speed. Held-jump boost is suppressed on the entry,
attack and timeout ticks. The original default-friction/zero-external-force
specialization and existing contact solver remain in use, with their documented
limits. Full low-Up force preamble, exceptional status/surface modes, exact
landing/braking, target pursuit/partner lookup, source sound2FA and attack
visual effects are not established by this integration.

Focused public checks confirm an inactive-frame yaw90 launch goes along the
correct world axis. Local entry5 is immediately decayed4.5; ordinary no-input
forward slowdown/drag gives4.5+4.5*(-.008)-.06=4.404 on that first tick, while
normal2 becomes1.9 with no held-jump addition. Five attack movement ticks end
with timer6; the following selector changes to fall12/request18 before movement.
The native default-off probe supplies jump/release/second-jump at the original
course start and also supplies angled input while attacking. It is an explicit
input sequence, not a retail execution trace or a full-course success test.


WCA28 validation: exact-owner instruction/dispatch witnesses PASS; focused
selector/public-controller checks PASS; required cumulative host gate PASS;
space-safe native build PASS. Code-only3DSX381608bytes, SHA256
`c772cbb8634e1cc9bc0dcf4f76c3198ee9df2ed06f7ab89b80bf7265200669e3`.
The exact build in Azahar2126.0 Old3DS/Vulkan passed the24tick input-only
original-start sequence: jump8/request14 tick1; attack14/timer2/age0/request14
/ball1/jumpTicks1 tick4; fall12/timer0/age5/request18/ball0/jumpTicks1 tick9.
Five attack movement ticks were observed. Angled input was supplied during
three attack ticks. The fall pose was visually inspected after the fixture.
Native source timing/selection is established for this bounded empty-target
sequence, not a matched original-PC trajectory, pursuit, landing or hardware.
Debug input flags were removed after saving private evidence; normal gameplay
uses the ordinary physical button adapter. No owner asset/import/pose data
changed, so the unchanged owner pose oracle was not rerun.
