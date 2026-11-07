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
on 4365A0 (current mode index; 8..11 only) and 436540 (frames since mode
set): modes 8-11 place the eye at Sonic + (0,0,-47.5) rotated by pitch F1C8 /
yaw 4000-SonicYaw (465F00) for 5 frames, then sidestep 0.2 on the camera yaw
circle. Default mode4 skips that. Shared body (parameter block +0C..+14 = eye):

- pitch: if |camRotX - F1C8| > 71C, approach by B6 (438350) and lift eye.
- manual L/R analog (03B0E808/80A > 0x80 after <<8): eye moves 2 units on the
  yaw circle, then 466B60; skipped for level9 act3.
- distance d=|Sonic-eye|, unit dir: d>60 -> (d-60)*.9+60, if >90 a camera ray
  test 4370C0 (43A060, flags400002, radius4) picks 20 when the hit is within3
  else 90; 35<=d<=60 kept; d<35 -> d*1.06 with minimum20. eye = Sonic-dir*d.
- if eye above Sonic and horizontal distance^2 < 900: push horizontally to30.
- 437E90/437F20: priority-4 (mode61) CAM regions push the eye: sphere 4367B0
  (inside +1C -> radius +14), box 436AD0 (to least-penetration face), gate
  436850 (crossing keeps the previous eye, 436560(0)).
- 462B40: accept the new eye unless the ray test from the old eye (radius4 or
  8 by which is nearer Sonic) hits; on hit use the clipped delta, or with
  manual input move only 10%. 4625E0 only acts in level6 act2.

Constants: 60 (7E09A8), .9, 90, 35, 1.06, 20, 3, 900, 30, .2, 2, .1, 8.
Remaining before a faithful port: the camera ray test 43A060, 466B60, the
view build 437A20/435600 and the adjust table functions (467D80...).

### Camera ray test chain

4370C0 -> 43A060 -> 439620(start, delta in/out, result, mask400002, radius).
450C10 builds the sweep work (start, delta, radius); then for each entry of
the active collision list 03B2F720 (12-byte {flags, object, task}, count
03B32724) whose flags miss the mask, 453820 sweeps the sphere against that
object; moving objects add their motion. The nearest hit (by distance from the
start) wins and is written back as clipped position/delta; the return value
is nonzero on any hit. 453820 (about394 decompiled lines, helpers 450810,
450920, 4509D0) is the original polygon sphere-sweep, the same solver the
player collision notes record as missing. Porting it is the next step for
both the mode4 camera and retail terrain contact.

### 453820 port (WCA82)

`src/game/pc2004_sphere_sweep.[ch]` ports the per-polygon sweep: 1.04 inflation
about the centroid, 450810 normal with 4509D0 zero->(0,1,0), 4B8E80 angles
(asin(z), atan2(x,y)*-1/2pi, with +-0.99996 pole and +-0.002 tiny-XY cases),
rotX(-ax) then rotZ(-az), samples every radius*.5 with the final sample at
start+delta, +-radius slab and XZ cross-sign containment, 0x800000 double
sided flip, and the smallest-end-height choice over up to16 hits. Hit outputs:
plane point, corrected start (signed radius above the plane), delta split.
Not ported: the hit record's 645270/645034 slope angles and the 450C10 broad
phase over the original z-sorted polygon lists (the native grid supplies a
superset instead). On owner EC terrain, 76/84 downward sweeps land within0.6
of the native vertical floor (normal projection and inflation explain the
rest). Next: 439620's clipped position/delta write-back for the camera.
