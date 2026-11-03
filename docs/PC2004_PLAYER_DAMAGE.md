# WCA40 — ordinary player hurt and recovery

This checkpoint connects previous-pass player/NPC CCL damage to Sonic's
ordinary PC2004 hurt preamble, movement, animation and display suppression.
It does not complete damage gameplay or the full controller.

## Source and implementation

Private authority is the owner's exact PC2004 sonic.exe, SHA256
6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74,
and private sonic.exe.c. Neither is project source. The static verifier retains
only hashes, addresses and small instruction witnesses, with no game payload.

- 496F50 tests the special invincibility flag, otherwise decrements an existing
  hurt timer and clears damage status. A nonzero incoming timer suppresses the
  damage query even when its decrement reaches zero. Accepted damage precedes
  queued spring commands and clears that pending command status.
- 492D70 sets timer120 and ordinary action16/request23. Source-local forward
  becomes negative run_speed; normal becomes jmp_y_spd; side is retained.
  Other signed action branches, shields and special effects are represented
  by the pure helper, but the native bridge supports ordinary Sonic only.
- Ordinary action16 dispatch uses PGetBreak/PGetSpeed and the existing position
  adapter. Grounded relaxation approaches physical X/Z toward gravity by
  0x800 BAMS. The canonical frame reconstruction inherits the existing
  principal-angle/pole/history limitations; it is not full retail rotation.
- Dense23 maps to original ACTION116, root72,22frames, property5 (clamp21).
  Dense24 maps to ACTION117, root72,38frames, property4 (NextAnim0). The player
  selector reads the previous actual animation request. It can leave hurt
  before timer expiry after the landing animation finishes. Pose selection
  stays read-only; native sampling does not loop property5.
- Source display4949C7 skips Sonic when hurtTimer bit1 is set. A skipped native
  body draw resets its GPU arena/statistics and still draws deferred water.
- 4506F0 consumes RNG even with zero rings or a secondary player. For primary
  positive rings it resets the entire signed16 counter and requests at most20
  tasks. Each task angle uses a separately computed integer (i*360)/count,
  then the float random phase and double BAMS conversion. Ordered hurt and
  ring-loss/death sounds remain separate requests.

Native calls the helper before command selection/movement and retains the
latest effect request with diagnostics. Ring scatter44FD10, sound, shader,
rumble, normal death/respawn and special-action effect executors remain OPEN.
Zero-ring source death command50 is retained and pauses this incomplete runtime
until reset; resuming cannot silently bypass the pending executor. This is a
quarantine for unfinished behavior, not an implemented death sequence.

## Checks actually run

- Required current host gate PASS. New focused cases cover timer boundaries,
  signed action/ring branches, shields, ordered effects, integer burst angles,
  hurt selector thresholds and timer-bit display gating. Public physics plus
  animation check:100ticks,75hurt frames (36air/39ground),clamp23 and landing24,
  then idle with21 immunity ticks remaining. Physics remains unchanged by pose
  selection. Existing homing/controller regressions pass in the same gate.
- Required exact-owner pose/import gate PASS, including source ACTION116/117,
  raw transition oracle and animated world oracle. Independent damage verifier
  PASS:7focused code spans,6calls, timer/recoil/blink/integer division and
  float/double constants. These are static/source checks, not retail execution.
- Staged code-only native build PASS. Final artifact426800bytes, SHA256
  5c75fb18894b8d6b3e2f858d1548fbe60df45a8d27d45de1d3a67fea9bb02b59.
  Existing uploader/placement-reader compiler warnings remain.
- That exact artifact ran Azahar2126.0 Old3DS/Vulkan, original act0 Saru row28
  with controlled7-ring inventory and no movement/button input. Contact tick1
  requests action16/timer120, removes7rings and requests7scatter tasks/2sounds.
  At100ticks:1entry,76hurt ticks (37air/39ground),33suppressed rendered frames,
  rings0,timer22,action1,grounded,wall0. Difference from the host hurt-tick count
  reflects original terrain/contact phase. No scatter/audio/death success claim.
- Initial426784-byte b0218dad candidate fixture faced the opposite direction:
  recoil reached a cliff wall, producing68wall contacts,99air hurt ticks and no
  recovery by100ticks: FAIL. Final fixture changes only its starting facing so
  backward recoil stays on supported ground. The cliff adapter limitation is
  still open; physics was not tuned to force this fixture to pass. A persistent
  pending-executor pause latch was also added before the final build.
- Final300-frame diagnostics include the paused tail:100ticks/6865ms,
  submit6178ms,max432ms,minLinear26569600,maxCmd0.239868. These are not steady
  gameplay FPS or physical Old3DS performance. Native raster hurt-pose fidelity
  has not been compared frame-by-frame to retail.

Owner packages remain external and unchanged. Temporary autoboot/probe flags
are removed after verification. Full player collision, cliff/loop traversal,
retail trajectories, power-ups, whole global RNG/task ordering and physical
Old3DS acceptance remain open. PHYSICS_1TO1 remains false.
