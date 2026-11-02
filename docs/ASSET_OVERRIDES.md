# Native SD texture overrides

CP9's first supported category is one texture replacement for one Sonic Emerald
Coast act. It replaces pixels only: original material IDs, geometry, movement,
collision, animation and object placement are unchanged. No Windows DLL is run.
Default assets stay active unless the owner explicitly enables a valid override.

## Install and switch

Place the owner-generated file at:

- `sdmc:/3ds/SA3DS/mods/emerald_coast.sa3mod`
- `sdmc:/3ds/SA3DS/mods/emerald_coast.enabled` containing `1` to opt in.

With the enabled marker absent, empty or different from `1`, boot uses the
original assets. In Emerald Coast, **SELECT + D-pad Up** enables/disables the
replacement and persists the requested setting. This does not rebuild or restart
the game. Disabling frees the extra replacement texture and restores the already
resident original. Enable/reload synchronizes the previous GPU frame, validates
the bounded file and uploads through the existing CPU texture path. Loading time
is excluded from the fixed-step clock; ordinary simulation still runs at60Hz.

Missing, truncated, corrupt, wrong-act, wrong-archive, wrong-index and mismatched
size files fall back to the original texture. The main runtime clears an earlier
override before attempting a reload. One file targets one act; entering the
other act restores originals when the file's act/hash does not match. This is a
small asset override foundation, not a DLL mod loader or a full mod manager.

## Convert an existing local Dreamcast texture

The importer reads an owner's installed Dreamcast Conversion archive or an
uncompressed local Dreamcast PVM, finds the same texture name in the original
PC2004 PVM, and ties the result to the native act package's exact original
texture archive hash. Outputs contain game-derived pixels and must remain
outside the source checkout. Do not commit the output, decoded texture images,
original files or private validation logs.

Example for the installed local Dreamcast Conversion by PkR:

```sh
python3 tools/build_dc_ec_texture_override.py \
  --source '/Users/emeraldhunter/Library/Application Support/Steam/steamapps/common/Sonic Adventure DX/mods/DreamcastConversion/system/BEACH01_DC.PVM.PRS' \
  --pc-pvm '/Users/emeraldhunter/Library/Application Support/Steam/steamapps/common/Sonic Adventure DX/system/BEACH01.PVM' \
  --baseline-package '/private/tmp/sa3ds-azahar-wca/user/sdmc/3ds/SA3DS/assets/emerald_coast_act0.sa3dspak' \
  --act 0 --name m128_cos002 \
  --output /private/tmp/emerald_coast.sa3mod \
  --report /private/tmp/emerald_coast-override-report.json
```

Copy that generated `.sa3mod` to the SD `mods` directory, then opt in with the
marker or runtime chord. Use the actual native act package for your SD install;
wrong source archives are rejected. The importer refuses to write pixel packages
or owner reports inside this code tree. It reads files locally; it does not
download assets or execute the installed mod's DLL.

The decoder currently supports the installed Dreamcast PVM directory profile
`0x0109` and square uncompressed twiddled PVRT formats1/2, with ARGB1555, RGB565
or ARGB4444 pixels. Format2 uses its original smallest-first mip chain,2-byte
prefix and original X-high Morton axis order. VQ/indexed encodings fail clearly;
they are not guessed or silently reinterpreted as PC2004 format9. Native
replacement dimensions must match the original and be powers of two from8 to256.

