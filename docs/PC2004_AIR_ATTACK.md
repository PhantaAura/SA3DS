# PC2004 airborne attack reconstruction ledger

Private reference: owner's executable SHA256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
This records source behavior for implementation. Target acquisition, attack
states and native air dash/homing are not yet connected; it is not an acceptance
claim. The private executable and decompiled text stay outside Git.

## Entry and selector

Sonic action8's selector496F50 first handles commands/exceptional contexts,
landing and action-button cancel492F50; then jump-press helper43BF40 may enter
action14 and call494B80. Helper43BF40 blocks status4000 and requires the pressed
jump mask. It returns1 plus whether target helper4B83F0 found a positive result.
**No target still returns1**, so it does not prevent an ordinary air dash. An
earlier reading that treated target absence as attack ineligibility was wrong.
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
