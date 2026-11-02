# PC2004 dash panel reconstruction

Reference is the owner's exact executable SHA256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
Private executable/decompiled text and converted models remain outside Git.
This checkpoint adds a portable task/direct-boost module, not a native panel
spawner or a complete Emerald Coast gameplay claim.

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
per display update, wrapping at8, with the source pause gates. The animated
model is not yet converted or connected to native rendering. Native panel
activation, shared category4 queue ordering and hit/boost observation remain
pending. The module must not be advertised as native panel acceptance.

Focused checks cover source defaults, ceil/low16 distinctions, first-frame
behavior, grounded/per-player gating, expiry order, full-frame direct speed,
retained action/animation/task position and capsule side/end boundaries.
`tools/verify_pc2004_dash_panel.py OWNER_SONIC_EXE` checks exact owner identity,
inspected function digests, calls, branch operands and collision/action records.
These checks do not establish a retail runtime trace, course completion or
hardware success.
