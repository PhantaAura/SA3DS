# PC2004 PVM/PVRT format-9 base-image order — 0.0.9.1B

## Result

**CONFIRMED for the currently audited SA1 PC2004 PVM archives:** the largest
format-9 image is stored **first** in the PVRT texel payload.

This corrects the earlier `SONIC.PVM` helper, which selected the final
`width*height*2` bytes of a full mip chain.  That old rule mixed lower mips and
alignment bytes into the supposed base image whenever mipmaps were present.

## Exact cross-archive witness

`BEACH01.PVM` and `BEACH03.PVM` both contain `m128_cos002`:

- `BEACH01.PVM`: PVMH flags `0x010f`; base-only format-9 payload.
- `BEACH03.PVM`: PVMH flags `0x000f`; full format-9 mip chain.
- The first 128x128x16-bpp source image bytes are identical between them.
- The decoded base RGBA8 images are identical.
- Across same-name, same-dimension, same-pixel-format entries shared by these
  two archives, 41 of 52 comparable base images are byte-identical.  The other
  11 are legitimate archive-specific variants, not an ordering contradiction.

Therefore the owner-side decoder now reads the largest mip from the start of
the PVRT texel payload.

## Accepted current format-9 payload layouts

For the audited SA1 files the strict generic parser accepts only:

1. base image only;
2. base image + 16 bytes zero padding;
3. complete base-first mip pyramid + 6 bytes trailing alignment.

Anything else fails loudly rather than being guessed.

## Pixel formats currently proven

- `0`: ARGB1555
- `1`: RGB565
- `2`: ARGB4444

All six audited Emerald Coast PVMs use data format `9`.

## Scope

This is source-format evidence, not a claim that final Old-3DS texture
residency should use RGBA8.  `SA3T` remains a pointer-free neutral validation
and import representation.  A later optimization pass should transcode owner
textures offline into a PICA200-friendly native format and stream/reside them
according to Old 3DS memory limits.
