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

## WCA81 decoded trigger semantics

Camera main438090 runs 4373E0 (one record set) and, when it returns0,
437FB0 (the stage CAM rows, newest first, gated by mode-table priority at
table+mode*0x10+4). Both dispatch on record byte+3:

- `+0` mode index, `+1` sub-priority, `+2` adjust index, `+3` shape.
- shape0 sphere (4357F0): center+8..+10, SQUARED radius at+1C, strict `<`.
- shape1 gate (435840): broad `dist^2<=x^2+y^2`; history selectors1/2 moved by
  the inverse of T(center)*Rx(+4)*Ry(+6); they must straddle local z=0 and the
  788640 crossing (plane z=0) must lie within +-x(+14), +-y(+18) inclusive.
- shape2 box (435AA0): broad `dist^2<=x^2+y^2+z^2`; history selector1 in the
  same local frame within +-(+14,+18,+1C) inclusive.

Every test raises player Y by4. Broad tests use the current task position;
details use position history. All five owner Emerald Coast CAM files use only
shapes0/1/2, positive sphere radii and nonnegative extents. Portable module:
`src/game/cam_trigger.[ch]` with host tests. The parallel-gate branch
(795770/788D40), mode table contents and the camera modes themselves remain
undecoded; no native camera uses this yet.

## WCA81 camera mode table

`*0x9759C4` -> mode table0x975410, 16-byte entries: +0 mode function (copied to
active `03B2CAE4` by436040), +4 priority byte (437FB0 gating; 4 = special
region handled by437900), +5 byte copied to `03B2CADF`, +8 word, +C optional
second function. Consecutive indices share a function and differ in priority:
0-3 462E90, 4-7 469590, 16 46A760, 20-21 46A350, 31-32 46A0A0, 39 469E90,
61 51F680 (empty, priority4). `*0x9759C8` -> adjust table0x9758E0, 8-byte
entries {flag word, function}: 0 467D80, 1-2 467DC0, 3-4 468000, 5 4682B0,
6-11 469D10. Stage initializers call 4345C0(4): mode4 (469590) is the default
follow camera (about390 instructions plus ~20 helpers in466B60..468790).
Mode functions fill the parameter block at `*0x7DFF90` (462E90: target +18..+20,
distance +8, angles +24/+28/+2C); 437A20/435600 later build the view.

### Mode4 (469590) opening, decoded

First update (param counter +0x20 == 0): parameter-block target(+18..+20) =
Sonic task position. Every update: goal = task position + (0, CharObj2+0x118,
0) + motionwk world velocity(+4..+C) * 8.0 (7DDC48); target +=
(goal - target) * 0.1 (7DD568), per axis with float stores. It then branches
on 4365A0 (camera state 8..11) and 436540 (<5); the remaining ~330
instructions and helpers 466B60..468790 are not yet decoded or ported.
