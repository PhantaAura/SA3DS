# PC2004 CCL contacts and Sonic strength fields

Scope: exact owner executable6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.
Native actor/task scheduling is still pending. This module implements the
source sphere path and contact side effects using caller-supplied owner CCI
records/category rules and already evaluated world centers. No owner tables,
collider records or converted assets are compiled into the project.

## Sonic's pre-movement update

49AA48..CB sets primary attack/defense from distinct player-work flags4/6,
current action and task status, preserving the high nibble of the damage byte:

| Condition, in source priority | Attack | Defense |
|---|---|---|
|Either player-work word8000 |3|3|
|Action44 |2|3|
|Action6 |1|3|
|Task status100 |1|1|
|Action45 |0|1|
|Otherwise |0|0|

49AACE..ED enables the secondary record only for action72, clearing attribute10;
other actions set that disabled bit. This runs after source action selection
but before movement. Native integration must preserve that phase: recomputing
from post-landing/post-movement state could change the current CCL pass.
Source4915F0's later center update remains separate. Sphere centers supplied to
this module must be its actual primary center, not the render/body anchor.

## Contact pipeline

418D00 tests task-position squared distance against summed broad radii. That
float distance is also available to the independent homing collector even when
broad overlap fails.41FEC0 filters category exclusion masks and disabled shape
attributes.41FD00 dispatches primary-versus-other0/7 to41AA00's sphere path;
secondary generic dispatch supports sphere0/sphere0 here. Other shape families
remain unsupported by this module, not silently approximated as spheres.

41AA00 stores squared center distance as float and compares strictly against
the retained radius sum squared.418E70 then reports each direction independently:
self attribute8 or other attribute4 suppresses that direction's list/cache write.
Duplicates and pair order are preserved. Lists saturate at16; even when full,
the last-contact self/other index and other CCL identity cache still update.
The cache is independent of whichever hit a later category query returns.

418DE0 compares incoming low2 attack bits against victim bits2..3 defense,
checks victim attribute400 and the victim category's damage-receive mask.
Success sets victim task status4, CCL outer flag20 and victim shape damage-bit10.
This produces the inputs consumed by the shared enemy damage helper. It does
not itself execute death, score or player injury logic.

The sphere response writes the first CCL's horizontal direction, using original
zero-horizontal fallback(0,0,.1), source normalization and float depth stores.
418D80's category receive/inhibit masks and both-4000/already-damaged condition
gate task displacement. Source X product retention differs from stored Z;
the port preserves those float boundaries using double intermediates. Relative
centers must be re-evaluated by callers before later pairs after task movement.
The actual Sonic/Saru records inhibit displacement in both directions, confirmed
with original category/push masks; contact/damage effects still occur.

418B60 clear preserves CCL upper flags, task upper/ground bits and opaque hit-flag
high bits, clears low collision flags/count/cache, resets all16 sentinels and
clears shape damage-bit10. Clear belongs after prior results are consumed.
The shared hit-query cursor has its own original lifecycle/reset boundary.

## Evidence and remaining work

Focused host checks cover strengths/priority, secondary enable, broad and strict
sphere boundaries, category/disabled filters, directional hit reporting, ordered
saturation/cache, victim damage, contact normal/push gates, clear semantics and
actual contact-to-shared-enemy-score/bounce decisions. Exact-owner verifier checks
nine helper identities and four cases using original91CBF0/96C9E8/91CC20 records
and category masks: ordinary unprotected, ball, Spin Dash and special player
flags. This uses controlled evaluated centers, not retail gameplay execution.
No x87 retained80 boundary parity is claimed.

Required cumulative host gate and staged native build passed. This unreferenced
module leaves381608B/ae538ee9 native artifact unchanged. No redundant emulator
or unchanged pose/import check was run; no new FPS/memory/hardware claim.

Next: owner collision-profile packaging/loading and actual native Saru actor
activation/list3/render/CCL flow, source pre-movement strength capture and actual
player/score/death/projectile execution. Homing entry needs its original two
same-tick helper calls and correct raw task-yaw bridge. Complete course/retail
physics/renderer/camera and other WCA goals remain open.
