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

## WCA31 original animated asset conversion

`tools/build_pc2004_saru_assets.py OWNER_EXE OWNER_SYSTEM EXTERNAL_OUTPUT_DIR`
writes saru.sa3c, saru.sa3t and a source manifest outside this code tree.
Six stable package ACTION slots reference the reviewed source routines:
0=96A89C,1=9669EC,2=967964,3=968ADC,4=969924,5=96B444.
Frame counts16/16/16/16/13/7. These slots are source identities, not guessed
behavior labels. Shared serialization preserves complete hierarchy/node flags,
raw source UVs, materials, normals and sampled-key channels. It keeps Sonic's
strict primitive profile; the explicit enemy option handles triangle meshes and
N-poly streams. Missing UV streams produce zero coordinates (material flags
retain texture-enable semantics). Native triangle-list preflight accepts the
corresponding source metadata tags while checking finite vertices/index bounds.

The source model contains15 Basic models,50 meshes,439 converted vertices,
244 triangles,13 distinct materials and12 textures. No vertex-color streams were
present. Original N-poly records follow the count/reversal strip representation
in the public [SA Tools polygon implementation](https://github.com/X-Hax/sa_tools/blob/master/Libraries/SAModel/Poly.cs).
This interpretation has not been matched to the retail rasterizer.

External SA3C42448B SHA256
90c9a7b419af786cd1264dd48163128704aa1c43e302795190d1665ab00adfd3;
external SA3T57184B SHA256
752b2037f7020e0997c9c66a67aace3a9669e6444bbc1b3ec2dadbdcb1d273fb.
RGBA pixels56832B;131 partially transparent texels. No owner payload is committed.

`tools/verify_pc2004_saru_assets.py OWNER_EXE SARU_SA3C C_WORLD_DUMPER`
independently checks732 expanded source corners with normals/raw UVs and all
50 material assignments. Thirty sampled C animated-world poses across the six
clips compare against raw integer-key/source-LUT equations:18240 components,
maxerror5.48858826e-6. This reuses the established raw-key oracle, without using
the converter to derive expected poses. GPU format/geometry/texture preflight
also passes. These checks establish source-data consistency, not AI timing,
blend timing, native enemy rendering, retail raster parity or hardware success.

Required host, owner pose/import and native build gates passed. The preflight
change yields381608B native artifact SHA256
 ae538ee995c835a23e3ba7a0dfac1236aa37177056ed6e36b8bb95c01684ca91.
No emulator run was repeated for this bounded format extension. Enemy AI,
projectiles, damage/death and native homing-target registration remain pending.

## WCA32 logic-side blend timing

4AB3F0 and4AB5E0 invoke406EC0 with current ACTION/frame, pending ACTION/frame
and old blend weight. Only afterward do they add blend rate, store the new
float weight and compare the retained sum to1. Strictly greater completes:
clear enemy flag200, zero weight, copy pending frame/ACTION to current.
Exactly1 continues blending for that call. The retained-sum distinction matters:
1+2^-25 rounds to float1 but the source comparison still completes.

Source AI writers set flag200, pending frame/rate/ACTION without clearing weight.
A request during a blend therefore retains weight. Constructor current frame3,
current/pending ACTION slot0, flags483 and zeroed remaining animation state are
preserved. The later4AB2B0 display only reads the resulting state. Moving this
advance to a normal render callback would change timing under skipped frames.
4AB3F0 has an initial visibility test and later global suppression;4AB5E0 has
its own differing gates. The portable helper accepts explicit draw eligibility;
callers must reproduce those conditions and call order, not assume one call per
rendered frame. Noneligible calls draw/advance nothing.

Focused host checks cover old-weight snapshot, strict equality/completion,
retained-sum boundary, interruption and noneligible calls. Original helper body
digests are verified by the owner definitions builder. Required cumulative host
and staged native build passed. This unreferenced helper leaves WCA31 native
artifact ae538ee9 identical. No new emulator/retail/hardware check is claimed.
Actual arbitrary-frame two-motion pose evaluation, AI state updates and native
actor connection remain pending; this state helper does not implement them.

## WCA33 actual two-motion poses

saru_pose.[ch] now evaluates the exact six-ACTION/38-node profile. Source406EC0
routes to406C40: each motion frame wraps independently by its own frame count.
406090 samples position as weighted products using404B20 key selection, next-key
wrap and denominator1 on nonpositive key-time differences.406140 samples raw
integer angles with truncation.406680 then blends those integer angles again
with truncation, without signed16 shortest-arc normalization. Position/scale use
weighted products. This differs from Sonic43F2A0's captured transition system;
that existing system is unchanged. Unblended Saru uses the ordinary ACTION path.
No SKIP or mixed rotation-order nodes occur in the current owner rig; the new
helper rejects those profiles. It owns no animation clock, actors or allocation.

Focused owner-free checks cover long raw-angle crossing, independent clip wrap,
wrap-to-first-key interpolation, ordinary-action fallback and invalid weights.
Independent raw-owner verifier checks seven blended poses/2394 local components,
including pending/current frames past their clip ends: maxerror4.92431651e-9.
Exact original sampler/combine/entry/world helper digests are owner-side gates.
This is source-equation evidence, not execution of original x87 retained80 math.
Required host, owner pose/import and native build gates passed; same native
artifact ae538ee9. No redundant emulator run. These helpers still need the real
AI/shared collision/projectile/damage/death/native actor lifecycle.

Original4ACF80 dispatch mapping, to reuse in the next implementation:
0->4ABCB0,1->4ABEF0,2->4ABFD0,3->4AC090,4->4AC470,5->4AC170,
6->4AC620,7->4AC250,8->common tail,9->4ACEC0,10->4AC7E0,
11->common tail,12->4ABA20 plus original death/effect/destruction calls.
4AB3F0 is a logic-side visual/terrain helper, not target detection.4ABB90 is
behavior arbitration, not blend-clock advancement. Blend advancement lives in
4AB3F0/4AB5E0;4AB2B0 is the read-only display. Keep these identities distinct.

## WCA34 ordinary actor state handlers

saru_actor.[ch] implements the ordinary4ACF80 dispatch after the caller's4258F0
special-mode and46C010 lifetime decisions. Constructor preserves original orbit,
mode, packed tag, initial frame and one RNG15 call. Active updates retain source
state order for idle0, gestures1/2, turns3/5, orbit4/6, facing7, throw9 and taunt10.
8/11/out-of-range use the common tail.12 emits the original death/destruction
request instead of running the normal tail. It does not execute death effects.

Important behavior:

- Idle consumes RNG before its logic draw and proximity arbitration. The625 and
  40000 squared-distance checks are strict. The middle-distance transition to7
  returns false, allowing the handler's remaining source code to continue.
- Current frames increment at source-specific rates, with source retained versus
  stored comparison boundaries. Blend completion can allow the handler to advance
  the newly selected clip on the same tick. Facing7 draws after its decisions;
  other handlers have their original earlier draw placement.
-3/5 cap facing by400 BAMS toward4000/C000, then select the original orbit clip.
  4/6 increment/decrement orbital phase by trunc(65536*sourceInverseTau/(radius+3)),
  retain raw task angles, update source-LUT orbit position and bob phases670/9A8.
-4AB3F0 has its initial visibility gate;4AB5E0 does not. Negative-scale status2000
  queries49EE30 before draw suppression.4CDF60 is the damped squash spring:
  retained velocity update, scaleY clamp.3..1.7, scaleX=2-scaleY. Draw scales are
  captured before this update; later display reads the updated state.
- Throw9 restores the saved task position, requests a projectile at original
  current frame>9.5 before adding.3, then returns to idle at clip end with source
  wait/blend/sound. The request retains joint14 pose context; it does not create
  a projectile. Mode2 skips the normal throw choice.
- Common damage calls retain the conditional-plus-unconditional order. Idle has
  an additional early conditional check. Death selected in the normal tail runs
  the death handler next tick. Short counter wraps16 bits; old long age>600
  clears status400. Status100 controls CCL enqueue; display is separately requested.

Services explicitly require original trig/atan/RNG, floor query and shared4CE030
damage behavior. The module emits dust, sound, projectile and death requests for
later original-system integration; no fake collision, substitute bomb or silent
native damage stub is supplied. Sound requests distinguish424880 actor/120 from
424FC0 positional calls. Source frame counts are supplied from the owner package.
SET activation, constructor versus active phase, task/list ordering, previous CCL,
special mode4AB7C0, effect execution, rendering and native homing remain caller work.

Focused source-neutral tests cover RNG consumption, strict distances, middle
arbitration behavior, constructor, turn/orbit states, blend completion/order,
visibility/suppression differences, mode2, projectile threshold/order, taunt
random boundary, damage/death ordering and counter/status timing. Owner verifier
checks actual13-entry jump-table/call mapping and source constants, then28 first
orbit ticks for all14 SET rows with source quarter-LUT: maxerror0.0. That fixture
retains Y through an injected floor callback and proves no actual floor collision.
It does not execute the retail AI, shared damage or projectile runtime.

Required host gate and staged native build passed. Pose/import data unchanged;
no repeated Sonic oracle. Unreferenced actor code leaves the381608B/ae538ee9
native artifact identical; no repeated emulator run or new FPS/memory claim.
