#!/usr/bin/env python3
"""Recover the PC2004 runtime SonicAnimData -> ___SONIC_ACTIONS slot remap.

The 146-entry SonicAnimData array lives in executable zero-fill/BSS, so its
contents are not present as file bytes.  The exact retail initializer at
0x007CEC90 fills every 16-byte AnimData_t row.  Its first field is either a
pointer loaded from the ___SONIC_ACTIONS pointer table or NULL.

This tool recognizes those exact initializer stores without executing x86.  In
addition to the ACTION pointer remap it recovers the five scalar fields that
control the retail animation state machine.  It emits only structural
indices/metadata; it does not extract Sega assets.
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


def _modrm_length(code: bytes, pos: int, operand_size: int, immediate_size: int = 0) -> int:
    """Length of the small ModR/M subset used by the retail initializer."""
    if pos + 2 > len(code):
        raise ReconError("truncated ModR/M instruction")
    modrm = code[pos + 1]
    mod = modrm >> 6
    rm = modrm & 7
    length = 2
    if mod != 3 and rm == 4:  # SIB
        if pos + length >= len(code):
            raise ReconError("truncated SIB")
        sib = code[pos + length]
        length += 1
        if mod == 0 and (sib & 7) == 5:
            length += 4
    if mod == 0 and rm == 5:
        length += 4
    elif mod == 1:
        length += 1
    elif mod == 2:
        length += 4
    return length + immediate_size


def _recover_scalar_stores(code: bytes):
    """Interpret the straight-line MOV/XOR subset in InitSonicAnimData.

    The exact PC2004 function contains only MOV, XOR ESI,ESI, PUSH/POP and RET.
    Tracking register constants is sufficient to recover stores compiled via
    either immediates or register reuse, while rejecting any changed opcode
    stream instead of guessing.
    """
    # Each register carries known-bit mask/value.  Partial byte assignments are
    # common because the compiler reuses CL/DL/BL for row fields.
    masks = [0] * 8
    values = [0] * 8
    stores = {}

    def assign(reg: int, value: int, mask: int = 0xFFFFFFFF):
        masks[reg] = mask
        values[reg] = value & mask

    def assign_low8(encoded: int, value: int):
        reg = encoded & 3
        shift = 8 if encoded >= 4 else 0
        mask = 0xFF << shift
        masks[reg] |= mask
        values[reg] = (values[reg] & ~mask) | ((value & 0xFF) << shift)

    def store(address: int, width: int, value: int, mask: int):
        if not (ANIMDATA_VA <= address < ANIMDATA_VA + ANIMDATA_COUNT * ANIMDATA_STRIDE):
            return
        # ACTION pointers are recovered independently from their source-table
        # loads; their runtime addresses are intentionally not interpreted.
        if (address - ANIMDATA_VA) % ANIMDATA_STRIDE == 0:
            return
        needed = (1 << (width * 8)) - 1
        if mask & needed != needed:
            raise ReconError(f"initializer store {address:#x}: register value is not constant")
        if address in stores:
            raise ReconError(f"initializer store {address:#x}: duplicate write")
        stores[address] = (width, value & needed)

    pos = 0
    while pos < len(code):
        op = code[pos]
        if op == 0xC3:  # RET
            pos += 1
            break
        if 0x50 <= op <= 0x57:  # PUSH reg
            pos += 1
            continue
        if 0x58 <= op <= 0x5F:  # POP reg
            assign(op - 0x58, 0, 0)
            pos += 1
            continue
        if op == 0xA1:  # MOV EAX,[moffs32]
            assign(0, 0, 0)
            pos += 5
            continue
        if op == 0xA3:  # MOV [moffs32],EAX
            address = struct.unpack_from("<I", code, pos + 1)[0]
            store(address, 4, values[0], masks[0])
            pos += 5
            continue
        if 0xB0 <= op <= 0xB7:  # MOV r8,imm8
            assign_low8(op - 0xB0, code[pos + 1])
            pos += 2
            continue
        if 0xB8 <= op <= 0xBF:  # MOV r32,imm32
            assign(op - 0xB8, struct.unpack_from("<I", code, pos + 1)[0])
            pos += 5
            continue
        if op == 0x33:  # XOR r32,r/m32 (only self-XOR is admitted)
            modrm = code[pos + 1]
            dst = (modrm >> 3) & 7
            src = modrm & 7
            if modrm >> 6 != 3 or dst != src:
                raise ReconError(f"unexpected XOR at initializer +{pos:#x}")
            assign(dst, 0)
            pos += 2
            continue
        if op == 0x8B:  # MOV r32,r/m32
            modrm = code[pos + 1]
            dst = (modrm >> 3) & 7
            length = _modrm_length(code, pos, 4)
            if modrm >> 6 == 3:
                src = modrm & 7
                masks[dst], values[dst] = masks[src], values[src]
            else:
                assign(dst, 0, 0)  # ACTION pointer load; not scalar metadata.
            pos += length
            continue
        if op in (0x88, 0x89):  # MOV r8/r32 to r/m
            width = 1 if op == 0x88 else 4
            modrm = code[pos + 1]
            src = (modrm >> 3) & 7
            length = _modrm_length(code, pos, width)
            if modrm >> 6 == 0 and (modrm & 7) == 5:
                address = struct.unpack_from("<I", code, pos + 2)[0]
                store(address, width, values[src], masks[src])
            pos += length
            continue
        if op in (0xC6, 0xC7):  # MOV imm8/imm32 to r/m
            width = 1 if op == 0xC6 else 4
            modrm = code[pos + 1]
            if (modrm >> 3) & 7:
                raise ReconError(f"unexpected C6/C7 group at initializer +{pos:#x}")
            length = _modrm_length(code, pos, width, width)
            if modrm >> 6 == 0 and (modrm & 7) == 5:
                address = struct.unpack_from("<I", code, pos + 2)[0]
                value = int.from_bytes(code[pos + length - width:pos + length], "little")
                store(address, width, value, (1 << (width * 8)) - 1)
            pos += length
            continue
        if op == 0x66:
            if pos + 2 >= len(code):
                raise ReconError("truncated operand-size-prefixed instruction")
            sub = code[pos + 1]
            if sub == 0x89:  # MOV r16 to r/m16
                modrm_pos = pos + 1
                modrm = code[pos + 2]
                src = (modrm >> 3) & 7
                length = 1 + _modrm_length(code, modrm_pos, 2)
                if modrm >> 6 == 0 and (modrm & 7) == 5:
                    address = struct.unpack_from("<I", code, pos + 3)[0]
                    store(address, 2, values[src], masks[src])
                pos += length
                continue
            if sub == 0xC7:  # MOV imm16 to r/m16
                modrm_pos = pos + 1
                modrm = code[pos + 2]
                if (modrm >> 3) & 7:
                    raise ReconError(f"unexpected 66 C7 group at initializer +{pos:#x}")
                length = 1 + _modrm_length(code, modrm_pos, 2, 2)
                if modrm >> 6 == 0 and (modrm & 7) == 5:
                    address = struct.unpack_from("<I", code, pos + 3)[0]
                    value = struct.unpack_from("<H", code, pos + length - 2)[0]
                    store(address, 2, value, 0xFFFF)
                pos += length
                continue
        raise ReconError(f"unexpected opcode {op:#04x} at initializer +{pos:#x}")

    if pos != len(code):
        raise ReconError(f"initializer decode ended at +{pos:#x}, expected +{len(code):#x}")

    rows = []
    for dense_id in range(ANIMDATA_COUNT):
        base = ANIMDATA_VA + dense_id * ANIMDATA_STRIDE
        fields = ((4, 1, "instance"), (5, 1, "property"), (6, 2, "next_animation"),
                  (8, 4, "transition_speed"), (12, 4, "animation_speed"))
        row = {"dense_id": dense_id}
        for offset, width, name in fields:
            actual = stores.get(base + offset)
            if actual is None or actual[0] != width:
                raise ReconError(f"dense animation {dense_id}: missing {name} store")
            raw = actual[1]
            row[name] = struct.unpack("<f", struct.pack("<I", raw))[0] if width == 4 else raw
        rows.append(row)
    return rows


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

    rows = _recover_scalar_stores(code)
    for row, slot in zip(rows, mapping):
        row["source_action_slot"] = slot

    return {
        "schema": "sa3ds-pc2004-sonic-animdata-v2",
        "source_sha256": sha,
        "initializer_va": f"0x{INIT_START_VA:08X}",
        "runtime_animdata_va": f"0x{ANIMDATA_VA:08X}",
        "dense_animation_count": ANIMDATA_COUNT,
        "source_action_slot_count": ACTION_SLOT_COUNT,
        "null_dense_animation_ids": nulls,
        "dense_to_source_action_slot": mapping,
        "animdata_rows": rows,
        "core_movement": {name: rows[dense]
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
