# WCA54 — native original game-over screen and stage handoff

The runtime now consumes the external seven-image menu package on entering
state18, runs original menu selection/timing requests, and submits the nine
source draw commands. No menu allocation or file read occurs during ordinary
course play. Package CRC, chunk identity, SA3T bounds, count and native texture
limits are checked before allocation. Original pixels stay outside Git.

D-pad edges map to the source toggle bits; A and Start map to accept-maskC.
Edges survive render updates without simulation ticks and are consumed once.
Start no longer exits before menu confirmation. Circle Pad virtual menu bits
are not connected yet; no guessed analog threshold is introduced. Source music1A
is requested/logged but not played; the audio executor remains incomplete.

Continue restores fresh lives4, clears the prior restart record, resets the
current native EC profile to its initial act0 and enters original fresh stage0/1.
Fresh fade initialization/reload takes6 updates and state4 takes49 world updates;
it does not increment the death restart counter. The initial player record is
seeded after a fresh spawn, with dead state clear. This uses the inherited native
player reset and current collision adapter, not recovered full player initialization.
This executable hosts an EC stage-only profile: Quit/timeout leaves that host
for its launcher, rather than recreating the retail front end. Other launch
profiles/Trial entry and complete remembered-stage/global task parity remain open.

Menu geometry uses tilted orthographic projection for the rotated3DS target.
Known nonintersecting far-to-near rectangles use source call order and ordinary
alpha blending; this is not a replacement for the general PC sprite/depth pipeline.
Every tint update obtains C3D_GetTexEnv(0) so Citro3D marks it dirty before the
next draw. Pixels use the preserved CPU upload path and linear filtering. The
full640x480 command space maps onto the native top screen. Texture resources
are released after the next synchronized frame retires prior menu draws; final
cleanup also fences them. World submissions stop while the menu owns the screen.

## Evidence

Required cumulative host gate PASS for integration/fresh fade behavior. The later
raster-only corrections were rebuilt and checked through actual native screenshots;
final lexical/HUD, BootTrace1 and native visual reintegration guards also PASS.
No import/pose data changed after WCA53, so its unchanged pose matrix was not rerun.
Final native build PASS:462612 bytes, SHA256
 d78093b65f4e16b8f94fd38649ceae0b684dff033055f0a90c83a937cf6dd4e3.
Existing uploader/placement-reader warnings remain; compiler also reports the
existing ring_runtime placement maybe-uninitialized warning. No build warning
is treated as retail or hardware evidence.

That exact artifact ran Azahar2126.0 Old3DS/Vulkan with the original Saru row28
zero-ring/lives0 fixture. Native raster inspection confirmed upright GAME OVER,
original Continue/Quit labels, tinted buttons and selection border. Real emulator
HID D-pad right/left changed the selected button; A confirmed Continue.
Log:choice=retry,timer1186,selection0; sourceFrame2324 entered0->1,lives4;
2329 loaded original act0 foot(-9,16,4),1->4;2378 returned4->15. The native
retry assertion passed fresh act0/lives4/valid initial record/cleared dead flag.
A later held-arrow attempt produced only limited displacement; it is not a
retail trajectory or sustained-HID verification and did not change physics.

On a second run of the same artifact, D-pad right/A chose Quit at timer1187,
selection1. The emulator returned to its game list after normal stage cleanup.
Timeout's1200-draw/1201st-update boundary is host-verified and shares that exit
branch; no separate native20-second timeout run was performed.

The first native candidate462092B/d44855cc showed a90-degree rotation and white
button tints. It failed raster acceptance. The final orthographic/dirty-state
corrections above were rerun on the exact new artifact. A reconstructed private
source-art reference was also inspected, not represented as an original-game
capture or native execution. The Mac lock delayed UI access until the owner
unlocked it; no screen check was claimed during that interval.

The private fixture holds after12 menu updates until real HID input to permit
inspection. That hold is only enabled by the existing private G fixture flag;
normal menu timeout/pulse is unchanged. Flags are removed after verification.
Full natural course/game-over, audio/HUD, original scheduler/camera/controller,
retail visual parity and physical Old3DS acceptance remain open.

## Owner data

Use tools/build_pc2004_gameover_assets.py OWNER_EXE OWNER_SYSTEM/GAMEOVER_E.PVM
OUTPUT to build the private package, then install it as
sdmc:/3ds/SA3DS/assets/gameover_assets.sa3dspak. The converter refuses output
inside the source tree. WCA53 package263024B/1486b393 is installed in the private
emulator and shared owner SD directories. Neither assets nor builds enter Git.
