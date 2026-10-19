# Sonic Adventure 3DS — fresh-chat handoff after 0.0.8.8.9

Copy the prompt below into a new project chat. Give the new chat the latest
checkpoint ZIP if it is not already available there.

---

## HANDOFF PROMPT

We are continuing **Sonic Adventure 3DS (SA3DS)**, a native Nintendo 3DS
reimplementation/adaptation of Sonic Adventure. Continue from the latest
source-only checkpoint **0.0.8.8.9 — Retail Collision Work Reconstruction /
Pre-Character Physics Freeze**. Do not restart the physics reverse engineering
from scratch unless new evidence or real-stage/hardware testing exposes a
specific mismatch.

### Project target / rules

- Old 3DS XL is the performance baseline; New 3DS enhancements can come later.
- Build a native 3DS runtime, not an emulator wrapper.
- Practical source/import foundation is PC/SADX, with Dreamcast SA1 visual
  authenticity as the default presentation target.
- Use only user-owned local game data. Distributed checkpoints must never
  contain Sega executables, DLLs, models, textures, PVM/PRS/ADX/audio, stages,
  saves, or extracted proprietary asset bytes.
- Extraction/runtime separation is mandatory: source profiles -> neutral SA3DS
  representation -> `.sa3dspak` -> 3DS runtime.
- Evidence gate: never label guessed constants, offsets, collision rules, camera
  rules, or asset semantics as exact. Keep candidate/evidence/proven labels.
- Keep physics independent from animation; physics owns motion, animation only
  presents the state.
- Bottom screen should ultimately be intentional SA1-era UI, not a permanent
  debug dashboard.

### Source policy

