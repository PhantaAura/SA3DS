# PC SADX CAM file format — validated subset, 0.0.9.1D

These are *observed byte layouts* in the owner-supplied system archives, **not** a decoded camera control algorithm.

```
Source CAM file:
  offset +0x00: u32 LE row_count
  offset +0x04: 60 bytes reserved zero (all 127 checked sources)
  offset +0x40: row_count * 0x40-byte records

Each 0x40-byte record:
  +0x00: u16 word_00 (semantic unverified)
  +0x02: u16 word_02 (semantic unverified)
  +0x04: u32 word_04 (semantic unverified; sometimes nonzero)
  +0x08: 56 bytes opaque (values often resemble vectors/floats, not yet proven)
```

Every record is stored unchanged. One original record (`CAM0701S.BIN`, index 169, row +0x20) contains 0xFFD00000; interpreting its bytes as float yields NaN, so filtering for float finiteness would lose retail data.

## SA3DCAM v1 chunk

`<8sHHIIIIIII>` header containing `SA3DCAM\0`, version 1.0, header size, source count, row count, 16-byte-aligned directory/row offsets, total size, and CRC32 over the row payload.

Directory records `<32sII32s>` contain zero-padded uppercase CAM source basename, first row offset *in records*, row count, and SHA-256 of original file. Row payload is a simple concatenation of source records in basename order. Parser enforces canonical offsets, padding, source fingerprints, contiguous row ranges and CRC32; no x86 addresses survive.

## Semantics to research later

Look up actual 2004 camera function callers and use runtime/test traces to determine flags, movement trigger volumes, priority and interpolation. `CAM` supplies camera setup data and not all camera behavior. `CAM*` filename variants are preserved rather than flattened together at runtime. The unknown-word names above must not be promoted to semantic names based on guesswork.
