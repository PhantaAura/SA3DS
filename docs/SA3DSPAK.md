# `.sa3dspak` version 1

All integers are unsigned little-endian. Files begin with a 48-byte header (`SA3DSPAK`, major/minor version, header/file sizes, TOC offset/count, flags, payload CRC-32, header CRC-32). Each 40-byte TOC entry contains a 16-byte NUL-padded type, 64-bit file offset and size, CRC-32, and flags. Chunks are 16-byte aligned.

Defined v1 concepts are `GEOMETRY`, `COLLISION`, `MATERIALS`, `TEXTURES`, `SET`, `CAMERA`, `AUDIO`, and `METADATA`. Their payload schemas are intentionally not invented here. Metadata records the source profile. Packages never contain host pointers, VAs, or RVAs.

The host and runtime validators check version, file size, count limits, all offset/size arithmetic, header/payload/chunk integrity, and reject malformed input before exposing a chunk. The runtime reader is allocation-free and returns views into a caller-owned buffer, allowing later streaming and Old 3DS budgeting.
