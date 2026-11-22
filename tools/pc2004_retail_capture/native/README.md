# Native original-game frame capture

This is the Wine-compatible counterpart of the existing Frida observer. It
records **one row on return from the original Sonic main routine**, with the
input-filter snapshot consumed at entry. It reads the physical and final logical
button states, raw axes, position, world/local velocity, task angles, animation
request/index/state/phase, and collision-work fields. Frame numbers are local
Sonic-update indices, not video-frame numbers or an observed global counter.

Capture version2 reads the active normalized analog at `3B0E7A0/4` on Sonic-main
entry, matching `43BDC0`'s input reads. Version1 captured the unused normalized
buffer at `3B0E368`; its `filtered_angle`/`filtered_magnitude` fields must not be
used as Sonic control inputs. The decoder identifies that limitation explicitly.
Raw peripheral fields and animation/physics observations retain their original
meaning. Forced-input branches and the control-timer gate still belong to
`GetAnalog`; a buffer snapshot is not a hook of that function's return value.

This follows RockMan3ds's useful pattern: keep frame-keyed original state in RAM,
then compare matching updates and report the first difference. Its PS1 oracle
RPC cannot directly drive a Windows executable, so this observer supplies the
corresponding endpoint for PC2004.

## Boundaries

Only the exact supported PC2004 executable fingerprint is admitted. The loader
selects a unique `sonic.exe` process in its Wine prefix and hashes its actual
executable before loading anything. The DLL checks the profile again and compares
the three observation entries against the owner's file before enabling hooks.
An existing modification at those entries causes rejection.

The observer installs reversible **code hooks**, using MinHook 1.3.4. It does not
change controller values, player state, physics, animation values, saves, or OS
keyboard/mouse input. The hooks preserve the original x87/SSE state. The raw
second Act1 argument is passed as 32 bits, without interpreting the decompiler's
float type. Code-entry restoration is checked when recording ends.

The disabled observer module and its trampolines remain resident until the game
exits, avoiding a race with a detour epilogue. Recorded rows are freed after the
external result is written. Closing/crashing the game before completion can lose
its RAM recording; a ready marker alone is not a successful capture.

All generated DLLs, executables, recordings, raw traces and CSVs must stay outside
the source checkout. None contains a redistributed original executable or asset.

## Build

Use a portable Zig C compiler and the official MinHook v1.3.4 source tree:

```sh
python3 tools/pc2004_retail_capture/native/build.py \
  --zig /external/zig/zig \
  --minhook-source /external/minhook-v1.3.4 \
  --output /private/tmp/sa3ds-observer-build
```

