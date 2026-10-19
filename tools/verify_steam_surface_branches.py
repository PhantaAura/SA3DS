#!/usr/bin/env python3
"""Verify the 0.0.8.8 Steam surface-acceleration evidence against one exact EXE.

No game data is emitted. The tool rejects every executable except the locked
Steam SADX fingerprint and checks instruction signatures plus referenced
floating-point constants used by the steep-surface branch.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

EXPECTED_SIZE = 90_922_000
EXPECTED_SHA256 = "e4330c00d7ee3910ecc1acab789ffd2a2d571c506984b99a8de4d3663e697fd6"
PGETACCEL_VA = 0x004DBE50

CONSTANTS = {
    "up_0_1": (0x00956C48, 0.10000000149011612),
    "cross_0_6": (0x0095A828, 0.6000000238418579),
    "speed_1_16": (0x009888B0, 1.159999966621399),
    "up_neg_0_4": (0x0095FF18, -0.4000000059604645),
    "weight_mul_5": (0x00959FF8, 5.0),
    "up_neg_0_3": (0x0095A808, -0.30000001192092896),
    "weight_mul_0_8": (0x00959FF0, 0.800000011920929),
    "up_neg_0_1": (0x0095A968, -0.10000000149011612),
    "weight_mul_0_4": (0x0095C670, 0.4000000059604645),
    "up_0_5": (0x00A5DE90, 0.5),
    "force_mul_4_225": (0x009888A8, 4.224999904632568),
    "up_0_7": (0x00959FD0, 0.699999988079071),
    "up_0_87": (0x009888A0, 0.8700000047683716),
    "side_mul_1_4": (0x0095CCA8, 1.399999976158142),
}

# Instruction sites that directly reference constants above. These signatures
# are narrow enough to catch a wrong revision/profile without embedding code.
REF_SIGNATURES = {
    0x004DBF8A: bytes.fromhex("dc1d486c9500"),   # 0.1
    0x004DBFC7: bytes.fromhex("dc1db0889800"),   # 1.16
    0x004DC005: bytes.fromhex("dc1d18ff9500"),   # -0.4
    0x004DC02E: bytes.fromhex("dc0df89f9500"),   # 5.0
    0x004DC045: bytes.fromhex("dc1d08a89500"),   # -0.3
    0x004DC06E: bytes.fromhex("dc0df09f9500"),   # 0.8
    0x004DC085: bytes.fromhex("dc1d68a99500"),   # -0.1
    0x004DC0AE: bytes.fromhex("dc0d70c69500"),   # 0.4
    0x004DC0C5: bytes.fromhex("dc1d90dea500"),   # 0.5
    0x004DC107: bytes.fromhex("dc0da8889800"),   # 4.225
    0x004DC127: bytes.fromhex("dc1dd09f9500"),   # 0.7
    0x004DC17F: bytes.fromhex("d9421c"),          # Up load before 0.87 compare
    0x004DC182: bytes.fromhex("dc1da0889800"),   # 0.87
    0x004DC1D0: bytes.fromhex("dc0da8cc9500"),   # 1.4
}


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def parse_sections(data: bytes):
    if data[:2] != b"MZ":
        raise ValueError("not MZ")
    peoff = struct.unpack_from("<I", data, 0x3C)[0]
    if data[peoff:peoff + 4] != b"PE\0\0":
        raise ValueError("not PE")
    coff = peoff + 4
    _, nsec, _, _, _, opt_size, _ = struct.unpack_from("<HHIIIHH", data, coff)
    opt = coff + 20
    if struct.unpack_from("<H", data, opt)[0] != 0x10B:
        raise ValueError("not PE32")
    image_base = struct.unpack_from("<I", data, opt + 28)[0]
    sections = []
    so = opt + opt_size
    for i in range(nsec):
        o = so + i * 40
        name = data[o:o + 8].split(b"\0", 1)[0].decode("ascii", "replace")
        vsize, rva, rsize, rptr = struct.unpack_from("<IIII", data, o + 8)
        sections.append((name, rva, vsize, rptr, rsize))
    return image_base, sections


def va_to_off(va, image_base, sections):
    rva = va - image_base
    for name, srva, vsize, rptr, rsize in sections:
        if srva <= rva < srva + max(vsize, rsize):
            return rptr + (rva - srva), name
    raise ValueError(f"VA {va:#x} unmapped")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("exe", type=Path)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    size = args.exe.stat().st_size
    digest = sha256(args.exe)
    if size != EXPECTED_SIZE or digest != EXPECTED_SHA256:
        print(f"unsupported EXE: size={size} sha256={digest}", file=sys.stderr)
        return 2

    data = args.exe.read_bytes()
    image_base, sections = parse_sections(data)

    # Function prologue + DisableControl mask on the locked Steam build.
    pget_off, _ = va_to_off(PGETACCEL_VA, image_base, sections)
    prologue = bytes.fromhex("558bec83ec50")
    if data[pget_off:pget_off + len(prologue)] != prologue:
        raise ValueError("PGetAcceleration candidate prologue mismatch")

    for va, signature in REF_SIGNATURES.items():
        off, _ = va_to_off(va, image_base, sections)
        if data[off:off + len(signature)] != signature:
            raise ValueError(f"instruction signature mismatch at {va:#x}")

    decoded = {}
    for name, (va, expected) in CONSTANTS.items():
        off, section = va_to_off(va, image_base, sections)
        value = struct.unpack_from("<d", data, off)[0]
        if value != expected:
            raise ValueError(f"constant mismatch {name}: {value!r} != {expected!r}")
        decoded[name] = {"va": va, "section": section, "double": value}

    result = {
        "schema": "sa3ds-steam-surface-evidence-v1",
        "exe_size": size,
        "exe_sha256": digest,
        "pgetacceleration_va": PGETACCEL_VA,
        "verified_instruction_signatures": len(REF_SIGNATURES),
        "constants": decoded,
        "note": "This verifies branch signatures/constants, not full collision ordering or 1:1 gameplay parity.",
    }
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print("Steam surface evidence: PASS")
        print(f"  PGetAcceleration candidate: {PGETACCEL_VA:#010x}")
        print(f"  instruction signatures: {len(REF_SIGNATURES)}")
        print(f"  decoded constants: {len(decoded)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
