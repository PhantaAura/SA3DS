# WCA69 — source SET unload rules and corrected whale camera reference

`set_object_clip.[ch]` reconstructs46C360 ->46C010 and the46B750 range helper.
Range is inclusive. X² and X²+Y² are checked early, then Z² is added; the sums
are not stored to float like441840's contact-length path. This is a separate
service, not the trigger sphere test. Host double preserves the recovered
operation/store boundaries but does not prove every extended-precision word.

The SET creator stores float(load distance+100) at runtime SET data+0xC via
46BCB4. For registry67/68 load160000, the unload threshold is160100. The helper
reads that runtime field when SET data exists; otherwise it uses168100. SET
flag8 and a zero threshold bypass clipping. A task is retained if the camera
or player0 is within the selected threshold. Player1 only retains it when its
character ID is0(Sonic), within a separate fixed1600 squared range. No reference
in range makes46C010 replace main with40B570 and return1. This requests deferred
unloading; the caller stops ordinary task work but does not free storage here.

## Correction: whale far deletion uses the camera

5022C0's initial read is03B2CBB0, not the player array at03B42E10. The camera
initializer438090 assigns its task work to that first address at438221. The
portable motion input is now explicitly `camera_position`; the earlier
`player_position` name was wrong. The independent chase-speed service still
reads4419C0's buffered Sonic position. Do not bind both inputs to the same live
player position. The mod-loader's primary header independently corroborates
[Camera_Data1 versus EntityData1Ptrs](https://raw.githubusercontent.com/X-Hax/sadx-mod-loader/master/SADXModLoader/include/SADXVariables.h).
Native whale consumption is not wired yet, so no player-visible fix is claimed.

Required cumulative host, focused exact-owner six-span/call/constant witness
and native build PASS. Tests cover inclusive boundaries, load+100, camera/
player0 alternatives, player1 character/range distinction, no-reference unload,
flag8/zero bypass and fallback410-unit boundary. Existing whale phase/actor
checks pass with the corrected camera input. No pose/import payload changed;
the unchanged owner pose matrix was not rerun. The owner gate now includes the
static clip witness for future runs.

Helpers are not connected to native gameplay and are linker-discarded. Image
remains WCA67's471008B SHA256
 a0b63f49de98835e97242f821851fe3ef30caf7be1cd8791cc079db5aed6e7b7.
That image still awaits its emulator screen check under the existing unlock
request; inspected Desktop preview stays WCA56 and failed lighting stays inactive.

This does not implement the whole SET loader: activation/filter/registry flags,
restored-entry data, task allocation order, persistent flags and deferred-free
lifetime still require their executor. Next connect that lifecycle and whale
loader/trigger/contact/history/effects, resolve exit-root behavior, then inspect
natural spawn/chase and implement pier/camera. Full CP1–CP9 is incomplete.
