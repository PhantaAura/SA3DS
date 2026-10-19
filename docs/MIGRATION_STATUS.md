# SA1 2004 PC to 3DS migration status

## Fixed source of truth

- Gameplay/reference version: the owner's exact Steam-converted 2004 PC SADX installation and exact `sonic.exe`/Ghidra export fingerprints recorded in `MODDING_PLAN.md`.
- Reference use is private. Distributed output must contain original clean-room port code and conversion tools only.

## Evidence levels

| Area | State | Evidence |
| --- | --- | --- |
| 3DS boot/Citro3D shell | implemented, previously seen on Old 3DS | inherited Pass10A/WIP9 logs and owner hardware metrics |
| External owner asset pair | implemented and host-tested | atomic SD loader tests and owner visual preflight |
| Sonic model/texture decode | implemented and host-tested | exact source hashes, deterministic SA3C/SA3T builders |
| Sonic hierarchy/pose | implemented, host-tested, prior hardware metrics | Pass11A hierarchy audit and Pass11B independent source-motion oracle |
| White Zone 60 Hz movement | implemented and host-tested | deterministic movement/physics suites |
| Retail PC visual parity | unverified | requires reference captures and Old 3DS comparison |
| Emerald Coast playable stage | partial reconstruction only | collision/visual tools exist; not integrated as a complete hardware-tested stage |
| Story, objects, enemies, audio, UI, saves | missing | future milestones |

## Known deviations

- The current `.3dsx` is a White Zone/Pose Lab vertical slice, not the full game.
- Owner data is converted to SA3C/SA3T rather than loading Windows PE/PVM data on-console.
- PC mod-loader compatibility and Dreamcast/DX asset switching are not implemented.

## Exact next task

Run the Pass11B-audited SD build with the existing owner asset pair on Old 3DS, capture both screens and logs, and use that evidence to decide whether the next code unit is a presentation correction or Emerald Coast integration.
