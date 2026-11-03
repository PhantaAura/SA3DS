# WCA45 — correct TEV material channel packing

The source SA3C material diffuse word is AARRGGBB. The shared native Sonic and
Emerald Coast material helper previously rotated that word into RRGGBBAA and
passed it to C3D_TexEnvColor. The TEV constant register actually takes R at bit0,
G8, B16 and A24, meaning the integer representation must be AABBGGRR.

Evidence:
- Installed devkitPro c3d/texenv.h assigns the color word without conversion.
- Citro3D writes the environment words directly to PICA registers:
  https://github.com/devkitPro/citro3d/blob/master/source/texenv.c
- Azahar's TevStageConfig declares const_r0/const_g8/const_b16/const_a24:
  https://github.com/azahar-emu/azahar/blob/master/src/video_core/pica/regs_texturing.h
- WCA43's opaque fade already confirmed high-byte alpha in an actual raster.

The helper now preserves alpha and green while exchanging red/blue positions.
For example, source opaque gray FFB2B2B2 formerly became B2B2B2FF: the register
read red255, green178, blue178, alpha178. It now reads gray178 and alpha255.
Source black remains black; transparency is retained. This only changes the
material constant, not the baseline CPU RGBA8 texture upload/swizzle path.
The existing shared helper also covers ring and movement-object terrain draws.

Required cumulative host gate PASS, including direct register-channel decoding
across all256 alpha/channel values, primary colors, opaque black/gray and
transparent white. Prior packing fails these checks. No pose or owner importer
changed, so the unchanged owner pose matrix was not rerun.

Native build via tools/build_3ds.sh PASS. Exact final440076-byte code-only
artifact SHA256:
1d8b24d8e4d9d069668ec8f57d81ae4f71d53da768f84e5e5824c19fda960732.
That artifact booted in Azahar2126.0 Old3DS/Vulkan with the external owner
packages. Actual raster inspection confirms sand and white structures no longer
have the previous pink cast. Sonic renders with white shoe/glove areas rather
than pink contamination. His lighting remains visibly incomplete; this check
is not a retail lighting/animation/full visual parity or hardware claim.
No owner data/build enters Git. Boot diagnostics/SD owner pair/CPU uploader
remain intact. Full WCA and physical Old3DS acceptance remain open.
