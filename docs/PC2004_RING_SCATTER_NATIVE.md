# WCA42 — native damage, scattered rings and collection

WCA42 connects WCA41's source task equations to native task allocation, source
CCL reports, shared owner ring geometry/textures and the signed16 ring counter.
The bounded original-enemy fixture loses seven rings and collects one back.
This does not complete the damage loop or full-stage object/physics parity.

## Owner data and integration

`tools/build_pc2004_ring_scatter.py OWNER_EXE EXTERNAL_OUTPUT` requires the exact
owner EXE, verifies source witnesses and writes a private SA3DSPAK outside the
source tree. Install it as `sdmc:/3ds/SA3DS/assets/ring_scatter.sa3dspak` alongside
the existing external owner packages. It carries source916B10 CCI and category7
rules, not gameplay code. This owner's generated432-byte package SHA256 is
913ba35beca18c7a98dea5a902a58feb73865a260338be365763e947e36d4df6.
It is installed in the private and shared local Azahar SD trees, never Git.
The parser borrows the validated CCI payload, limits size/chunks, rejects
reserved/version errors and inherits SA3DSPAK bounds/CRC validation. Missing
resources retain damage requests and emit diagnostics; no replacement collider
is guessed. The converter refuses output under its source tree.

Damage preamble4506F0 requests allocate individual list2 tasks in reverse
creation order. Failed allocations advance the burst index without inventing
an actor. The source initializer consumes another shared RNG value per allocated
task. The native task bridge consumes previous CCL flags/cache, runs before the
existing SET object phase, emits the following pass's source spheres and applies
source ring awards. Category7 resolves in shared cached-contact queries; task
ID11 rings are excluded from the homing list. Only supported player/ring sphere
pairs are connected; full source inter-object CCL/list scheduling remains open.

The existing highest-static-floor query now optionally returns the same selected
triangle's normal. The ring adapter derives canonical X/Z and uses owner's trig
for its Ninja Rz/Rx transform. Imported polygon edge arithmetic, source angle
rounding, dynamic land objects and exceptional gravity remain unverified. Tasks
outside the guaranteed resident radius are explicitly deferred, not mistaken
for source no-floor. Whole-course residency for long-lived scattered rings is
still incomplete. Spawn-phase lifetime persists separately from clearing tasks;
sharing03B42FE8 with all ordinary SET/group ring tasks remains open. Whole-game
RNG call ordering also remains incomplete despite using one modeled native RNG.

Normal scattered-ring draws reuse the owner's original ring model/materials and
CPU-uploaded textures. They use source worldY+3.44 and float spin interpreted as
degrees by44F960. Preserve the initializer quirk: it first stores a converted
random BAMS integer in that degree field. No substitute mesh was generated.
Source logic advances clocks; native rendering reads the cached draw request.
Full pause/display callback scheduling and retail raster comparison remain open.
Source shadow, pickup particle art, rumble/shader effects and audio are pending;
particle requests are counted explicitly. Zero-ring source death/respawn remains
quarantined by WCA40 pending its real executor.

## Checks actually run

- Required current host gate PASS, including unchanged hurt/homing/controller
  checks, scattered-ring lifecycle cases and the new synthetic asset reader
  valid/reserved-header/size-cap cases. Only whitespace changed in the reader
  afterwards to remove new GCC indentation warnings; no redundant gate rerun.
- Required exact-owner pose/import gate PASS for the new owner collision
  conversion, including raw pose/transition/animated-world checks and both
  damage/scatter static owner verifiers. Owner Sonic assets remain unchanged.
- Staged native build PASS with the original CPU uploader/SD pair/diagnostics.
  Final433652-byte artifact SHA256:
  1892f46ae1cd52f16d6d4594d25475373db44329541c07e796b1d390d9516ef6.
  Existing unrelated uploader/placement-reader warnings remain; the new
  indentation warnings were removed before building this exact artifact.
- This exact artifact ran Azahar2126.0 Old3DS/Vulkan, original act0 Saru row28,
  private R fixture. It seeds7rings, starts in the supported-ground facing from
  WCA40, then uses ordinary analog steering toward a live scattered ring from
  tick80. It stops on the first actual task award, or fails at180ticks.
  No post-start teleport, forced collision, direct award or physics tuning.
- At tick129:1hurt entry,76hurt ticks (37air/39ground),44suppressed rendered body
  frames,7scatter requests,1ring recovered,timer0,action2,grounded,wall0.
  Actual tasks:7spawned,896updates,566CCL queue events,508draw submissions,
  1source award,0allocation failures,0deferred queries. Native expiry/deletion
  and particle art were not checked in this short interaction fixture; their
  task boundary equations have host coverage. Ordinary pickup particles had
  not advanced because the fixture paused on the award update.
- Screenshot inspected: Sonic and a scattered ring render on the source terrain,
  HUD counter1. This confirms visible native content, not retail raster fidelity.
-300-frame diagnostics include the paused tail:129ticks/6937ms,submit6168ms,
  max433ms,minLinear26569600,maxCmd0.254395. Not steady gameplay FPS or physical
  Old3DS performance. Temporary private fixture/autoboot flags removed afterward.

Next: original player death command/respawn/checkpoints and pickup/shadow/audio
executors, plus remaining bombs/enemies/monitors, whole-course collision/camera,
whale/events/HUD/renderer/hardware gates. Full WCA and PHYSICS_1TO1 remain open.
