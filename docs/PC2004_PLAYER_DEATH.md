# WCA43 — ordinary zero-ring death and act-start respawn

WCA43 replaces the ordinary zero-ring quarantine with source command50,
passive action86, original death animations, the separate122-update restart
task, source fade/reload phases and life deduction. The bounded native test
respawns at the original act start and regains control. Checkpoint activation,
game-over/menu execution, audio and other death causes remain incomplete.

## Source contracts

Authority remains the owner's exact PC2004 EXE, SHA256
6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74,
and private sonic.exe.c. No executable data or decompiled code enters Git.
The static verifier retains hashes/addresses/small witnesses only.

-440CD0 sets command50/status1000/dead flag4000 and allocates440C80 in list0.
 40B030 prepends new tasks;40B0C0 snapshots the next link before callbacks.
 A task created during Sonic's callback therefore first updates next world tick,
 before Sonic. Native death tasks use that order; source allocation failure
 remains observable rather than inventing a restart.
-440C80 increments its uint32 counter, then tests the OLD unsigned value>120.
 Restart request2 therefore occurs on update122 fromzero. It requests audio
 cleanup and deletion; special4258F0 can delete early. The portable helper also
 preserves optional parent-status reporting and uint32 wrap behavior. Ordinary
 native tasks have no parent. No guessed120-tick reset is used.
-495FA0 command50 writes action86/request86, masks status withDAFF and emits
 sound5DE (or alternate7FE/channel1). Command/status provenance is retained.
 Source action86 selection is the default no-selector branch. Movement49BDB4
 writes87 from incoming ground bits, then443AD0/448E50/443F50/44CDF0/43EE70.
 The original grounded first-death-frame write happens before positive normal
 velocity departs ground. No normal hurt-recovery selector may release this state.
- Dense86 maps to ACTION116/root72/22frames/property5/self-next/transition1.
 Dense87 maps to ACTION118/root72/70frames/property5/self-next/transition.25.
 ACTION118 is distinct from normal hurt landing117. It clamps at69; it does
 not NextAnim0. Pose selection remains read-only and native property5 sampling
 remains nonlooping.
- Action-stage416610 states15/6/2/3/4 follow source faders42C630/650 and
42BF70/42C570. Source low16 return semantics differ between initial delay and
completion; they cannot be replaced by one generic guessed fade duration.
 At ordinary interval1, source fade-out completes on call46, reload is requested
 on the sixth fade-in call, then state4 runs49 world updates before returning15.
 Input40EF50/40EF40 is disabled through fade/reload, including the final state4
 world update. Latched button edges are consumed even when no world task runs.
-425AF0 initializes fresh lives to4;425B60 uses signed8 addition and only clamps
 a positive award that becomes negative to127. Ring extra-life events feed that
 counter. The actual game-over branch is surfaced as pending, not infinite lives.

Native reload recenters the actual imported collision window at the owner's
current-act start before resetting player/actors/animation. It does not teleport
into the old collision window or use the debug zone-reset path. With no native
checkpoint activated, this is the source act-start case only. A source-frame
counter now persists across respawn, separately from reset player.sim_frame;
full global RNG/task/contact/clock parity remains open.

The fade uses a tiny persistent linear quad and the existing shader with a
constant black TEV color. TEV alpha is packed in the high byte. CPU texture
upload, external owner-pair loading and boot diagnostics remain intact. Other
material tint packing needs a separate renderer audit; no broad color-fidelity
claim follows from the fade fix.

## Checks actually run

Required current host gate PASS, including existing controller/hurt/homing and
new death/restart cases. The focused death check covers command masking/sounds,
old-counter121/122/wrap/special behavior, passive physics with held analog/jump,
read-only death pose/clamp, source46/6/49 fade boundaries and signed lives.
A later input-phase result-field edit was covered by the focused check and final
native run; unchanged matrices were not repeated. One test expectation for an
already-pending request incorrectly expected controls enabled; it was corrected
to match the no-world/no-input result, then the focused check passed.

Required exact-owner pose/import gate PASS, now explicitly including ACTION118:
223nodes,7frames,14049scalar comparisons,maxerror9.57568359e-7. Transition and
animated-world oracles also pass. No owner package was changed. The final static
owner verifier PASS covers14 focused code spans, task-order/death/fader/input/life
witnesses and source action-stage dispatch. These are static checks, not retail
trajectory execution.

Final staged code-only native build PASS:438752bytes, SHA256
3c9ddacba4e0336ea94f680dfaae356f349981f41f7f1a34489f8f0007d23bca.
Existing unrelated uploader/placement-reader compiler warnings remain.

That exact artifact ran Azahar2126.0 Old3DS/Vulkan, original act0 Saru row28,
controlled zero-ring inventory and supported recoil facing. Native events:

| Source-frame diagnostic | Actual event |
|---|---|
|2|accepted zero-ring damage; action86 command; death task created|
|124|task counter122 requests restart2; stage15->6|
|170|fade-out completes; stage6->2; lives4->3|
|171|stage2->3, opaque fade retained|
|176|owner act0 start(-9,16,4) and resident collision reloaded; stage3->4|
|225|fade-in completes; stage4->15|
|243|PASS:12 forward-input/horizontal-motion updates, cleared dead flag|

The uninterrupted final fixture records1death task,1restart request,168death
pose updates,96clamp observations,1reload,lives3,rings0,action2. The control check
requires positive forward analog AND positive world-Z velocity, grounded action,
and no dead4000 flag; vertical velocity alone cannot pass it.

A separate inspection run of this SAME final artifact used private Z F to pause
at opaque source fade169. Screenshot inspected: top screen fully black, bottom
console still shows lives4/flow6. The keyboard API could not send simultaneous
Select+A nonmodifier keys, so the same binary was replayed with only that private
pause disabled for the uninterrupted result above. Screenshot after respawn was
also inspected: visible original beach/Sonic, lives3/flow15.

An earlier acdd3653 candidate FAILED the opaque raster check: it packed alpha in
the low byte and the scene remained visible. The corrected high-byte packing is
verified visually on the final artifact. Earlier cbe1a3f5 ran the flow but had a
weaker control assertion and no opaque raster check; it is not the final preview.

Final300-frame diagnostics include paused tail:243logic slots/7622ms,
submit6093ms,max432ms,minLinear26569472,maxCmd0.253601. Not steady gameplay FPS
or measured physical Old3DS performance. Native inspection/fixture flags are
removed after checking; owner data remains external.

## Remaining acceptance

Original checkpoints and their respawn records, game-over/menu/HUD, audio,
fall/water/death-mesh producers, full pause/special-mode behavior, full-stage
collision/camera/traversal and retail/hardware comparisons remain open. The
ordinary no-checkpoint zero-ring path is bounded evidence, not full CP7.

Owner now reports otherwise smooth Old3DS play with sudden freezes while moving.
Exact tested build/timing data are not yet established. Next investigation is
synchronous collision-region reconstruction and SD logging during movement,
with measurements before claiming a hardware stall fix. Full WCA and
PHYSICS_1TO1 remain incomplete.
