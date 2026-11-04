# WCA58 — owner-profile Sonic lighting candidate

Native implementation is connected, with HOST and NATIVE BUILD evidence.
Raster acceptance FAILED in the WCA62 unlocked-screen inspection: the combined
WCA59 candidate boots and renders in Azahar2126 Old3DS/Vulkan, but highlights
turn most of Sonic white. This is emulator execution evidence and a visual
failure, not retail parity or hardware acceptance.
The Desktop preview remains the inspected WCA56 binary.

The EC Sonic context optionally reads assets/pc2004_lighting.sa3dspak from SD.
It uses the WCA55 raw owner profile (592B/198238851d765ba8b736bd27378d722fbde6522abbac30c5367212c2ec3ff309).
It is installed only in the private emulator SD, not the owner's shared SD or
source tree. Missing/invalid data or allocation failure retains the WCA56
material-only renderer. Pose Lab and other actors use the preserved renderer.
The existing CPU texture uploader, owner pair loader and boot diagnostics remain.

The candidate transforms original normals with the inverse transpose of the
complete actor/animated-node matrix. It computes directional Gouraud diffuse
and local-viewer halfway-vector specular separately. Both pending color streams
remain in a bounded linear arena until frame synchronization; textures are
shared. The first draw modulates texture and diffuse; a second equal-depth,
no-depth-write additive draw adds specular after texturing. Texture alpha clips
both passes; the original ordinary alpha material additionally scales the
specular contribution by alpha. The caller restores the existing shader,
attribute loaders and depth/blend state before transparent terrain rendering.
Unlit material flags disable both light contributions.

This implements the recovered ordinary Basic source choices: copied-white
COLOR1 diffuse/ambient, material-sourced specular, and original API material
byte magnitudes/power. Active act0/1 light selection follows the owner profile.
It currently supplies global ambient0, local viewer enabled and normal
normalization disabled. These are explicit candidate assumptions: Sonic's
post-display mode0 can clear global ambient, and default device state supports
the viewer/normal settings, but the first display and all inherited state/order
have NOT been measured in the retail driver. Material clamping/driver precision,
all exceptional states, alpha ordering and full visual parity remain open.
Do not treat matching mathematical examples as retail rendering evidence.

## Evidence

Required cumulative host gate PASS, including directional front/back lighting,
asymmetric diffuse RGB/alpha, halfway-vector specular, raw material clamp,
explicit normal normalization, and rotated/nonuniform actor-node inverse
transpose cases. Owner API oracle and26 exact spans PASS. An independent raw
CHRMODELS audit now compares4525 converted position/normal pairs as exact bytes,
not separate unordered position and normal sets; stage22237 positions and480
instance transforms remain exact. No importer or pose sampling changed, so the
unchanged pose matrix was not rerun.

Final native build PASS,470776B SHA256
 d16e50794c5c33cf065536f8f4a861527163a8de56221d877add11756b10f96b.
New code introduces no compiler warnings; existing inactive uploader and
movement/ring maybe-uninitialized warnings remain. Extra vertex math, memory
and draw cost still require native/physical Old3DS inspection. The owner's
movement-freeze report remains unresolved by this work.

Mathematical guidance (Direct3D9 documentation, not evidence of the exact D3D8
driver): [diffuse](https://learn.microsoft.com/en-us/windows/win32/direct3d9/diffuse-lighting),
[ambient](https://learn.microsoft.com/en-us/windows/win32/direct3d9/ambient-lighting),
[specular](https://learn.microsoft.com/en-us/windows/win32/direct3d9/specular-lighting).
[Microsoft D3DLIGHT8 layout](https://learn.microsoft.com/en-us/previous-versions/ms886261(v=msdn.10))
corroborates the API snapshot. Exact source boundaries are recorded in
[the source contract](PC2004_CHARACTER_LIGHT_STATE.md).

WCA62 inspected combined binary470768B SHA256
013d05779a47d8d3171e8e3e9ea737356fa48e60808ea7fdc0dda8d9948be642.
Boot diagnostic confirms SONIC_LIGHTING active. A300-frame opening-region
window took21496ms,1194ticks, maxSubmit457ms, minLinear25510656B,
maxCmd.415649, mean12406 terrain triangles/98draws. This is emulator-only
performance and does not prove the owner movement-freeze fix.

Next: resolve driver material consumption and inherited state against the PC
reference before changing brightness constants. Moonlight now reports the PC
offline; no new PC screenshot comparison is claimed. Missing-profile fallback
inspection remains open. Keep this visually failed candidate separate from
the inspected Desktop preview. Full
CP1–CP9, retail trajectories and physical Old3DS acceptance remain incomplete.
