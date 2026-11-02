# PC2004 spring task, collision and player command bridge

This records static reconstruction against owner executable SHA256
`6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.
The private executable/decompilation and generated owner packages stay outside
Git. The equations/constants below are gameplay reconstruction, not imported art
or copied decompiled routine bodies. No retail trace or native spring gameplay
result is claimed by this document.

## New portable module

`src/game/spring_runtime.[ch]` implements the spring actor task phases, ordinary
Sonic command3/4 selection, action9/10 movement selection, signed control timer,
and original primary-collider narrow-phase hit predicates. It does not spawn,
load, draw, apply a launch to the port's controller, or invoke world collision.
Those are explicit caller integration steps, not implied by passing unit tests.

`sa3ds_springb_physical_frame` reconstructs the full SpringB task frame from
supplied source trig, postmultiplying Rz(SETZ)*Rx(SETX)*Ry(SETY-4000).
Port forward is source localX, Up is localY and port right is negative localZ.
At SETY4000 the source taskY is zero, so the Y rotation is skipped rather than
introducing a tiny analytic cosine residue. Class1 preserves its existing
player frame. Tests cover all64 combinations of quarterturns plus nonzeroX/Z
and confirm that the launch world velocity is the physical Up column*strength.

SpringB initializes its cooldown to10 at7A4EB9, while SpringA starts at0.
Thus B may enqueue on its first active tick after initialization; A waits11.
The initial draft incorrectly started both at0; source inspection corrected
that before native object integration, with a separate instruction witness.

The parent caller owns object loading, placement identity, collision queues,
first-hit order, the original task/controller-position bridge, physical task
frames, source parameter provenance and exceptional action/death/damage gates.

## Object tick ordering

Source task class1 is `0x7A4C30`, class2 SpringB `0x7A4E50`; both are SET task
list2. Their CCL category is4 (category queue limit256 at41C280). Collision
results consumed by the active task come from the preceding CCL pass, not a new
same-tick geometric query. Preserve that phase and list ordering in integration.

- Action0 initializes the CCL record, enters action1 and returns without draw or
  collision enqueue. The initially zero work-byte cooldown stays zero.
- Action1 draws. When the source pause gate is inactive, increment cooldown;
  only values above10 inspect a prior hit or enqueue CCL. With a matching
  player0/1 hit, remember that player and enter action2 without enqueue.
  Otherwise enqueue. Clamp the cooldown to50 after these decisions.
- Action2 runs on the following object update. Class1 clears cooldown to0;
  SpringB assigns5. The shared7A46C0 launch preparation sets action1 and display
  phase.1. The source action2 path does not run through action1's pause gate.
  Source launch sound is9; audio delivery remains caller-owned.
- After a launch, class1 resumes enqueue after11 active ticks and SpringB after6.
  Activation/culling is not guessed by this module; `skip_update` means the
  caller already made the original scheduling/loading decision.

The existing `spring_launch` helper rejects an unsupported original off-ground
getter branch:441990 leaves its scratch unwritten when ground status&3 is zero.
`spring_actor_tick` therefore returns failure transactionally in that case;
this is a limitation, not an invented airborne spring trajectory.

## Command and status semantics

Shared launch7A46C0 calls441370 for class1 or4413E0 for SpringB. Both set pending
command status0x1000 before the next player command-consumption phase. Class1
writes world velocity and retains task angles; SpringB writes local(0,strength,0),
angles(SETX,0x4000-SETY,SETZ), world-transforms that velocity, and stores the low
signed16 control timer. Strength is SET parameterY+5.

The ordinary consumer495FA0 uses a command-minus-one dispatch table4967B4:
command3 ->495FFF ->action9/request16, status &=~0x2500;
command4 ->496046 ->action10/request16, then4965B4 conditionally clears0x500
only if0x100 was set, and clears0x2000. **Neither clears low ground bits&3.**
Do not force the first launched movement tick to airborne before source contact
resolution. The pending command flag is cleared later by446F10, not by this
consumer. Exceptional external contexts tested before the switch remain outside
the portable ordinary consumer.

Launch positions from `spring_launch_prepare` are source task/foot origins.
The current controller stores a support center, so its existing one-unit Up
bridge must be applied when writing `SA1PlayerState.pos`. Do not treat the launch
position as the model center or add an arbitrary foot lift.

## Spring actions and movement

Sonic_Act1 at496F50 indexes its state table49A850 with **action-1**; main movement
at49AB29 indexes49BFBC directly by action. These distinct conventions matter.

Actions9/10 first handle ordinary command/damage gates. Then world motion Y<=0
selects action12/request17. Action9's exceptional request39 maps40 and41 maps42;
ordinary spring request16 maps17. Local Speed.y does not decide this apex test.

Movement dispatch:

- Action9 at49AF71 calls44B9C0, then PGetSpeed443F50, PSetPosition44CDF0 and
  PResetPosition43EE70. Do not invoke jump-hold acceleration for spring rise.
- Action10 at49AF7E compares local Speed.y with player parameter+0xB8,
  **nocon_speed=3 for default Sonic**, not jmp_addit(+0xD4). At equality or above
  use4432E0 (no-input acceleration); below use44B9C0. The action still remains10
  until the separate world-Y apex test selects action12.
- The newly selected action12 has its ordinary fall dispatch, including its
  orientation/special-air checks; the caller must not infer all of that from a
  bare AIR_CONTROL enum.

`spring_uncontrolled_add_velocity` reconstructs4432E0 with an explicit
zero-pending-player-impulse assumption. Input force must already be transformed
from Gravity*weight + source world force into the current task frame. It adds
forward `air_resist` (+F0, -.008 for default Sonic), normal `air_resist_y`, and
side `air_resist_z`. It implements the inherited grounded tick's zero-speed
lim_frict deadbands, nonzero-axis ground friction and no-cross guards. It returns
AddVelocity only; PGetSpeed and position/contact ordering are separate.
Nonzero pending impulse buffers and status4000 delegation are not synthesized.

GetAnalog43BE9C checks the signed16 control timer for **nonzero** before the
update tail. `spring_filter_analog` clears only logical movement axes, preserving
button requests and raw-device provenance. Tail446F48 clamps nonpositive timers
zero; positive timers decrement once. Thus timer1 blocks the current update and
then becomes0; a negative stored timer also blocks that update before clearing.

## Primary CCL collision predicates

Sonic init49ABC3 passes records91CBF0/91CC20 to41CAF0. The first unchanged record
is form7, radius4, half-height4, foot radius.65, attributes0x420. Its center is
absolute because attribute0x20 is set. Each player tail4915F0 writes
`taskOrigin + physicalRotation*(0,center_height,0)` there; default center_height
is5.4. Do not substitute the port's support center or a renderer bounds sphere.
The secondary attack record has attributes0x382008 and is rejected against
spring category4 by category mask90E3D4[4]=0x100000.

The list0/list4 primary path41FEC0 ->41FD00 dispatches by the *spring* form:

- SpringB record38CD470 is form0, center0 and radius14. It calls41AA00 even
  though Sonic's primary form is7. Record a hit for strict 3D squared distance
  between primary center and spring task position <(primaryRadius+14)^2.
  Coordinate products accumulate before one float32 distance store; radius sum
  remains retained for comparison. The portable module uses double retained
  intermediates, not a complete original x8780 precision model.
- Class1 record38CD440 is form1, center0, radius6 and half-height10. It calls
  special form7/cylinder41CCD0, not the generic sphere/cylinder routine.
  In default gravity-axis3, helper41C5D0 constructs the player body center from
  taskOrigin+(0,playerHalfHeight,0), independent of the absolute sphere center.
  The source special top inset and ordinary body bands are preserved by
  `spring_primary_overlap`, including strict contact boundaries. Spring SET
  angle rotations cannot move its zero CCL center; the form1 extents remain
  world-Y aligned in this path. Other gravity axes return unsupported.

For these original records the push masks prevent displacement in both
response directions; the hit record and CCL side flags are produced. The module
returns the hit predicate only. The default player's broadphase radius was
computed from both original records and exceeds the ordinary primary narrow
reach; queue/category/disabled-record filtering still belongs to the caller.
`spring_query_player_hit` translates the separate41C6E0/41CBC0 hit-list
query. It scans at most16 source slots, in record order, stops on signed
other-shape byte==-1, and returns the first nonnull task with nonnull CCL and
short category0. It does not skip an unsupported player index to find a later
player0/1. Query cursor41B9F0 is shared: changing query type or task pointer
restarts at0; otherwise it resumes after the last match, capped at16. The
portable query takes that start slot explicitly; integration owns this phase.

Changes to primary dimensions, category flags or other gameplay state require
actual source collider inputs, not assumptions hidden in the helper.

## Validation and limits

`tests/run_spring_runtime.sh` PASS covers init/delay/enqueue/launch/cooldown,
paused/skipped ticks, transactional off-ground rejection, command status masks,
low-ground preservation, signed control timers, distinct world/local apex and
nocon_speed comparisons, source no-input acceleration examples, sphere entry
boundaries, class1 top/body/underbody boundaries and unsupported gravity axes.
`tests/test_pc2004_spring_runtime_evidence.py OWNER_SONIC_EXE` PASS verifies the
exact profile, inspected range digests, instruction/call witnesses,
state/command dispatch indices, CCL forms/dimensions and category masks.

No native spring integration, visual assets, independent retail collision trace,
x87 bit-exactness, dynamic-object impulse interaction, exceptional gravity axes,
complete player action system, full-course progression or hardware verification
is established by these tests. Keep WCA and PHYSICS_1TO1 incomplete.
