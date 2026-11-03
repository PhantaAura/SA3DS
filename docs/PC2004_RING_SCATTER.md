# WCA41 — scattered-ring task equations

This is a source/host checkpoint, not native scattered-ring gameplay. WCA40's
native damage bridge still retains scatter requests without creating these
tasks. The runnable preview remains the WCA40 artifact from commit8b780d4.

`ring_scatter.[ch]` implements the five-way44FD10 task dispatcher, independently
of SET direct/group rings. Caller services supply exact owner trig/atan, source
floor query and Ninja Rz/Rx transform. Outputs request real collision, display,
ring awards, sound and particle art; this module does not fabricate executors.

The authority is the owner's same exact PC2004 executable SHA256
6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.
REA opened that exact target. Its full Ghidra analysis timed out at330seconds;
that unchanged expensive attempt must not be repeated. A targeted no-analysis
private Ghidra script recovered44FD10 successfully. LLVM disassembly resolved
ambiguous registers, stack slots and branch operands. The supplied private C
omits44FD10, but includes44F7A0/44F930/44F960 and49E920. All recovered code,
projects and executable data remain outside Git.

## Preserved behavior

- New tasks cycle shared spawn phase0..8, cache originY-100, request source
  collider916B10/category7, shift worldY by10 and initialize source float fields.
  Each initializer consumes a fresh RNG value for spin. This is additional to
  4506F0's shared burst phase. Caller task creation/list order remains essential.
- The floor probe runs on its phase countdown, querying worldY+6.88 and writing
  source physical X/Z. Missing source sentinel -1000000 falls back toY-100.
  The service must implement source49E920's highest eligible floor below hint,
  not an arbitrary nearest floor. Native floor adapter is not connected here.
- State1 uses radial source sine/cosine times f32(.3), stored parabola age and
  source f32(1/3), floor bounce scale decrement f32(.04), launch decrement
  f32(.28). A tilted upY below f32(.7) updates yaw from transformed direction.
  Collision registration starts only after47 state1 updates and uses temporary
  Y+3.44. Source task ID11 keeps these rings out of homing selection.
- Once radial scale is below f32(.2), every branch transitions to state2 in the
  same update. The2.4/13 comparisons choose drift, not a new elapsed phase.
  State2 preserves its prior age, registers a temporary shifted collider,
  requests normal draw on global frame bit1, selects its display callback and
  marks action3 at unsigned age100. Action3 deletes on the following update.
- State1 pickup requires cached primary-player ownership to award; state2's
  source branch omits that ownership test. Both suppress awards when the
  primary dead flag4000 is set. Hit flags clear and pickup effects initialize
  regardless of whether the ring is awarded. Their display callback changes
  differ and are preserved as outputs.
- Pickup fields advance by source f32(.8) while unpaused; particle frame
  truncation/visibility and delayed deletion honor render suppression.
  Caller owns shared pickup parameters, actual sprites and callback scheduling.
  Display routines can have source clock side effects; a future adapter must
  use the actual logic/display/pause schedule, not advance once per arbitrary
  GPU query or assume all callbacks are pure.

## Evidence and limits

Required current host gate PASS including new task tests. The ordinary flat
fixture reaches state2 at208 post-init updates; later bounce count18 excludes
bounces before its first47-update assertion. Cases cover initializer RNG/phase,
first-step parabola, grace boundary, full lifetime, temporary CCL positions,
frame-bit draw/counter99/100/delete boundaries, phase-specific attribution,
paused/suppressed pickup, low-scale branch intervals and a known90-degree tilt.
These are source equation checks, not a retail trajectory oracle.

The exact-owner static verifier PASS covers4 code spans, five dispatch targets,
all relevant memory constants and grace/frame/counter instruction operands.
No asset importer/pose change occurred, so the unchanged owner pose matrix was
not rerun. No new native build, emulator check, hardware result or scattered-ring
visual/collection claim belongs to this source-only checkpoint.

Next: external owner CCI/category7 rules; native creation/reverse task ordering,
shared RNG/spawn-phase lifetime, floor/tilt bridge, CCL registration and real
ring/particle rendering; then a bounded native damage/scatter/collection check.
Full stage death/respawn/audio/controller/collision and course acceptance stay
open. PHYSICS_1TO1 remains false.

Private continuation aids (never stage these):
`/private/tmp/sa3ds-wca41-scatter-private.c`,
`/private/tmp/sa3ds-wca41-scatter-full.asm`,
`/private/tmp/sa3ds-wca41-scatter-helper.asm`,
`/private/tmp/SA3DSScatterOnly.java`,
`/private/tmp/sa3ds-scatter-only-project/scatter_only.gpr`.
The targeted project can be processed without reimport/whole-game analysis;
its private output is not redistributable source. The launch uses existing
Semeru21 JAVA_HOME and JDK_JAVA_OPTIONS with a task-local Java user.home because
the global Ghidra java_home.save path is sandbox protected.
