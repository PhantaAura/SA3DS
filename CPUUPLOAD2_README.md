# SA3DS WIP9 CPUUpload2 — Old 3DS XL hardware follow-up

This is a *diagnostic alternative uploader*, not a confirmed fix. Previous
TextureSyncFix was reportedly tested but still showed black/blank screens;
the submitted log was unchanged. CPUUpload2 uses a separate log filename and
a different GPU texture-loading path, avoiding C3D_SyncDisplayTransfer for
Sonic textures entirely. This retains the original 60Hz physics and animation.

Select Y first to check Sonic texture GPU allocations without shaders/draw.
If Y passes, relaunch and press A to attempt the full White Zone. B and X
retain the original independent tests. If a path freezes, write down the
last text on the bottom screen and send /3ds/SA3DS/wip9_cpuupload2.log.

No Sega assets, extracted textures or binaries are distributed. 3dsx is built
on Mac by the provided build script and requires local owner-generated
sonic.sa3c and sonic.sa3t at SD:/3ds/SA3DS/assets/. Confirm compilation and
runtime results on hardware; host-only tests do not prove them.
