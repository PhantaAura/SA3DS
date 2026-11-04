# WCA64 — original whale vertex and normal morph banks

`whale_shape.[ch]` loads and samples both original owner-generated shape banks:
cycle(duration32, nine position and nine normal keys) and exit(duration30,
seven position and seven normal keys). Each key contains238 source-order XYZ
vectors. Thirty unique arrays back32 key records because exit reuses its
terminal arrays. This is not a rigid substitute or the ambient dolphin model.

Source405EF0 wraps the nonnegative display frame at the selected motion duration.
405550 chooses the last key whose index is at/below truncated frame.4054C0
computes the key blend with a stored float weight. Importantly, at a last-key
wrap it converts the negative key-frame difference to an unsigned magnitude
before the float store: the cycle's final segment therefore has a huge blend
denominator. The sampler retains that quirk rather than substituting a normal
cyclic interpolation interval. Exit's terminal key at30 supplies the preceding
segment; exact duration30 is wrapped to0 before the key search.

405630 blends positions.405590 blends normals, stores X/Y, retains unrounded Z
for the length test and then reloads all stored components for final scaling.
Length squared must exceed the source float1e-5; smaller/opposed results become
(0,0,1). No normal-renormalization rule is invented from a different engine.
C double approximates bounded x87 expressions with recovered float stores;
full extended-precision word identity is still unproven.

The pointer-free WHALE_SHAPE package is generated only from the exact owner's
executable by build_pc2004_whale_shape.py. Seven code-span witnesses constrain
bank display/selection/blending and the source threshold/dispatch are checked.
The loader validates format, bounded counts/offsets, ordered keys and finite
vectors before exposing borrowed views. Caller retains package lifetime.
The private package86384B SHA256
45eadac82738e2b7912b070b984220df451c03b97989b796642830895cb74e79
is /private/tmp/sa3ds-wca64-whale-shape.sa3dspak, outside Git and shared SD.

## Evidence and limits

Required cumulative host, exact-owner pose/import gate and native build PASS.
Synthetic fixtures check blend/cancellation/normalization, unsigned final-key
interval, duration wrap and rejected frame/bank/capacity. Independent owner
oracle reads source headers/key pointers directly, compares all32 loaded raw
key arrays byte-for-byte, then compares711144 sampled XYZ components across
both banks at eighth-frame intervals including duration wrap; maximum numeric
error against that raw-owner mathematical oracle is0. Twelve malformed package
cases plus CRC corruption are rejected. This is importer/equation evidence,
not a retail-game replay or proof of every x87 rounding boundary.

Native helper is not yet called and is linker-discarded. Artifact stays exactly
470768B/013d05779a47d8d3171e8e3e9ea737356fa48e60808ea7fdc0dda8d9948be642;
no unchanged emulator replay was run. That artifact's optional Sonic lighting
already failed WCA62 raster acceptance and its private profile is inactive.
Desktop preview remains the inspected WCA56 binary.

Next: import original Basic geometry/material/texture with explicit source
vertex identity through UV splitting. Dynamic GPU vertices must sample the
source-order shape arrays without losing seam identity, preserve the CPU
texture-upload path and synchronize pending vertex storage. Then connect the
original root-motion/actor transforms, shared lifetime, phase-effects ordering,
history/CCL, triggers, pier and camera. CP6 and full CP1–CP9 remain incomplete.
