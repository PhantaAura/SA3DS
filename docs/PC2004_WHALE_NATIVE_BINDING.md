# WCA75 — SET preparation and native AO whale binding

`set_preparation.[ch]` adds the SET work-table rules the whale triggers need.
46C1D0 zeroes every entry and46BD20 appends the act's rows with flags|=0x8000
and load counter0. A row whose (type&FFF)+(act*100+level)*100 equals global
0091BA00 would receive a mission-table flag word; that table is not imported,
so a match is rejected. The exact executable has no static writer of0091BA00,
which therefore cannot match a level>=1 row. 46BE00 keeps only flags&7FE9 on
entries without0x4000 and installs main40B570 when the OLD flags had bit0;
bit0 and the association survive until that callback. 46BE50 compacts.
49DD70 calls46BF20, which resets rows without0x10 to flags0x8000/loads0.
46C2A0 runs clear, one idle step, compaction, then append.

`whale_task.[ch]` is the AO task executor on the shared list3 scheduler.
4F88B0 (lifetime helper) defers the old task, clears the shared kill word and
allocates constructor5029F0. The constructor runs on a LATER traversal: actor
state with the retained stage tip, two-sphere CCL with only the second shape
disabled, camera-disable request, destructor5022A0 and main5022C0. Main reads
Camera_Data1, the shared kill word and prior CCL status bit8, runs the actor
phase/tip/common tail, publishes the pre-movement tip into the first CCI and
the moved origin into the second, then records the502210 display transform.
Dive deletion is deferred (main40B570) and still runs the tail; far deletion
stops it. The destructor's4F8930 identity guard keeps a replaced task from
clearing its successor; RumbleA(0,0) is always requested. A task replaced
before its constructor ran has no destructor. Three bounded slots; exhaustion
reports an error and leaves a null current task, as the source would fault.

Disassembly of5028BB..502903 confirms bit8 is EntityData1 status byte+4 and
the bounce is441280(0,headingX*8,2,headingZ*8): world velocity, then G2P local
Speed with x/z in[-0.001,0.001) zeroed. Tip is the global03C5E3E8 triple,
enqueue41C280, wave4F89D0(x,z,0,1,13).

## Native binding

`main.c` loads `whale_triggers`, `whale_tip` (v2 required for action2 root),
`whale_ccl` and `whale_visual` packages from `sdmc:/3ds/SA3DS/assets/`. Missing
or invalid packages leave the whale absent, never a stand-in. Fresh stage/act
entry runs cold preparation for the current act's rows; death reload and
clear-world restarts destroy the task tree, then apply46BF20. Each world tick
runs the SET loading phase then the list traversal after checkpoints; the CCL
pass clears every whale body and tests player shapes against both spheres.
CCL IDs use base0x30000. The single sampled mesh draws the current task.

Bounded adapters: Camera_Data1 is the native follow-camera eye (retail camera
is CP4); 441280's G2P uses the flat yaw frame; triggers/whale share their own
list instead of the complete world list order; 46C2A0's multi-frame act change
is applied as task-tree destruction plus cold preparation. Wave, splash,
rumble, sound, free-camera and camera-disable requests are counted only.

## Evidence

Host: SET preparation and executor synthetic tests PASS (summon, delayed
constructor, emergence, bit8 bounce, kill-to-dive deletion, guarded destructor,
replacement, pre-constructor free, exhaustion). Owner gate: new46BD20/46BE00/
46BF20/46C2A0/49DD70 witnesses and an owner-package runtime PASS: real rows
435/693, v2 tip, CCL 40/60/150, visual sampling, summon->construct->emerge at
tick13, kill->delete in224 ticks (libm sine stands in for the Ninja table).
Required cumulative host and native build PASS; native image now links the
whale (498888B), no longer linker-discarded.

Azahar 2126.0 isolated Old3DS profile with locally generated owner packages:
boot log reports the four packages loaded and connected. The explicit
`emerald_coast_whale_probe.flag` fixture reported FAIL: the summon sphere at
(4301,8.5,480) has no static support within400 units. SET inventory shows why:
the route there is built from type59 `O PIER` (62 rows, callback4FF060),
type24 `C CUBE` (50) and type20 `WALL` (17) objects, none implemented. No whale
raster, chase or hardware result is claimed. Next: O PIER/C CUBE/WALL objects,
then natural summon, pier destruction and camera. CP6 and CP1–CP9 remain open.
