# WCA74 — stock initial exit root and null-channel fallback

Whale display502210 chooses motion0106BB80 in action2 and010AD448 otherwise.
The exit motion declares duration30, type3, interpolation2 and mdata pointer
03C5E3F8. That sixteen-byte descriptor is in the PE section's virtual zero-fill
tail, beyond its raw file bytes. A file-backed VA reader correctly rejects it;
reading a fabricated file offset would be wrong. The importer now checks that
the entire descriptor lies inside exactly one section's zero-fill interval.
The executable identity is pinned first.

405260 selects the angle evaluator404DC0. When its channel pointer is null,
404DD4 branches to404E26, which reads original object angles(+14/+18/+1C) and
rotates using those values. Supported root0106BB4C has zero angles/translation,
unit scale and no child or sibling. Thus the stock initial exit motion uses
zero root pitch. The non-null cycle still samples its32 pitch keys; it is not
replaced with the tip's custom29->0 interpolation. In particular29.5 blends
cycle keys29/30, whereas the collision tip blends29/0.

This is an explicit offline initial-profile result. No live original-game
memory capture ran, and indirect guest writes/modified motions remain outside
its claim. WCA66's uncertainty concerned runtime mutation, not the now-proven
null-channel fallback branch. Full retail execution proof remains open.

WHALE_TIP payload keeps its160-byte layout. Version2/profile1 certifies the
supported initial null-channel/zero-bind root in addition to32 pitch words.
Version1/profile0 remains readable for cycle/tip sampling, but the new root
selector rejects action2 on that profile. Unsupported combinations reject.
The new selector chooses source cycle pitch ordinarily and zero bind in action2.
Native caller uses this selector when wiring the actor display.

Owner package /private/tmp/sa3ds-wca74-whale-tip.sa3dspak is528B, SHA256
 afcbf95a9edcd20d883e72595211a45ac91ea62c51ea8c0f767fb1482ae312d2.
It remains private and uninstalled. Twelve source spans, PE section-tail proof,
32 raw keys,257 cycle samples and malformed/CRC cases pass. Synthetic checks
separate cycle interpolation from tip interpolation and require explicit exit
profile authorization. Required host, full exact-owner pose/import and native
build gates pass. No original data or converted payload was added to Git.

The linker-discarded helper does not change the native image:473464B,
SHA256c4d216883b97133426f09bf3e99339b83287e181b0a31525796c909fc8aa17c5,
already inspected in WCA73. No duplicate emulator check ran. Whale gameplay,
SET cold preparation/clearing, resource lifetime, effects and draw binding still
need connection before the natural chase can be assessed.