Primary development/extraction source is **SADX 2004 US PC**:
- `sonic.exe`
- size `55,549,952`
- SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`
- image base `0x00400000`
- profile `SADX_SOURCE_2004_US` / `sadx-2004-pc-6b0937ca`

Secondary independent comparison source is clean Steam SADX:
- `Sonic Adventure DX.exe`
- size `90,922,000`
- SHA-256 `e4330c00d7ee3910ecc1acab789ffd2a2d571c506984b99a8de4d3663e697fd6`
- image base `0x00400000`
- profile `SADX_SOURCE_STEAM`

2004 is primary because it matches the Mod Loader/SA Tools ecosystem. Steam is
not discarded; keep it for ownership/source baseline, existing RE, and
cross-checking. A Dreamcast Conversion/visual profile is an adapter, not a
separate runtime and does not automatically imply Dreamcast physics/camera.

### Physics status — DO NOT REDO COMPLETED WORK

Completed and passing:
- 0.0.8.4 Ground Parity Candidate
- 0.0.8.5 Air Parity Candidate
- 0.0.8.6 Roll + Spin Dash Candidate
- 0.0.8.7 General Surface Physics Foundation
- 0.0.8.8 Surface Evidence Pass
- 0.0.8.8.5 Steam <-> 2004 core physics parity audit
- 0.0.8.8.6 exact 2004 Sonic Spin Dash evidence lock
- 0.0.8.8.7 collision/support ordering evidence lock
- 0.0.8.8.8 exact checked `PGetBreak` / `PGetInertia` static-player arithmetic
- 0.0.8.8.9 retail collision-work (`csts`) reconstruction / core physics freeze

Steam and 2004 player parameter tables are byte-identical:
- record size `0x84`, 8 records, 1,056 total bytes
- shared table SHA-256
  `3f69076118fcc6a77c55e600eb337944eb6562e5b0979104b58058ad64ba5107`
- Steam table VA `0x025D9B90`
- 2004 table VA `0x009154E8`

Relevant 2004 functions:
- `PInitialize` `0x00442750`
- `PGetInertia` `0x00443650`
- `PGetSpeed` `0x00443F50`
- `PGetBreak` `0x00448E50`
- `PGetAccelerationAir` `0x0044B9C0`
- `PGetAcceleration` `0x0044C270`
- `PSetPosition` `0x0044CDF0`
- `PResetPosition` `0x0043EE70`
- `HandleSurfaceFlags` `0x0043CB90`

Spin Dash exact checked behavior:
- minimum starting charge `2.0`
- held test: if current charge `< 10.0`, add `0.4`
- 10.0 is not a hard clamp; pre-add comparison allows ~10.4 overshoot
- timeout 300 frames
- release copies stored charge directly to forward speed
- action 4 = Spin Dash; action 5 = Roll
- Spin Dash movement order:
  `PGetRotation -> PGetBreak -> PGetSpeed -> PSetPosition -> PResetPosition`
- Roll movement order:
  `PGetRotation -> PGetInertia -> PGetSpeed -> PSetPosition -> PResetPosition`

Support-order correction:
- old synthetic `normal_y < 0.3 && speed < 1.16 -> detach` rule was removed from
  primary 2004 path because `1.16` has four direct refs in `PGetAcceleration`
  and none in exact `PSetPosition/PResetPosition`.
- movement uses previous support/frame, integrates, then refreshes support.

### 0.0.8.8.9 csts discovery

Exact 2004 `playerwk+0x64` points to a separately allocated zeroed **0x84-byte**
collision/support work block. Public X-Hax `SADXStructsNew.h` is used only to
cross-check field names; exact binary offsets are authority.

Portable layout:

```c
struct csts {
    float radius;              // 0x00
    vec3 pos;                  // 0x04
    vec3 spd;                  // 0x10
    vec3 tnorm;                // 0x1C
    uint16_t find_count;       // 0x28
    uint16_t selected_nmb;     // 0x2A
    float yt;                  // 0x2C
    float yb;                  // 0x30
    int32_t angx;              // 0x34
    int32_t angz;              // 0x38
    vec3 normal;               // 0x3C
    vec3 normal2;              // 0x48
    vec3 onpoly;               // 0x54
    vec3 pshbk;                // 0x60
    vec3 anaspdh;              // 0x6C
    vec3 anaspdv;              // 0x78
}; // 0x84
```

Normal helper evidence:
- initializes `tnorm=(0,1,0)`
- copies task X/Z angles to +0x34/+0x38
- loads player parameter `height`, multiplies by `0.5f`, uses it as csts radius
- Sonic height = 10.0, so reconstructed retail csts radius = **5.0**
- broad polygon routine `0x00453820`
- arbitration A `0x0043CCE0`
- arbitration B `0x0043D180`
- both arbitration paths read `find_count` at +0x28 and contain cutoff `0x2C00`
- accepted flags flow through `HandleSurfaceFlags` `0x0043CB90`

0.0.8.8.9 source adds `pc2004_collision_work.[ch]`, compile-time offset/size
asserts, layout/init tests, runtime diagnostics, and
`verify_pc2004_collision_work.py`.

Important boundary: the complete retail polygon iteration/contact-response
algorithm is NOT yet translated 1:1. Unknown response-vector semantics are not
fabricated. The original white lab keeps its 1-unit fixture stand-off while
retail csts radius 5.0 is tracked separately. Full polygon edge parity is now a
**real-stage collision calibration gate**, not a blocker for character work.

Therefore freeze the **core movement math and csts architecture** now. Do not
claim global 1:1 physics/collision yet.

### Immediate next task — 0.0.8.9 Character Presentation Test

Start with **0.0.8.9A Sonic Source Extraction**.

Preferred 2004 source set:
1. `sonic.exe`
2. clean `CHRMODELS_orig.dll`
3. `SONIC.PVM`

Known clean model DLL:
- `CHRMODELS_orig.dll`
- size `7,647,232`
- SHA-256 `76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5`

Do **not** use the converted install's current 1,031,680-byte `CHRMODELS.dll`
as base model data; it appears to be the Mod Loader/current hook DLL.

Known 2004-compatible texture archive:
- `SONIC.PVM`
- SHA-256 `f8e30dfbebeb901a0ac210a61929c45fbcefcc1a2758cba400f959989b93ecbe`

0.0.8.9A goal: locate/extract Sonic model hierarchy, skeleton/object tree,
motion/animation references, and textures into a **neutral SA3DS character
representation**, with manifests/diagnostics but no Sega asset bytes in the
source checkpoint. 2004 first, Steam cross-check, Dreamcast visual profile later.

Then:
- **0.0.8.9B:** Old-3DS-friendly textured model + skeleton renderer
- **0.0.8.9C:** animation playback and physics -> animation bridge
  (`Idle`, `Walk`, `Run`, `Jump`, `Fall`, `Land`, `Roll`, `Spin Dash` minimum)
- **0.0.8.9D:** put animated Sonic into the white Movement Lab, build `.3dsx`,
  test on real Old 3DS hardware. Physics owns movement; animation never drives
  core velocity.

### White Movement Lab / controls

The lab uses original non-Sega geometry: runway, slopes, banks, raised paths,
ramps, drops/reset areas and loop-like surfaces. After 0.0.8.9D it should be the
first proper playable SA3DS movement build with animated Sonic.

Control plan:
- Circle Pad: move/steer
- A: jump
- B/X: contextual ACTION
- L/R: camera
- Y: recenter
- START: pause eventually; current dev sandbox may exit
- SELECT: dev overlay only
- touch: UI only
- D-Up: Light Speed request
- D-Down: Spin Dash request
- D-L/R: camera utilities
Do not hardcode Spin Dash exclusively to B; Classic/Restored profiles can map
logical actions differently while sharing gameplay logic.

### New roadmap

PHASE A — character / playable lab
1. 0.0.8.8.9 Retail Collision Work Reconstruction — COMPLETE; core physics freeze
2. 0.0.8.9A Sonic source extraction
3. 0.0.8.9B model/skeleton renderer
4. 0.0.8.9C animation playback + physics/animation bridge
5. 0.0.8.9D animated Sonic in white lab + `.3dsx` + Old 3DS hardware test

PHASE B — first real stage
6. 0.0.9 Emerald Coast LandTable extraction
7. 0.1.0 `.sa3dspak` stage+character runtime packaging
8. 0.1.1 first real Emerald Coast render
9. 0.1.2 real-stage collision/physics calibration; validate remaining polygon,
   support-loss, wall/ceiling, dynamic-contact edge cases here
10. 0.1.3 complete Sonic character runtime/actions needed by Emerald Coast
11. 0.1.4 Emerald Coast gameplay objects
12. 0.1.5 playable Emerald Coast vertical slice
13. 0.2.x master Emerald Coast

PHASE C — Adventure / story foundation
14. 0.3.0 Station Square Adventure Field foundation
15. 0.3.1 story-state/event gating
16. 0.3.2 NPC/doors/transitions/field-object systems
17. 0.3.3 first complete story slice: Station Square -> Emerald Coast -> return
18. 0.4.x next stage/Adventure Field cycle
19. later: remaining stages/bosses, all characters, cutscenes/audio, saves,
   Chao, complete story, optional New 3DS enhancements

Architecture path:
`ENGINE FOUNDATION -> PLAYABLE WHITE LAB -> EMERALD COAST -> STATION SQUARE + STORY STATE -> FIRST COMPLETE STORY SLICE -> NEXT STAGE/HUB EVENT -> ...`

### Test/build discipline

Before advancing a checkpoint:
- run `sh tests/run_host_tests.sh`
- run exact-source verifiers when the owner-supplied EXEs are available
- preserve trace determinism and the 19 Movement-Lab regressions
- add regression tests whenever new proven behavior is promoted
- package source-only checkpoint and report SHA-256
- never include the local Sega inputs in the ZIP

On the user's Mac, devkitPro/devkitARM is expected at:

```sh
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=/opt/devkitpro/devkitARM
export PATH="$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"
```

Old 3DS hardware is the final authority for Circle Pad deadzone, latency,
frame-pacing, camera feel, and surface stability.

**Continue directly with 0.0.8.9A. Do not spend another patch re-investigating
completed ground/air/Spin-Dash/Break/Inertia/csts work unless new evidence
requires it.**

---
