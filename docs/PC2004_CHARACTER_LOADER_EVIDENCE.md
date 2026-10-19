# PC 2004 Character Loader Evidence — 0.0.9.0A

## CONFIRMED from exact `sonic.exe`

Exact source: 55,549,952 bytes, SHA-256 `6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74`.

The loader at **VA 0x00401AC0** imports `LoadLibraryA` at **0x007DB040** and `GetProcAddress` at **0x007DB044**, loads `SYSTEM\chrmodels.dll`, and resolves named character exports.

Sonic handoff is now exact:

| Export | receiving global | loader witness |
|---|---:|---:|
| `___SONIC_MODELS` | `0x038F6EE8` | `0x00401BF5` |
| `___SONIC_OBJECTS` | `0x038F6E80` | `0x00401C04` |
| `___SONIC_ACTIONS` | `0x038F6E7C` | `0x00401C13` |
| `___SONIC_MOTIONS` | `0x03A8AF70` | `0x00401C22` |

The executable uses the export addresses as DWORD tables. Runtime witnesses also confirm that action pointers expose an object pointer at `+0x00` and a motion pointer at `+0x04`.

## CONFIRMED from exact SADX Mod Loader proxy

The modded profile's 1,031,680-byte `system\CHRMODELS.dll` hash is identical to `.modloader/SADXModLoader.dll`: `3293cf354dcfc62274c2ad25c09041606d6ad55483f070c233111cd9b47a97ba`.

That DLL has **no normal PE export directory**. Its code explicitly obtains module `CHRMODELS_orig` and asks `GetProcAddress` for character symbols, including `___SONIC_OBJECTS`. Therefore the modded install's live `CHRMODELS.dll` is a Mod Loader proxy, while original character data lives behind `CHRMODELS_orig.dll`.

The same exact proxy build contains code-backed Sonic table counts:

- `___SONIC_MODELS`: **11** entries
- `___SONIC_OBJECTS`: **79** entries
- `___SONIC_ACTIONS`: **149** entries

Its recursive object walker proves these `NJS_OBJECT` pointer offsets for the original data it consumes:

- model pointer: **+0x04**
- child pointer: **+0x2C**
- sibling pointer: **+0x30**
- minimum object size implied: **0x34 bytes**

And the 2004 executable proves the action's first two pointer fields:

- object/root pointer: **+0x00**
- motion pointer: **+0x04**
- minimum action size implied: **8 bytes**

## Still UNKNOWN

- source object transform field semantics/order/angle units
- exact model/mesh/material layout semantics
- exact `___SONIC_MOTIONS` table count
- motion key/channel binary layout
- source material/render flags
- real Sonic geometry/texture counts

These stay unknown until `CHRMODELS_orig.dll` bytes are available and fingerprint-verified.

## Machine-checked byte witnesses

`tools/recon_pc2004_character_loader.py` refuses to promote the key layout/count
facts if these instruction bytes differ from the exact fingerprinted binaries.

### `sonic.exe`

| VA | bytes | meaning used by this gate |
|---:|---|---|
| `0x00494426` | `8B 01` | read ACTION +0x00 object/root pointer |
| `0x0049442C` | `8B 15 7C 6E 8F 03 3B 0A` | compare current action pointer against Sonic action-table entry |
| `0x00494464` | `8B 51 04` | read ACTION +0x04 motion pointer |
| `0x004945A8` | `8B 15 70 AF A8 03 8B 42 04` | use Sonic motion table and entry +4 |

### SADX Mod Loader proxy

| VA | bytes | meaning used by this gate |
|---:|---|---|
| `0x1005EA57` | `68 E4 BF 0D 10 68 C4 BF 0D 10` | original-module/export probe context |
| `0x10093237` | `8B 4E 04` | object +0x04 model pointer |
| `0x10093243` | `8B 4E 2C` | object +0x2C child pointer |
| `0x1009324F` | `8B 76 30` | object +0x30 sibling pointer |
| `0x100937BC` | `6A 01 6A 4F` | Sonic object count 0x4F / 79 |
| `0x10093841` | `81 FE 95 00 00 00` | Sonic action traversal limit 0x95 / 149 |
| `0x10093849` | `6A 00 6A 0B` | Sonic model count 0x0B / 11 |

The fingerprint check plus these byte checks make accidental address reuse from
the Steam/Gate-0 executable or a different Mod Loader build fail closed.
