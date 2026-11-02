# PC2004 homing target collection and pursuit

Reference: owner's exact executable SHA256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
The private game and decompiled text are not project source. This checkpoint
implements the portable collector and492300 pursuit/decay decisions. It does
not connect native enemy actors or claim native homing acceptance.

`pc2004_homing.[ch]` takes pointer-free actor identities, a previous completed
CCL pass's candidate list, current actor/first-collider poses, raw source task
yaw, optional partner identity and source math callbacks. It returns either
local-speed decay plus acceleration, acceleration without decay, or an updated
task yaw and direct world velocity. The caller owns source initialization,
selector ordering, complete task frame, world/local conversion, collision,
position integration, sound/effects and target lifetime.

## Candidate collection

418D00 computes squared distance between task positions through418B30, stores
that value as float32, and calls418BE0 independently of physical body overlap.
Only actual player0/1 with category0 can collect a target with outer CCL flag40
and distance strictly below10000. The regular list holds at most290hex (656)
entries, in collision-pair order, without sorting or deduplication. A target
with task ID11 goes into a separate source list (capacity256), not this homing
list. Ordinary rings explicitly assign ID11 at450435. The module excludes them;
it does not turn rings into enemy targets or implement Light Dash.

Stored distances are from task positions in the prior CCL pass. During pursuit,
492300 reads each current target's first collider center. If its shape flag20
is clear it adds current task position; otherwise the center is already world
absolute. It does not rotate that offset or recompute center distance for
ranking. A closer model center cannot override a smaller recorded task-distance
value. Strictly-smaller replacement preserves the first entry on equal ties.

4B83F0, used by43BF40's jump-press query, examines a separate first-CCL task-ID8
context. It does not search this homing table. The earlier ledger's generic
"target helper" description was misleading; no target is still eligible for
the ordinary action8 second-jump attack.

## Pursuit decisions and math

With no partner, a positive timer OR an empty candidate list increments signed
low16 timer, multiplies only local forward speed by.9 and requests44C270 then
443F50. Positive timer locks out newly appearing candidates. If tracking search
finds no eligible target, timer becomes1 and the helpers run WITHOUT that step's
forward decay. These are separate outcomes, not one generic air-dash update.

A valid partner is seeded from current task distance/bearing with strict10000
range and inclusive5000-BAMS facing bound, even for the special player. Regular
candidates use that same facing bound unless player-work flag8000 bypasses it.
A candidate must have strictly smaller cached distance to replace the partner.
The4383B0 absolute angle result is unsigned and includes8000; it must not wrap
to a negative short. Approach438350 uses signed-short direction, with negative
source task-Y movement at the exact half-turn, capped800 BAMS (1000 special).

After choosing a target, an initially negative timer and positive target deltaY
clamp deltaY to0 and set timer1. Other successful initial tracking sets timer0.
Selection happens before this upward rule; above targets are not filtered out
in favor of a farther below target. The normalized vertical component and
limited current yaw form speed5 (10 special) world velocity. Tracking does not
apply ordinary gravity/acceleration. Age>180 consumes one source random15 value
and uses the original .7+.1*random/32768 speed factor. The module requires that
RNG callback instead of inventing an unrelated random sequence.

Bearing uses the original conversion after atan2: multiply65536 then the exact
source inverse-tau double **.15915497620314795**, and truncate. That differs
from mathematical1/(2*pi). Callers must supply the original bearing/trig policy;
tests use that conversion with analytic trig, not a claim about x87/Ninja-table
bit precision. The native owner quarter-wave table can supply source trig when
real actor integration is ready. Raw task yaw must be provided; projected
heading from an arbitrary inclined frame is not proven equivalent.

## Required native integration

Initialize494B80 with timer=-1/age0. The truly empty-list/no-partner case sets
forward5 (10 special) and timer1 directly. Otherwise initialization calls492300;
a nonzero resulting timer then overwrites local forward with5. The subsequent
action14 movement invokes492300 again in that SAME player update. Connecting
pursuit only on the next tick, or attaching targets after the empty-target
selector has already set timer1, would change the original behavior.

Real actor/CCL ownership and prior-pass ordering must be connected before
advertising native pursuit. The current stage has no native enemy/monitor
actors, and its rings are excluded from the regular table. The existing native
air-dash bridge therefore remains on the explicit empty-target path. Remaining
work includes real target lifecycle/hits, full frame/landing behavior, partner
lookup, shared source RNG, effects/audio and original-PC trajectory comparison.

Owner registry inspection locates the next actors: E SARU kind29/task4AD140
(9 act0/5 act1 placements), E SAITO kind30/task7A1380 (2 act0), E AMEMB kind31/
task7AA960 (2 act1), item boxes13 and82, save points19. The private inventory is
not a converted asset package or an implemented enemy. Original actor behavior
must accompany their models; no placeholder targets were added.

## WCA29 validation

Focused checks cover collection range/cap/flags/ring exclusion, collision-order
ties, cached task-distance/current-center distinctions, absolute centers,
facing bounds/half-turn/turn rate, upward entry, partner priority and injected
RNG. Exact-owner code/data witnesses verify collector/pursuit/helper ranges,
ring ID, source inverse tau and RNG profile. Required cumulative host gate and
native build passed. The unreferenced module is removed by linking; the native
3DSX remains exactly381608bytes/SHA256
`c772cbb8634e1cc9bc0dcf4f76c3198ee9df2ed06f7ab89b80bf7265200669e3`.
No duplicate emulator run or unchanged pose oracle was needed. No native target
pursuit, retail trace, full-course or hardware success is claimed.
