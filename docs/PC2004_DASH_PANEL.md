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
