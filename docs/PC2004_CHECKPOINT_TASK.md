# WCA47 — original checkpoint task and owner assets

This adds the portable original post task and an external owner-data pipeline.
The native runtime does not yet instantiate them. WCA45 remains the runnable
baseline; CP5/7 checkpoint acceptance and the full WCA remain incomplete.

## Source task and ordering

Authority remains the exact owner PC2004 EXE/private C. WCA46's bounded targeted
disassembly supplies the original task and its helpers. The static verifier now
checks20 owner code spans, original clock/ring-reset calls, registry19,
five dispatcher targets, clipping calls and exact angular constants.

The parent44F540 checks46C360 clipping before its action dispatcher. State0
allocates/initializes the post work, two dynamic-land children and source CCI,
registers display/delete callbacks, positions the children using916A50 offsets,
and sets the SET preservation flag10 when a SET entry is present. Initial
angles are4000/-4000. Previously activated SET flag4 instead starts state3 with
postX0 and postY-4000 (hex). The portable initialize
request leaves allocation/children/CCL/preservation work to its native caller.

State1 displays the inactive art FIRST, synchronizes children and consumes
previous CCL flag1 plus cached player0 identity. It then selects state2, captures
actual player state, sets SET flag4, requests sound10 and initializes the
300-count checkpoint-time display and speed-derived post spin. It does not
switch the already submitted display to active art during that same update.
States2/3 use active post textures. State2 additionally advances signed16
angular accumulation and damping/spring force while player0 exists. Both
states advance the time-display blink; state4 only displays. The helper exposes
requests and snapshots without executing arbitrary native services.

44F150 damps the post's signed16 Y angular velocity by source0.995f. Integer
ratios preserve the source float constants exactly across host/ARM:8346665 /
8388608 and -13421773 /1099511627776. The spring-force branch retains strict
3640 cutoff,8192 reflection threshold,18386 target and low-word/truncation
semantics. Initial signed-speed conversion retains10*65536*(double1/360),
without a fabricated minimum impulse. Computing the player's signed speed from
its local vector and relative yaw remains a caller service, not implemented by
that conversion. The first three moving updates emit the original38E effect
request; the effect executor remains separate. State3 begins when both angular
velocities become zero. The300-count time display suppresses odd tenth phases
1/3/5/7/9/11. No particle/audio/HUD raster is claimed here.

The SET loader46BA40 stores loadRadiusSquared+100 for unloading. The owner
checkpoint registry supplies360000, so the package retains360100. Clip46C010
keeps the task when any eligible camera/player is within range; SET flag8 or
zero range disables clipping, and a Sonic partner uses1600. Range46B750 admits
equality and rejects unordered comparisons. These routines are inspected and
hashed, but their camera/player clipping service is not yet wired to the helper.

## External package and pose

`tools/build_pc2004_checkpoint_assets.py OWNER_EXE OWNER_SYSTEM OUTPUT`
requires the exact owner identity and refuses output inside the source tree.
It carries seven original Sonic SET rows (four act0/three act1), original CCI,
player CCI/category rules, original Ninja quarter-wave table, visual-node
positions and separate dynamic-land offsets. It contains five Basic components:
base, two inactive posts and two active posts. Only the first material of each
post receives the original59/60 texture selection; other material fields remain.
The CHECKPOINT format is versioned and the borrowed reader rejects CRC/bounds,
unknown/duplicate chunks, malformed placements/CCI/LUT, oversized geometry or
textures and nonzero reserved fields. Bytes must outlive every borrowed view.

The callback pose follows44EEF0 parent T/Rz/Rx/Ry and44EFE0 post T/Ry(+/-)/Rx.
It uses original lookup samples, preserves each callback's translation and
ignores default post object rotation/UNIT flags as the actual callback does.
It does not substitute a static hierarchy evaluator or rotate about world origin.
Dynamic-land children retain their distinct916A50 offsets for future integration.
Tint/fog/lighting callbacks and dynamic-land collision remain separate executors.

Final private package150416 bytes SHA256
038013498daf121e79b0faa80175c7d5e108c23a0283075654833ec0f0de9138,
located at /private/tmp/sa3ds-wca47-checkpoint.sa3dspak. It has not been installed
into the native asset tree because there is no native loader call yet. No
converted owner data, private disassembly or build enters Git.

## Verification and next integration

Required cumulative host gate PASS. New checks cover state initialization,
persisted activation, prior-CCL identity, display-before-activation snapshots,
positive/negative spin,743-update settle fixture, three effects,300-count blink,
all65536 signed damping inputs and independent force/wrapping boundary cases.
Synthetic asset checks cover borrowed valid views, poses and bad CRC/size/
reserved/CCI rejection. Additional force-boundary assertions were added after
the cumulative gate and the focused task runner passed again; production unchanged.

Required exact-owner pose/import gate PASS, now including independent raw-owner
checkpoint callback matrix/material checks: seven placements,42 poses,
2016 matrix components,max error4.72656257e-6; inactive59/active60 selectors match.
Existing source Sonic pose/transition/world gates also pass. Static owner witness
extension to SET clipping/loader hashes passed separately after that gate.
Native staged compilation PASS, no new warnings. The linker drops the currently
unreferenced components: output is byte-identical to WCA45's440076-byte
1d8b24d8 artifact. No redundant emulator boot was run and no new native checkpoint
behavior is claimed. Existing uploader/placement-reader warnings remain.

Next: native loader/art, list6 post task before the CCL pass, source spheres and
cached identities, SET flags10/4 across reload/unload, source time service,
actual player record capture and respawn, then a bounded activation/death/
checkpoint-restore fixture with raster inspection. Dynamic-land children,
original culling/clock order, particle/audio/HUD/tint executors and natural
whole-course checkpoint traversal must remain explicit outstanding gates.
