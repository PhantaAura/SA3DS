# WCA55 — original character light and material API state

The current native renderer has no original character lighting. This checkpoint
adds owner profile extraction and portable API-state builders, without changing
native shading. The exact executable imports d3d8.dll.40A630 constructs a104-byte
directional light;40AA30/40AAB0 select character profiles. Sonic's ordinary
4948C0 display chooses mode2, selecting EC level1/type0. Both Sonic acts resolve
to table row3 (act1 falls back to act0): own direction(0,-1,-.033), diffuse scalar
1.001999974, specularRGB.75 and ambientRGB.805. Direction is not normalized by
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

## Next native correction

Source4948C0 sets draw-control10 and constant white via4128A0/402F40/77FF90.
403470->77EF70 invokes781CC0, which copies the constant color and sets work bit2;
784850 then resolves diffuse from that copied constant. The native Sonic path
currently multiplies by each mesh's stored diffuse, including grayB2 and blueB1,
which is a distinct pending source-state mismatch. Verify the exact chain and
wire only the supported ordinary Sonic profile; keep Pose Lab and unrelated
actors explicit. Then recover the remaining material-source/FVF/normal/view and
specular-composition rules before enabling native character lighting. Full WCA,
retail lighting/animation trajectories and physical Old3DS acceptance remain open.
