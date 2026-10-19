# SA3DS WIP9 BootTrace1 — Old 3DS startup isolation

This diagnostic is a **modified build of WIP9-MacFix2**, not a replacement physics implementation or a release-gate pass. All pre-existing White Zone source is retained. The intended build uses external owner assets on SD only.

## Why

- BootDiag1 was reported to display its screens and react to A/B on an Old 3DS XL.
- Earlier full WIP9-MacFix2 compiled and then appeared black on both screens.
- WIP9 failed to force error messages through the bottom screen's framebuffer before waiting for START.
- The original runtime also did not explicitly flush/swap the bottom console while Citro3D owns the upper display. BootTrace1 uses single buffering for bottom + bottom-only swap. This is an untested on-console correction, not a proven fix.

## Build on your Mac

Use the companion `SA3DS_WIP9_BootTrace1_MacBuild.sh`. It verifies this source ZIP, extracts it into a fresh directory, runs all inherited host gates plus BootTrace1 guards, compiles with existing devkitARM/Citro3D, and creates **SA3DS_WIP9_BootTrace1.3dsx** next to the script.

No Sonic asset files are in this release. The code-only build assumes the same locally generated `sonic.sa3c` and `sonic.sa3t` remain installed privately on the 3DS SD card:

```
sdmc:/3ds/SA3DS/assets/sonic.sa3c
sdmc:/3ds/SA3DS/assets/sonic.sa3t
```

Copy BootTrace1 `.3dsx` to your SD card as `/3ds/SA3DS_WIP9_BootTrace1.3dsx` without overwriting your existing WIP9 copy.

## Three tests, one app

On launch, a menu offers:

- **B — GPU-only**: no Sonic data required; tries Citro3D init, render-target allocation, and a *solid blue* top-screen clear. It should say `GPU-ONLY: frame submitted` on bottom. If the bottom says success but top is black, report that; success in submitting the call doesn't prove it displayed correctly.
- **X — asset-only**: initializes RomFS and runs the existing SA3C/SA3T loader and texture, geometry, and animation/ACTION preflights *without Citro3D*. A PASS means those source-level validations succeeded **on your 3DS**; it does not mean the game can render Sonic.
- **A — FULL WIP9**: initializes the actual WIP9 Citro3D White Zone runtime, with checkpoints before ROMFS, C3D, model/texture preflight, GPU texture upload, shader initialization, and first frame submission. It still has the original 60 Hz physics simulation and controls.

Run **B, then X, then A**; relaunch the app between tests if needed. START exits when the program is responsive. If a hard hang happens, note the *last visible stage number and text*, rather than holding HOME/POWER repeatedly. If the screen is black, the text file may still preserve the last checkpoint (not guaranteed if the console locks completely).

### Diagnostic log

```
/3ds/SA3DS/wip9_boottrace.log
```

This is written after each startup checkpoint; it does not modify the converted Sonic packages. The last `ENTER` without a later `OK` or `RESULT` is a clue to the failing phase, **not absolute proof** of the exact instruction that caused it. The log is append-only across launches; use the final `BEGIN WIP9 BootTrace1 app launched` to distinguish each run.

If B fails: focus on Citro3D/target. If B works and X fails: focus on source assets/parser/validation. If both pass but A fails: focus on WIP9's renderer, GPU allocations, and first Sonic draw.

## Status / limits

- Host source tests: run on this working copy.
- Real devkitARM compile: **not available in the authoring environment; run MacBuild on Mac**.
- Real 3DS BootTrace1 execution: **not yet performed**.
- Source package must contain no copyrighted Sega data.
- Physics 1:1 parity is **not** proven.
