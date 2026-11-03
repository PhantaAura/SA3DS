# WCA37 — native E SARU ordinary AI and read-only display

Starting0972a00. The active0G runtime now loads owner-generated
`saru_definitions.sa3dspak`, `saru.sa3c`, and `saru.sa3t` from the external SD
assets directory. No owner payload is compiled or checked into this repository.
Missing/invalid resources leave an explicit SARU boot diagnostic. Existing CPU
texture uploading, Sonic ACTION preflights, SD owner-pair loading and boot logs
are retained.

The definitions converter now writes compatible version2: fourteen original SET
rows, the enemy CCI, two owner player CCI records, and category0/3 rules. Version1
remains readable for historical initialization fixtures; native loading requires
version2. New profile fields are currently prerequisites for the next CCL bridge,
not a claim that native enemy combat exists.

Native tasks retain head insertion order, source600/700 distance thresholds,
source clip-level selection, original initializer/orbit, six source motion frame
counts, ordinary source actor decisions and read-only4AB2B0 pose display. Shared
textures are uploaded once. The GPU batch arena budgets all14 possible actor
submissions and resets once before the actor loop, then appends unique vertex
spans. The original single-Sonic draw wrapper still resets once per draw. Native
logic uses simulation ticks; rendering does not advance enemy animation.

The static floor adapter selects the highest resident upward triangle no higher
than taskY+4, excludes flags400002, and returns the source -1e6 no-floor sentinel.
It does not reuse the player's nearest-support-height query. A query outside490
world units of the collision-region center reports unavailable rather than
missing floor. Floor-tracked enemies defer until their complete orbit fits that
region. This is a bounded static-triangle adapter: original452B30 edge precision,
COL candidate ordering, dynamic platforms, shadow drawing and full route
residency are not reconstructed.

## Evidence

- Exact-owner definitions round trip/init verifier: fourteen SET/CCI rows, source
  quarter-LUT orbit max error0.0; version2 playerCCI/rule bytes match exact owner
  executable. This is data/equation evidence, not retail execution.
- Required `bash tests/run_sa3ds_current_host.sh`: PASS. Added multi-instance arena
  capacity/overflow/isolation, active0G begin/append contract and static floor
  highest-below/exclusion/no-floor/unavailable checks. Historical source contracts
  now delimit the actual Sonic block so separate enemy scaling is allowed.
- Required exact-owner `tests/run_pass11b_source_pose.sh`: PASS, including original
  ACTIONs, transition and animated-world oracles. Saru rig/clip bytes themselves
  are unchanged from WCA31.
- `bash tools/build_3ds.sh`: PASS. Code-only401128B artifact, SHA256
  `3f81bd669d185e49e0bf3831f45e0bc97f07b4bd0f5d06da8edfb552ee25936c`.
- Azahar2126.0/private Old3DS/Vulkan profile: explicit original act0 SET row26
  fixture PASS180 simulation ticks,163 actor draws before pause,max1 actor,
  action mask00000065 (states0,2,5,6),zero projectile requests. Enemy visibly
  inspected beside the original terrain/Sonic. This does not prove a multi-actor
  raster, source visual parity, combat, course traversal or physical hardware.
  Runtime300-frame diagnostic includes the paused tail:180ticks/6598ms,
  submit5861ms/max428ms,minLinear26569600,maxCmd.217834; not gameplay FPS proof.

Private owner definitions v2 are1344B SHA256
`bf89d5577aeca3755d7cf38c8498cec920a26218282d7e73164276d1448ffa53`.
The optional private `emerald_coast_saru_probe.flag` contains a source SET row,
relocates the player to imported nearby support and pauses after180 ticks. It is
removed after this check and is not required in normal play.

## Remaining work

Native CCL generation/previous-pass scheduling and pre-movement Sonic strength
capture are not connected. The existing shared source damage helper sees an
empty native collision/explosion world; ordinary contact cannot yet damage
Sonic or kill an enemy. Projectile/sound/dust requests are counted but their
original executors are pending. Score, bounce, death fragments/animal, held bomb,
projectile physics, special-world mode, homing registry and full SET persistence
are pending. Source visibility/camera relevance is currently the native frustum
adapter; logic render mode6/multipass rendering is not reproduced. RNG uses the
MSVC generator shared among native Saru actors, seeded1 on reset; full original
global task RNG order is not proven. Full WCA/CP5 and physics1to1 remain false.
