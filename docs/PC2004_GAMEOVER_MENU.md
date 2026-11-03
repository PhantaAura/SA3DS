# WCA53 — original menu rules, draw commands and private art

Portable416610 states18/19 now retain initialization/selection0/timer1200,
confirm-maskC priority over timeout and toggle-mask300C0, one XOR toggle for
any matching bits,1200 draw updates and timeout on the next call. Results expose
source cleanup/texture-release/music1A requests, retry/lives-reset or quit.
These are service requests, not native stage-entry executors.

42BFD0 yields nine commands: full-screen texture0, title1, two colored4 layers,
two3 borders, selected2 overlay and half-size5/6 labels. Coordinates, reciprocal
depth inputs, scales and ARGB tint are retained. Pulse reads the previous signed8
value, uses absolute/128, then stores(old+4)&FC. Selected base cycles red/blue;
unselected base is blue. Overlay red=236+19*f, green=152+83*f, blue0. Actual
texture raster/blending/fog/global render state still require native comparison.

Original77FD80 uses PVMH count and77F3F0 global indices, then sends each PVRT
through77FBD0/77F910/77F2F0.77F2F0 reads pixel format and width/height from PVRT.
The English GAMEOVER_E PVMH pixel0 entries2..6 disagree with actual PVRT pixel2.
The opt-in pixel_format_from_pvrt profile preserves original data/hash and reads
that authoritative format; strict default behavior is unchanged. This bounded
profile still requires matching dimensions/data format and supported encoding.
It is not a general reconstruction of every PVM loader feature.

The converter emits an external seven-image SA3DSPAK. Private English package:
263024 bytes, SHA2561486b3937f596af3bd21bd6109a9a79a7228a52c9d894809d4d334c23742f69c,
/private/tmp/sa3ds-wca53-gameover.sa3dspak. Owner pixels remain outside Git.

Focused menu checks PASS timing/priority/toggle/cleanup and all256 pulse values
with both selections. Owner assets PASS seven direct raw-pixel oracles, default
rejection, explicit authority and malformed size/unsupported-format rejection.
Exact-owner6-span witnesses PASS. Required host and exact-owner pose/import gates
PASS. Native build PASS, but helpers are unreferenced:457836-byte artifact remains
WCA52 hash4accef903822139953ae2568750f9a748f3425bfe26f166469526e3768e76ed1.
No redundant emulator replay ran. Native screen/control/timeout, stage retry/quit
handoff, audio, retail/full-course and hardware acceptance remain open.