Primary format references:
[PuyoTools PVR texture documentation](https://github.com/nickworonekin/puyotools/wiki/PVR-Texture),
[PuyoTools original square/mipmap decoder](https://github.com/nickworonekin/puyotools/blob/master/src/PuyoTools.Core/Textures/Pvr/DataCodecs/SquareTwiddledMipmapsDataCodec.cs).
The local archive supplies directory/padding witnesses. Existing PC2004 decoding
is kept separate from the Dreamcast decoder.

## Format and runtime bounds

`.sa3mod` is little endian,64-byte header then a single-texture SA3T payload:

| Offset | Type | Meaning |
|---|---|---|
| 0 | 8 bytes | `SA3DMOD\0` |
| 8 /10 | uint16 | version1 /0 |
| 12 | uint32 | header size64 |
| 16 | uint32 | exact total file size |
| 20 | uint32 | Sonic act0 or1 |
| 24 | uint32 | local original texture index |
| 28 | uint32 | CRC32 of the entire file, with these4 bytes treated as zero |
| 32 | 32 bytes | expected original texture-archive SHA256 |
| 64 | SA3T | exactly one RGBA8 texture |

The maximum file size is262304 bytes. Opening a view performs no allocation.
The file's act, original archive hash, target index, dimensions, CRC and SA3T
bounds must all pass. File bytes live only until CPU upload completes. The GPU
retains original texture storage and allocates at most one additional256KiB
RGBA8 texture while enabled. Disabled mode has no override file buffer or extra
texture allocation, only small fixed diagnostic fields.

GPU set stages an upload before replacing an existing override. Upload failure
leaves that earlier GPU override intact; the product reload path explicitly
clears first so invalid/missing reloads always use original assets. Set/clear
must run after frame synchronization. `override_submitted_draws` counts actual
draw submissions using the replacement; binding a culled texture alone does not
increment it. It resets with the per-frame submission counters and on clear.

## Verification and limitations

Owner-free gates:

```sh
bash tests/run_texture_override.sh
bash tests/run_emerald_coast_gpu_runtime.sh
```

These test PRS literals/overlapping back-references/truncation/caps, synthetic
original Dreamcast mip/axis decode, header/CRC/act/hash/index/dimension rejection,
zero-view fallback and baseline immutability. The mocked SDK test runs the actual
native GPU implementation: original versus replacement binding, actual override
submission count, atomic upload failure, clear and complete texture cleanup.
It is not a rasterization or hardware test.

A real local conversion was read by the same C runtime reader: act0 PC texture0
`m128_cos002`128x128 from Dreamcast texture12 in `BEACH01_DC.PVM.PRS`. The local
archive's source hash is
`4ff783da89c18313a04d4264ff7551d3aec64d19ed561b0e89bc119b90a07dd1`;
the generated65696-byte override hash is
`8b1b7135de37fe83cb3121a90e0f37a489ec50236558b3baa3bd48cfb92e9460`.
The converted pixels differ from the PC image in30621 RGBA components (after correcting the PC2004 linear format9 decoder). That
proves a real different owner-source replacement exists, not native visual
acceptance. Exact native enable/disable submission and visual checks are recorded
in the unified WCA checkpoint document when actually performed. Hardware testing
is separate; no hardware result is implied here.


## Native owner replacement demonstration November 2

Identical315864-byte native build SHA256
b06f7ec7053651437da4dbe21bd0c6d67512883fe92d61f89c5688f3c9f7eeb4
was run with corrected PC2004 baseline caches in the isolated Azahar profile.
Default/disabled: original selected, modDraws0/modBytes0. Enabled by SD flag:
real converted Dreamcast package8b1b7135... was accepted and actual draw
submissions used the replacement; modBytes65536 and free linear memory decreased
by exactly65536. Enabled with the package absent: explicit missing-replacement
fallback, modDraws0/modBytes0, original memory budget restored. Each bounded
opening replay reached tick270 without a draw failure. No rebuild occurred
between configurations. Original and enabled previews were visually inspected;
this proves SD replacement operation, not pixel-identical Dreamcast raster output.

Counters accumulate between view resets in the opening subset, so the logged
modDraws values there must not be described as per-frame totals. Full-act view
updates reset them each frame. The SELECT+D-padUp shortcut remains unverified
through actual HID; SD enable/disable across restart is the verified route.
Malformed/CRC mismatch rejection and atomic upload failure are covered by focused
host reader/actual-GPU mock checks, not claimed as additional native runs.
The verified owner package is restored and default disabled after the check.