The actual build used Zig 0.17.1. Its official macOS ARM archive SHA-256 was
`75f1bc7a1e9852f17ec9a0c1895f767c4ec56d46bc8a834920eece1d0359dd8d`.
The downloaded [MinHook v1.3.4 source archive](https://github.com/TsudaKageyu/minhook/releases/tag/v1.3.4)
SHA-256 was `3f9a4c9801f9ae39ef45a5aae699b3a530c087533c6c324fce0919d82147b642`.
The build records input/artifact hashes and copies MinHook's license beside the
binaries. No owner data is needed to compile it.

## Capture

First run `tools/verify_pc2004_retail_capture.py` against the exact owner EXE.
Use the dedicated reference Wine prefix. `inject.exe --probe` only checks the
process fingerprint and reads the current controller state; it installs no hooks.
Wait until its player-task readiness check says yes before injecting. A running
process during startup or on a menu is not sufficient; early Wine injection can
stall initialization. Prefer one capture per fresh reference process.

Create a fresh external run directory, copy `observer.dll` there, and write:

```ini
[capture]
frames=1800
timeout_seconds=180
```

Save that as `capture.ini`. Run `inject.exe` in the same prefix, passing the full
Windows path to that DLL. Process enumeration crosses Wine's virtual-desktop
boundary, where `FindWindow` from another desktop cannot find the game.

`capture.ready.json` means the three hooks were installed. Play normally.
`capture.raw` is written after the requested count or timeout; check
`capture.error.txt` and decode the result before accepting it. The buffer limit
is 60,000 updates. No frame rows are written to disk during gameplay.

```sh
python3 tools/pc2004_retail_capture/native/decode.py \
  /external/run/capture.raw --output /external/run/frames.csv
```

The decoder rejects incompatible profiles/layouts, error markers, missing
chronology, non-finite values and missing original Act1 updates. It hashes the
raw and CSV artifacts. Missing collision/camera snapshots have explicit validity
fields; do not interpret their zero placeholders as observed geometry.

## Compare playback first

`check-playback` links the actual SA3DS animation runtime. Supply seven numeric
columns: capture-local frame, requested animation, source index, animation state,
phase, local forward speed, and original shared speed scalar. These correspond
to CSV columns `frame animation_requested animation_source animation_transition
animation_frame local_velocity_x spindash_charge`.

The comparator seeds known stable spans and drives the scripted-request API with
recorded requests/scalars, isolating playback. It compares active IDs, blending
state and phase, and stops at the first divergence. It excludes states outside
2/3/9/10, including automatic NextAnim boundaries whose incoming request was not
captured separately. It does **not** validate the control selector, terrain,
model sampling, camera or renderer.

## First live evidence

The owner-controlled run produced 1,800 original Sonic-main returns, all with
Act1 observed, zero producer errors and zero warmup omissions. Decode checks
passed. The run contains walk/run, spin/jump, roll, hurt and braking states.
The capture covered 115.80 wall-clock seconds, so neither video timestamps nor
capture-local indices establish a locked 60 Hz reference.

The linked playback component matched **1,552 updates across eight seeded
spans**; 240 updates were excluded. This is component evidence, not port parity.
The capture was mid-session and did not record a stage ID or matched SA3DS spawn.
A complete position/physics comparison still needs matching stage, initial state,
inputs and update alignment. The original code entries were restored after capture.

## Targeted original loop scenario

`prepare_loop_scenario.py` builds an explicitly state-writing reference variant
outside Git. It validates the executable, derives its one-time start from owner
SET row0, boots the original Trial dispatcher in memory, and holds logical forward.
It changes no movement/animation code or OS input. It is not a normal-start course
acceptance run. The ordinary `observer.dll` remains passive.

```sh
python3 tools/pc2004_retail_capture/native/prepare_loop_scenario.py \
  --owner-game /owner/PC2004 \
  --zig /external/zig/zig --minhook-source /external/minhook-v1.3.4 \
  --output /external/fresh-loop-scenario
```

Launch its `start.exe /external/reference/game/sonic.exe` in the isolated Wine
virtual desktop. Wait for a ready player, then load its `scenario_observer.dll`
using `inject.exe ... --expected-game /external/reference/game/sonic.exe`.
The optional expected-game argument binds both probe and injection to that exact
copy, in addition to the existing unique-process and executable-hash checks.
Use one capture per fresh reference process.

New recordings include `loop-state.raw`, an optional92-byte-per-update sidecar.
The decoder also accepts the earlier32-byte loop sidecar and recordings without
one. It joins by capture index and checks length, chronology and finite values.
The extended fields record incoming position, local/world velocity, task angles
and collision Up, alongside path distance, pointer, control timer and headings.
Incoming position matters: world tasks can change it after the previous Sonic
main snapshot. The main224-byte record layout stays unchanged.

`check-loop` and `compare_loop.py` compare the held-input43C130 component using
that incoming state and an external owner path package. The recovered first-loop
recording has360 valid updates; all79 active loop updates have exact path-distance
agreement and at most1 BAMS (about0.0055 degrees) heading error in the portable C
component. This admits no claim about force integration, contact, route parity or
emulator/hardware play. The original reference enters at17 and exits grounded at96.

The November22 recovery rebuilt helpers with verified official Zig0.15.2 and
MinHook1.3.4. Keep compiler caches and capture artifacts in persistent external
storage. The source reference launcher now selects `CHRMODELS_orig.dll` for the
private copy when available: the installed `CHRMODELS.dll` can be Mod Manager's
proxy and otherwise blocks on missing private mod profiles. The owner install is
unchanged; the selected original DLL path/hash is recorded in `reference.json`.
