# Universal Modder SA1 resource-family notes (user supplied, 2026-10-12)

These statements summarize the **user-provided Universal Modder excerpt** as candidate investigation guidance, not freshly audited results in this checkpoint. Do not infer precise per-file internals from extensions alone.

- `PVP/GVP`: palette resources (`PVPL/GVPL` reported); many `AL_*` names concern Chao appearance. **Not geometry.**
- `PRS`: compression wrapper; decode before attributing contents. The reported root `SONIC.PRS` is grouped with Game Gear resources; `*_DC.PRS` mod packs may serve different purposes.
- `PVMX`: mod-loader texture archive; maintain material texture references and index ordering.
- `SoundData/SE/*.DAT`: sound banks (reported `COMMON_BANK00.dat` starts `V2.DMZ`); do not assume 1 file = 1 SFX.
- `ADX`: encoded audio; native 3DS backend needs its own decode/conversion process.
- `MPG`: prerendered movies, not engine-native event scripts.
- `NB` (`NJB` examples): Ninja model/motion data; not player AI.
- `.sa1mdl`, `.sa1lvl`, `.saanim`: SA Tools model, level and animation containers, not gameplay implementation. Dreamcast Conversion reportedly has numerous exported files but cannot be assumed complete.
- `BIN`: group by exact filename family; `SET` placements, `CAM` setup, `*_MES_*` message data, `PATH*` paths (unproven layout), etc. `K_SONIC.BIN` does **not** prove a complete physics definition.

**Implementation consequence:** our 1B PVM, 1C SET, and 1D CAM neutral importers remain separate. Never treat 3DS code, textures, object placements, camera setup, player physics, audio, and events as interchangeable types. The 2004 SADX bytes remain the authoritative PC reference. Any Dreamcast Conversion asset import should use an explicit modded source profile and preserve texture/material pairing.
