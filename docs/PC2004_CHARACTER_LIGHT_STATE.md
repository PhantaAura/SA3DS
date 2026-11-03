# WCA55 — original character light and material API state

The current native renderer has no original character lighting. This checkpoint
adds owner profile extraction and portable API-state builders, without changing
native shading. The exact executable imports d3d8.dll.40A630 constructs a104-byte
directional light;40AA30/40AAB0 select character profiles. Sonic's ordinary
4948C0 display chooses mode2, selecting EC level1/type0. Both Sonic acts resolve
to table row3 (act1 falls back to act0): own direction(0,-1,-.033), diffuse scalar
1.001999974, specularRGB.805 and ambientRGB.75 (corrected by WCA57 below). Direction is not normalized by
this callback. Other rendering/material-source/normal-transform stages remain.

The API builder retains diffuse alpha1, specular/ambient alpha0, zero position/
range/attenuation/spot fields and source enabled-light mask. Own/stage direction,
reverse direction and optional light3 reverse-fill branches are represented for
ordinary EC. Ice Cap/Casino-specific branches and event light overrides are not
included. The packaged stage record is the40A8A0 initial default, not a claimed
snapshot of all live stage lights; normal EC character profile uses its own
light direction. Both selected character records retain all96 bytes.

784850 converts resolved packed material colors into a68-byte API material.
Its byte components become floats0..255 without division by255; diffuse is also
copied to ambient, specular is retained, emissive is zero and source power is
copied. These are API arguments. COLORVERTEX/material-source/FVF selection must
be resolved before deriving final pixel colors; applying a generic normalized
material formula would not reproduce this boundary. The portable builder takes
already-resolved diffuse and does not guess constant-material blend handling.
API layout can also be compared with the primary SwiftShader D3D8 headers:
https://swiftshader.googlesource.com/SwiftShader/+/498499b4f158bb3c4fc0c96da1053a6933faf8ac/include/Direct3D/d3d8types.h

## Verification

Exact-owner7-span witnesses PASS. Required cumulative host gate and exact-owner
pose/import gate PASS. The raw-owner oracle checks both acts/all104 light bytes,
version/NaN/profile rejection and CRC failure. Focused checks cover source API
layout, own/stage/reverse/fill direction, raw material byte range/channel order
and act bounds. These prove state construction, not execution by the PC driver.

Private owner package592 bytes SHA256
198238851d765ba8b736bd27378d722fbde6522abbac30c5367212c2ec3ff309,
/private/tmp/sa3ds-wca55-lighting.sa3dspak; it is not installed or committed.
Native build PASS; unreferenced helpers are linker-discarded and the native
artifact remains WCA54's462612-byte d78093b65f4e16b8f94fd38649ceae0b684dff033055f0a90c83a937cf6dd4e3.
No redundant emulator replay ran. Source material/light values are not baked
into redistributed assets or reference code. CPU upload/SD/boot paths unchanged.

## WCA56 — native ordinary Sonic constant diffuse

Source4948C0 sets draw-control10 and constant white via4128A0/402F40/77FF90.
403470->77EF70 invokes781CC0, which copies the constant color and sets work bit2;
784850 then resolves diffuse from that copied constant. The previous native Sonic path
multiplied by each mesh's stored diffuse, including grayB2 and blueB1. The ordinary
Emerald Coast Sonic renderer now explicitly binds the copied white diffuse;
Pose Lab and unrelated actors retain the stored-material default. The texture
uploader, model emitter, shader, scale and animation are unchanged. The source
witness now checks17 spans across the display/action/constant-copy chain.
Required host gate and native build PASS. The integration guard permits only
this resolved-material branch and still compares the remaining mesh emitter
against the preserved baseline. Native raster inspection is recorded below.

Recover remaining material-source/FVF/normal/view and specular-composition rules
before enabling native character lighting. Full WCA,
retail lighting/animation trajectories and physical Old3DS acceptance remain open.


WCA56 exact462596-byte artifact SHA256
78f5506a69818cfa4570bbb5c587020ba9b5c3f49dcccc1e73bda753989f8429
booted in Azahar2126 Old3DS/Vulkan with owner SD packages. Boot diagnostics
confirmed SONIC_MATERIAL binding. Rear/side camera raster inspection showed
white gloves/shoes and textured Sonic; Sonic's body remains flat without
directional/specular shading. No retail pixel, trajectory or physical Old3DS
claim. Temporary private EC autoboot flag removed after inspection.


## WCA57 — corrected light fields and Basic vertex boundary

WCA55 swapped the selected profile's ambient and specular offsets in both the
portable builder and its oracle. Its prior all-byte PASS did not catch this
shared interpretation error. WCA57 corrects the builder and oracle:40A653/65D/
665 write profile+20/24/28 to light+36/40/44 (ambient .75), whereas40A677/6AF/
6B5 write profile+52/56/60 to light+20/24/28 (specular .805). Explicit instruction
store checks now constrain this mapping as well as whole-function digests.
The external package contains raw profiles and does not change. The builder
remains unconnected, so no existing native raster used the swapped fields.

The exact normal+UV/no-mesh-color Basic path is now witnessed:784AE0 dispatch5
at389D710 selects389D6E0, containing triangle783830, quad783920 and strip7835F0.
Each writes XYZ at0, normal at12, resolved packed diffuse at24 and UV at28/32.
The buffer-init table's ninth entry is36-byte stride/FVF152 (XYZ,NORMAL,DIFFUSE,
TEX1; no separate SPECULAR).794600 binds that FVF and stream stride. Diffuse is
therefore supplied by the resolved per-vertex color, not absent from the stream.
781CC0 selects COLOR1 for diffuse/specular;791860 restores material sources.
These are static API/vertex contracts, not a measured PC driver output.

Remaining shading work includes complete inherited global ambient/COLORVERTEX/
LOCALVIEWER/NORMALIZENORMALS state, model/view normal transforms, driver clamping,
specular composition after texturing, and native raster comparison.412420 mode0
can set global ambient0/normalization1, while device setup796710 initializes a
different global ambient; the actual ordinary gameplay ordering must be traced.
Do not substitute an assumed brightness multiplier. Full retail/hardware parity
and CP1–CP9 remain incomplete.
