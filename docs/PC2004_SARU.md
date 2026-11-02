# PC2004 E SARU reconstruction

Scope: exact owner executable profile6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74,
registry kind29, constructor4AD140, update4ACF80, orbit4AB240, display4AB2B0.
Private LLVM disassembly and owner data are references only; no payload is committed.

## Definitions and initialization

The registry places9 enemies in Sonic act0 and5 in act1, list3, flags2,
allocation1, load distance squared360000. Runtime46C010 receives490000,
not the generic object hysteresis value360100. Its retention includes camera
and player relevance, plus a SET flag bypass; Sonic distance alone is insufficient.

The owner package retains complete32-byte SET records with act/source-row IDs
and the48-byte original CCL record96C9E8. Constructor registers one sphere in
category3. Shared CCL initialization sets target flag40 for this category;
task ID is not SET kind29. Rings use their distinct task ID11.

4CC990 allocates zeroed enemy work; constructor leaves enemy flags483,
behaviorE0, initial ACTION96A89C, animation frame3, parametersCC=1.6,
D0=1,3C=.1. One runtime RNG15 call determines floor(rand15*20/32768).
Do not bake that random value into owner assets.

ScaleZ is orbit radius, clamped to a minimum1. ScaleX sign selects task status
2000; magnitude is discarded. ScaleX/Y become1. ScaleY selects mode1 below0,
mode2 at1000 or above, otherwise0. All14 owner rows currently use mode0.
SET angleZ low/high bytes are separate values: the low tag goes to enemy+4C,
the high byte remains task angleZ. Its low tag is used by Knuckles-only tracking;
its broader meaning is unresolved. Do not treat it as ordinary model rotation.

AngleX supplies the Y-axis orbit phase.4AB240 computes radius along X through
a float Ninja Y-rotation matrix, then adds original spawn X/Z. Task Y remains
unchanged. AngleY is retained as initial facing. Source matrix float stores
precede multiplication and position addition. Fresh task status is100 or2100.
The portable initializer intentionally does not allocate an engine enemy or run AI.

## Original rendering and remaining work

Display visibility radius20; texture list962560 resolves E_SARU. Transform is
translation, Y rotation(angleX+angleY), scale1.2 times task X/Y/X, then Y rotation8000.
Normal draw uses current ACTION/frame; enemy blend flag selects the blend path.
Default ACTION96A89C points to root965510/motion96A890:38 nodes,16 frames,
type3/interpolation2, one position channel with16 keys and12 rotation channels
with192 keys. Model, animation blending and other clips are not converted yet.

Update dispatches13 actions, checks shared enemy damage, records current position,
advances task counters, conditionally queues CCL under status100 and invokes display.
Original AI/projectile/death/effects and shared enemy collision/physics remain open.
Native spawning is deliberately not claimed by this definitions-only checkpoint.

## Evidence

External package1232B, SHA256
34bcee1a00dd156f3ae90c16fe65b138eb1fd617195794afa74685d934f43974.
Builder verifies executable identity and constructor/update/orbit/display digests.
Independent verifier checks every original SET row, original CCL bytes and
source quarter-LUT orbit equations through the C runtime:14 rows, max error0.0.
This does not execute retail code or prove native AI behavior.

Required host gate, exact-owner pose/import gate and staged native build passed.
Native binary unchanged:381608B/c772cbb8; no repeated emulator run was necessary.
