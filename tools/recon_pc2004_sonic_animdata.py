#!/usr/bin/env python3
"""Recover the PC2004 runtime SonicAnimData -> ___SONIC_ACTIONS slot remap.

The 146-entry SonicAnimData array lives in executable zero-fill/BSS, so its
contents are not present as file bytes.  The exact retail initializer at
0x007CEC90 fills every 16-byte AnimData_t row.  Its first field is either a
pointer loaded from the ___SONIC_ACTIONS pointer table or NULL.

This tool recognizes those exact initializer stores without executing x86.
It emits only structural indices/metadata; it does not extract Sega assets.
"""
from __future__ import annotations

from pathlib import Path
import argparse
import hashlib
import json
import struct

EXPECTED_SIZE = 55_549_952
EXPECTED_SHA256 = "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"
EXPECTED_IMAGE_BASE = 0x00400000
INIT_START_VA = 0x007CEC90
INIT_END_VA = 0x007D0A8A  # one byte past RET at 0x007D0A89
ANIMDATA_VA = 0x03C56210
ANIMDATA_COUNT = 146
ANIMDATA_STRIDE = 16
ACTION_SLOT_COUNT = 149
ACTION_TABLE_GLOBAL = 0x038F6E7C
NULL_DENSE_IDS = (75, 76)

# Dense runtime animation IDs useful to Movement Lab. Names match the public
# SADX animation enum; the slot values are recovered from the exact executable.
CORE_DENSE_IDS = {
    "Stand": 0,
    "Walk1": 9,
    "Walk2": 10,
    "Walk3": 11,
    "Run1": 12,
    "Run2": 13,
    "JumpOrSpin": 14,
    "Roll": 15,
    "Spring": 16,
    "SpringToFall": 17,
    "Fall": 18,
    "Land": 19,
    "Brake": 20,
    "Uncurl": 29,
    "TurnAround": 30,
    "Skidding": 31,
    "SpinBall": 32,
}


class ReconError(ValueError):
    pass


def _pe_sections(data: bytes):
    if data[:2] != b"MZ":
        raise ReconError("not MZ")
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ReconError("not PE")
    coff = pe + 4
    nsec = struct.unpack_from("<H", data, coff + 2)[0]
    optsz = struct.unpack_from("<H", data, coff + 16)[0]
    opt = coff + 20
    if struct.unpack_from("<H", data, opt)[0] != 0x10B:
        raise ReconError("not PE32")
    image_base = struct.unpack_from("<I", data, opt + 28)[0]
    sections = []
    sec = opt + optsz
    for i in range(nsec):
        o = sec + i * 40
        vsize, rva, rawsize, rawoff = struct.unpack_from("<IIII", data, o + 8)
        sections.append((rva, max(vsize, rawsize), rawsize, rawoff))
    return image_base, sections


def _va_to_off(data: bytes, va: int) -> int:
    image_base, sections = _pe_sections(data)
    rva = va - image_base
    for srva, span, rawsize, rawoff in sections:
        if srva <= rva < srva + span:
            delta = rva - srva
            if delta >= rawsize:
                raise ReconError(f"VA {va:#x} is zero-fill")
            return rawoff + delta
    raise ReconError(f"unmapped VA {va:#x}")


def _nearest_action_load(code: bytes, before: int, reg: int):
    """Find nearest `mov reg,[eax+disp]` in the preceding AnimData row setup."""
    lo = max(0, before - 96)
    candidates = []
    for q in range(lo, before):
        if code[q] != 0x8B or q + 1 >= len(code):
            continue
        modrm = code[q + 1]
        if ((modrm >> 3) & 7) != reg or (modrm & 7) != 0:  # base EAX
            continue
        mod = (modrm >> 6) & 3
        if mod == 0:
            disp = 0
        elif mod == 1 and q + 3 <= len(code):
            disp = struct.unpack_from("<b", code, q + 2)[0]
        elif mod == 2 and q + 6 <= len(code):
            disp = struct.unpack_from("<i", code, q + 2)[0]
        else:
            continue
        if disp >= 0 and disp % 4 == 0 and disp // 4 < ACTION_SLOT_COUNT:
            candidates.append((q, disp // 4))
    return candidates[-1][1] if candidates else None


def recover_mapping(path: Path):
    data = Path(path).read_bytes()
    sha = hashlib.sha256(data).hexdigest()
    if len(data) != EXPECTED_SIZE or sha != EXPECTED_SHA256:
        raise ReconError(
            f"unsupported sonic.exe: size={len(data)} sha256={sha}; expected exact PC2004 profile"
        )
    image_base, _ = _pe_sections(data)
    if image_base != EXPECTED_IMAGE_BASE:
        raise ReconError(f"unexpected image base {image_base:#x}")
    start = _va_to_off(data, INIT_START_VA)
    end = _va_to_off(data, INIT_END_VA - 1) + 1
    code = data[start:end]

    # Exact function witnesses: load ___SONIC_ACTIONS receiving global and RET.
    if code[:5] != b"\xA1" + struct.pack("<I", ACTION_TABLE_GLOBAL):
        raise ReconError("SonicAnimData initializer prologue witness mismatch")
    if code[-1] != 0xC3:
        raise ReconError("SonicAnimData initializer RET witness mismatch")

    mapping = []
    for dense_id in range(ANIMDATA_COUNT):
        dest = ANIMDATA_VA + dense_id * ANIMDATA_STRIDE
        stores = []
        # 89 /r with mod=00 r/m=101 is mov [disp32],r32.
        for reg in range(8):
            pattern = bytes((0x89, 0x05 | (reg << 3))) + struct.pack("<I", dest)
            pos = code.find(pattern)
            if pos >= 0:
                stores.append((pos, reg))
        if len(stores) != 1:
            raise ReconError(f"dense animation {dense_id}: expected one pointer-field store, got {stores}")
        pos, reg = stores[0]
        slot = _nearest_action_load(code, pos, reg)
        if slot is None:
            # IDs 75/76 intentionally store ESI, which the initializer zeroes.
            if dense_id not in NULL_DENSE_IDS or reg != 6 or b"\x33\xF6" not in code[:128]:
                raise ReconError(f"dense animation {dense_id}: unresolved pointer source")
        mapping.append(slot)

    nulls = [i for i, slot in enumerate(mapping) if slot is None]
    if nulls != list(NULL_DENSE_IDS):
        raise ReconError(f"unexpected null dense animation IDs: {nulls}")
    if any(slot in (24, 25, 26) for slot in mapping if slot is not None):
        raise ReconError("mapping references a null ___SONIC_ACTIONS slot")

    return {
        "schema": "sa3ds-pc2004-sonic-animdata-remap-v1",
        "source_sha256": sha,
        "initializer_va": f"0x{INIT_START_VA:08X}",
        "runtime_animdata_va": f"0x{ANIMDATA_VA:08X}",
        "dense_animation_count": ANIMDATA_COUNT,
        "source_action_slot_count": ACTION_SLOT_COUNT,
        "null_dense_animation_ids": nulls,
        "dense_to_source_action_slot": mapping,
        "core_movement": {name: {"dense_id": dense, "source_action_slot": mapping[dense]}
                          for name, dense in CORE_DENSE_IDS.items()},
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("sonic_exe", type=Path)
    ap.add_argument("--json", type=Path)
    args = ap.parse_args()
    report = recover_mapping(args.sonic_exe)
    text = json.dumps(report, indent=2) + "\n"
    if args.json:
        args.json.write_text(text, encoding="utf-8")
    print(text, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
