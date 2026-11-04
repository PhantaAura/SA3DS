# WCA63 — original whale animated collision tip

`whale_tip.[ch]` implements50264A..502754, using an external owner-generated
32-key pitch bank. It samples BEFORE WCA62 common horizontal movement. Original
local point(0,-5,120) is transformed by yaw then sampled X pitch, translated by
actor position/drawY, and hidden at(0,-100000,0) during emergence. Visible body
roll is not substituted for this collision pitch.

The dispatcher floors the frame, blends adjacent raw signed pitch keys and
truncates the result toward zero. For floor29 the next key is0. Exact frame30
instead uses key30/31; it must not be wrapped to0 or rejected as an invalid30-key
track. The original table actually has32 sequential indexed rows. Supported
phase frames are0..30 inclusive; unsupported out-of-range frames fail explicitly.
Ninja Y/X rotations retain observed float matrix stores. Caller sine service
must use the owner lookup table. C double does not prove all x87 intermediate
rounding, including values extremely close to integer truncation boundaries.
Full retail numerical parity remains open.

`build_pc2004_whale_tip.py` checks exact executable identity and four source/data
spans, then writes a pointer-free WHALE_TIP package outside this repository.
Loader validates CRC/size/version/count/stride/offset/reserved/flags and signed
BAMS limits. The view borrows bytes; caller retains package lifetime. No source
key values or converted assets are shipped in Git. Private package464B SHA256
5924c8bfca2fa099ec2a5e86c02f66bee82993c5b6a49fa8ef9fedf3d793ee28
is at /private/tmp/sa3ds-wca63-whale-tip.sa3dspak, not installed to shared SD.

Required cumulative host, exact-owner pose/import gate and native build PASS.
Focused checks cover Y/X rotation/translation, negative truncation,29->0 blend,
exact30, emergence and unsupported-frame rejection. Independent owner audit
compares all32 loaded signed words against separately addressed source rows and
rejects eight structurally malformed packages plus CRC corruption. These checks
prove bounded equations/import, not execution in the original game.

Helper remains unreferenced/linker-discarded. Native artifact is unchanged
470768B SHA256013d05779a47d8d3171e8e3e9ea737356fa48e60808ea7fdc0dda8d9948be642;
therefore no redundant emulator replay. The optional lighting in this artifact
already failed raster acceptance; Desktop preview remains inspected WCA56.

## Next native whale boundary

Owner shape inventory confirms one238-vertex Basic model. The cycle has nine
position and nine normal keys; exit has seven each, with a reused terminal array.
Both channels are finite238-vector arrays. Shape type0x30 selects405AA0 through
900884. The source normal blending helper405590 includes normalization and a
small-length fallback; position blending405630 is separate. These observations
constrain the next importer/sampler; no visible shape renderer is implemented
or accepted yet. Native shared lifetime, phase-effects ordering, history/CCL,
wave/audio, pier/camera, triggers and full-course chase remain incomplete.
