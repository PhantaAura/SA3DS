# WCA72 — whale CCI and category rules

The owner constructor502A9E..502AEF copies two48-byte records from010C8790,
category2, explicitly overrides broad radius to150, then sets attribute0x10 at
array+0x34. That address belongs to the SECOND record, not the first. Emergence
50235B clears the same bit. The existing actor helper already tracks this flag.

Both records are form0 spheres. First radius40, attributes0x420 (absolute
world center and damage-receive), push0x70, damage0x21. Second radius60,
attributes0, push0x70, damage0. First center is the animated shared tip copied
at502906 BEFORE enqueue41C280; tip was computed before horizontal movement.
Second raw local center is zero. Evaluator419160 therefore places it at the
current task origin regardless of root pitch/roll/drawY. Broad-phase reference
remains task origin, independently of first world center. Do not merge spheres.

Category2 rules come from original tables: receive2, inhibit32, damage64,
excluded attributes0x80000. Against ordinary category0 Sonic, both push gates
are false. Confirmed first-sphere contact can hurt Sonic; second records contact
without that attack. The source consumes preceding CCL hits before enqueue, so
publication preserves status/cache/damage. Clearing stays an explicit original
CCL-phase call. Source441AD0 character resolution and player0 world-velocity
bounce remain the actor caller's services, not new collision side effects.

Owner builder produces a pointer-free WHALE_CCL chunk with144-byte payload:
32-byte header (magicSA3WCCL, version1, count2, offset48, stride48, total144,
reserved0),16-byte config(category/receive/inhibit/damage bytes, excludedU32,
broadF32,reservedU32), then96 original CCI bytes. Strict reader copies decoded
state; package lifetime is independent afterward. Unsupported forms/local
centers/other dimensions, bad header/nonfinite values and corrupt CRC reject.
No source asset is embedded in the repository.

Private package /private/tmp/sa3ds-wca72-whale-ccl.sa3dspak:464B,
SHA25660a6f850b3fb1918df97931e568e39b17a8a7c637476c72755989540bcaf4ac6.
Eight exact owner spans and33 independently read metadata bytes match. Actual
owner profile passes two contact paths;16 malformed cases plusCRC reject.
Required host gate, full exact-owner pose/import gate and native build PASS.

Native renderer image stayed byte-identical to WCA67:471008B,
SHA256a0b63f49de98835e97242f821851fe3ef30caf7be1cd8791cc079db5aed6e7b7.
WCA72 actually launched it in Azahar2126.0 isolated Old3DS/Vulkan profile.
Menu, EC act0 terrain/objects/material-only Sonic and bounded rightward movement
rendered; player moved from(-9,5,4) to about(48.3,-0.3,-44). This is a fallback
smoke check, not lighting parity, whale execution or physical hardware evidence.
Boot log confirms Profile unavailable; material-only Sonic retained. Rejected
lighting profile remained inactive. Whale component is currently linker-discarded.
Native world/SET/task/CCL queue/services/render binding and full chase remain open.
