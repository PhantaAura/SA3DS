# Windowed PC2004 reference on macOS

This launcher uses the owner's existing Wine app and exact PC2004 installation.
It prepares a new **external** game directory, copies the executable, settings
and saves, and links the two data directories. Nothing from that directory may
be committed. The original executable and installation are unchanged.

The November 21 investigation found an early exit in `7943C0`, after `7941D0`
failed to select a display format. Wine exposed 18 modes; zero matched the
original requested 640×480. A temporary copy using the original 800×600 preset,
a matching Wine virtual desktop and OpenGL reached the animated title screen
and remained running for repeated window captures. The Vulkan attempt created
a surface but reported missing fixed-function render states; a black startup
capture was not proof of a working reference.

The owner subsequently controlled the game into the Chaos 0 encounter. A
45-second, window-only recording completed and a gameplay frame was decoded.
The owner reported unplayably low frame rate with the original Wine D3D8 /
OpenGL path. This is not an acceptable motion-timing oracle yet. The separate
d3d8to9 v1.16.0 experiment has rendered the title screen; the owner reported it faster but still slow. Neither path has passed performance
acceptance. No renderer or gameplay parity is claimed.

The next reference acceptance is responsive Emerald Coast gameplay and a
recorded original input/state trace alongside the video.

## Launch

Provide the existing isolated Wine `.app`, owner game directory and a new output
directory outside any source checkout:

```sh
python3 tools/pc2004_reference_macos/launch.py \
  --wine-app '/external/Sonic PC2004 Oracle.app' \
  --owner-game '/external/owner-game' \
  --output '/private/tmp/sa3ds-reference-run'
```

`--prepare-only` validates and prepares the directory without starting Wine.
`reference.json` and `wine.log` stay with the private files. The launcher waits
for the isolated prefix's Wine processes after Explorer hands off. Use a prefix
dedicated to the reference, since that wait includes other programs in it.
The desktop is 800×600 and the original windowed setting is enabled. The game
may activate its own Wine window during startup; the tool sends no OS input.

## Capture only Sonic's window

On macOS 14 or later, build the small helper outside the source tree:

```sh
swiftc tools/pc2004_reference_macos/capture.swift -o /private/tmp/sa3ds-window-capture
/private/tmp/sa3ds-window-capture SONIC_MAC_PID /private/tmp/sonic-reference.png
```

Use the Mac process ID of the particular `sonic.exe` Wine process, rather than
its Windows process ID. The helper accepts only that process's
`SonicAdventureDXPC` window. It never captures the desktop, activates a window,
or sends keys. Existing macOS screen-recording permission is required; errors
and a 15-second timeout are reported explicitly. A hidden or transitioning
window may not be shareable. Capture outputs inside a Git checkout are rejected.

Screenshots and generated owner packages remain private external references.
Keep altered reference music out of audio-fidelity comparisons.

## Record an owner-controlled run

On macOS 15 or later:

```sh
swiftc tools/pc2004_reference_macos/record.swift -o /private/tmp/sa3ds-window-record
/private/tmp/sa3ds-window-record SONIC_MAC_PID /private/tmp/sonic-run.mp4 45
```

The recorder includes only the selected Sonic window, capped at 30 capture
frames per second. It includes no microphone, application audio, desktop, or
keyboard logging. The console reports recording start and successful finish.
A video capture rate is not the game's simulation/render rate. Video of a slow
reference must not be used to tune animation timing to wall-clock seconds.

This records the visible result of the owner's controls. It does **not** record
exact buttons, axes, simulation ticks, or player state. Use the separate
[`pc2004_retail_capture`](../pc2004_retail_capture/README.md) endpoint on its
supported runtime for a verified input/state trace.

## Graphics experiments

`--graphics-dir /external/reference-graphics` stages only `d3d8.dll`,
`d3d9.dll`, `dxvk.conf`, `libMoltenVK.dylib` and `MoltenVK_icd.json` when
present. It records each SHA-256 in the external manifest and sets overrides
only for that launch. The original prefix's libraries are unchanged.
`--fps-hud` requests a DXVK FPS counter; it does not measure simulation ticks.

The [d3d8to9 v1.16.0 release](https://github.com/crosire/d3d8to9/releases/tag/v1.16.0)
was observed rendering and improved the owner's reported responsiveness, but
remained too slow. Its DLL SHA-256 was
`122928cfe225c25d30decf7184a5d37e490cecf3b58256ba3206c7e1853f8ab8`.
The older Mac D3D9 DXVK package failed device creation with the bundled
MoltenVK. A matched modern DXVK/MoltenVK pair initialized a swapchain but
then exited during startup; its use is experimental and not accepted.
Never treat a created graphics device or a black window as gameplay evidence.
