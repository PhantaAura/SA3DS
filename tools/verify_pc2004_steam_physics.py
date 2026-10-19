#!/usr/bin/env python3
"""Static parity audit for the exact SA3DS Steam and 2004 SADX profiles.

This tool does not emit game code or data. It verifies two exact user-owned
executables, compares the full eight-record player-parameter table, and checks
selected ground/air kernel signatures that are semantically shared between the
2004 PC and Steam builds.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

STEAM = {
    "label": "Steam SADX",
    "size": 90_922_000,
    "sha256": "e4330c00d7ee3910ecc1acab789ffd2a2d571c506984b99a8de4d3663e697fd6",
    "table_va": 0x025D9B90,
    "initializer_va": 0x004DAA00,
    "ground_va": 0x004DBE50,
    "air_va": 0x004DDEC0,
}

PC2004 = {
    "label": "SADX 2004 US",
    "size": 55_549_952,
    "sha256": "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74",
    "table_va": 0x009154E8,
    "initializer_va": 0x00442750,
    "ground_va": 0x0044C270,
    "air_va": 0x0044B9C0,
}

RECORD_SIZE = 0x84
RECORD_COUNT = 8
TABLE_SIZE = RECORD_SIZE * RECORD_COUNT
CHARACTERS = ["Sonic", "Eggman", "Tails", "Knuckles", "Tikal", "Amy", "Gamma", "Big"]

# Steam 0.0.8.8 steep-surface branch evidence.
STEAM_GROUND_CONSTANTS = {
    "up_0_1": (0x00956C48, 0.10000000149011612, "d"),
    "cross_0_6": (0x0095A828, 0.6000000238418579, "d"),
    "speed_1_16": (0x009888B0, 1.159999966621399, "d"),
    "up_neg_0_4": (0x0095FF18, -0.4000000059604645, "d"),
    "weight_mul_5": (0x00959FF8, 5.0, "d"),
    "up_neg_0_3": (0x0095A808, -0.30000001192092896, "d"),
    "weight_mul_0_8": (0x00959FF0, 0.800000011920929, "d"),
    "up_neg_0_1": (0x0095A968, -0.10000000149011612, "d"),
    "weight_mul_0_4": (0x0095C670, 0.4000000059604645, "d"),
    "up_0_5": (0x00A5DE90, 0.5, "d"),
    "force_mul_4_225": (0x009888A8, 4.224999904632568, "d"),
    "up_0_7": (0x00959FD0, 0.699999988079071, "d"),
    "up_0_87": (0x009888A0, 0.8700000047683716, "d"),
    "side_mul_1_4": (0x0095CCA8, 1.399999976158142, "d"),
}

STEAM_GROUND_REFS = {
    0x004DBF8A: "dc1d486c9500",
    0x004DBFC7: "dc1db0889800",
    0x004DC005: "dc1d18ff9500",
    0x004DC02E: "dc0df89f9500",
    0x004DC045: "dc1d08a89500",
    0x004DC06E: "dc0df09f9500",
    0x004DC085: "dc1d68a99500",
    0x004DC0AE: "dc0d70c69500",
    0x004DC0C5: "dc1d90dea500",
    0x004DC107: "dc0da8889800",
    0x004DC127: "dc1dd09f9500",
    0x004DC17F: "d9421c",
    0x004DC182: "dc1da0889800",
    0x004DC1D0: "dc0da8cc9500",
}

# Matching values in the 2004 PC PGetAcceleration implementation. 2004 stores
# these as f32 while the Steam compiler references equivalent promoted doubles.
PC_GROUND_CONSTANTS = {
    "up_0_1": (0x007DD568, 0.10000000149011612, "f"),
    "cross_0_6": (0x007DD584, 0.6000000238418579, "f"),
    "speed_1_16": (0x007E0178, 1.159999966621399, "f"),
    "up_neg_0_4": (0x007E00E4, -0.4000000059604645, "f"),
    "weight_mul_5": (0x007DCCFC, 5.0, "f"),
    "up_neg_0_3": (0x007E01A8, -0.30000001192092896, "f"),
    "weight_mul_0_8": (0x007DCE5C, 0.800000011920929, "f"),
    "up_neg_0_1": (0x007E0064, -0.10000000149011612, "f"),
    "weight_mul_0_4": (0x007DD550, 0.4000000059604645, "f"),
    "up_0_5": (0x007DCC98, 0.5, "f"),
    "force_mul_4_225": (0x007E01A4, 4.224999904632568, "f"),
    "up_0_7": (0x007DD578, 0.699999988079071, "f"),
    "up_0_87": (0x007E0124, 0.8700000047683716, "f"),
    "side_mul_1_4": (0x007E0068, 1.399999976158142, "f"),
}

PC_GROUND_REFS = {
    0x0044C38D: "d81d68d57d00",
    0x0044C3A0: "d81d84d57d00",
    0x0044C3B0: "d81d78017e00",
    0x0044C3DC: "d81de4007e00",
    0x0044C3FF: "d80dfccc7d00",
    0x0044C458: "d81da8017e00",
    0x0044C47B: "d80d5cce7d00",
    0x0044C486: "d81d64007e00",
    0x0044C4A9: "d80d50d57d00",
    0x0044C4B7: "d81d98cc7d00",
    0x0044C4EA: "d80da4017e00",
    0x0044C506: "d81d78d57d00",
    0x0044C543: "d81d24017e00",
    0x0044C582: "d80d68007e00",
}

# Semantically paired PGetAccelerationAir checks. The registers/local-stack
# choices differ between compilers; the player-physics offsets and thresholds
# are the evidence we care about.
AIR_PAIRS = {
    "disable_control_0x4000": (
        (0x004DDED3, "0fbf480481e100400000"),
        (0x0044B9CA, "668b46042500400000"),
    ),
    "weight_offset_0x114": (
        (0x004DDF18, "d88814010000"),
        (0x0044BA00, "d88f14010000"),
    ),
    "air_resist_air_offset_0xec": (
        (0x004DDF61, "d982ec000000"),
        (0x0044BA58, "d987ec000000"),
    ),
    "air_resist_y_offset_0xf4": (
        (0x004DDF76, "d981f4000000"),
        (0x0044BA74, "d987f4000000"),
    ),
    "air_resist_z_offset_0xf8": (
        (0x004DDF8B, "d980f8000000"),
        (0x0044BA85, "d987f8000000"),
    ),
    "run_speed_offset_0xc4": (
        (0x004DDFEF, "d980c4000000"),
        (0x0044BAC2, "d89fc4000000"),
    ),
    "angle_threshold_0x6000": (
        (0x004DDFFE, "817de000600000"),
        (0x0044BAD4, "81f900600000"),
    ),
    "air_break_offset_0xe8": (
        (0x004DE00A, "d981e8000000"),
        (0x0044BAE0, "d88fe8000000"),
    ),
    "angle_threshold_0x1000": (
        (0x004DE018, "817de000100000"),
        (0x0044BAE8, "81f900100000"),
    ),
    "air_accel_offset_0xdc": (
        (0x004DE050, "d981dc000000"),
        (0x0044BB0A, "d88fdc000000"),
    ),
}


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def parse_pe(data: bytes):
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


def va_to_off(va: int, image_base: int, sections):
    rva = va - image_base
    for name, srva, vsize, rptr, rsize in sections:
        if srva <= rva < srva + rsize:
            return rptr + (rva - srva), name
    raise ValueError(f"VA {va:#x} not backed by file data")


def require_bytes(data, image_base, sections, va, hex_bytes, label):
    expected = bytes.fromhex(hex_bytes)
    off, _ = va_to_off(va, image_base, sections)
    actual = data[off:off + len(expected)]
    if actual != expected:
        raise ValueError(f"{label} signature mismatch at {va:#x}")


def verify_profile(path: Path, profile):
    size = path.stat().st_size
    digest = sha256(path)
    if size != profile["size"] or digest != profile["sha256"]:
        raise ValueError(
            f"unsupported {profile['label']} executable: size={size} sha256={digest}"
        )
    data = path.read_bytes()
    ib, sections = parse_pe(data)
    return data, ib, sections, {"size": size, "sha256": digest, "image_base": ib}


def read_constant(data, ib, sections, va, fmt):
    n = 8 if fmt == "d" else 4
    off, section = va_to_off(va, ib, sections)
    value = struct.unpack_from("<" + fmt, data, off)[0]
    return value, section


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("steam_exe", type=Path)
    ap.add_argument("pc2004_exe", type=Path)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    try:
        sdata, sib, ssecs, sprof = verify_profile(args.steam_exe, STEAM)
        pdata, pib, psecs, pprof = verify_profile(args.pc2004_exe, PC2004)

        # Exact initializer structure: CharID + 0x09, stride 0x84, table base,
        # copy 0x21 dwords (= 0x84 bytes).
        require_bytes(sdata, sib, ssecs, 0x004DAA2F,
                      "0fb6710969f68400000081c6909b5d02",
                      "Steam parameter initializer")
        require_bytes(sdata, sib, ssecs, 0x004DAA48,
                      "b921000000f3a5", "Steam 0x84 copy")
        require_bytes(pdata, pib, psecs, 0x0044276F,
                      "0fb6730969f684000000", "2004 parameter initializer")
        require_bytes(pdata, pib, psecs, 0x0044277B,
                      "81c6e8549100", "2004 parameter table base")
        require_bytes(pdata, pib, psecs, 0x00442787,
                      "b921000000f3a5", "2004 0x84 copy")

        soff, ssection = va_to_off(STEAM["table_va"], sib, ssecs)
        poff, psection = va_to_off(PC2004["table_va"], pib, psecs)
        stable = sdata[soff:soff + TABLE_SIZE]
        ptable = pdata[poff:poff + TABLE_SIZE]
        if stable != ptable:
            raise ValueError("player parameter tables differ")
        table_hash = hashlib.sha256(stable).hexdigest()
        character_equal = []
        for i, name in enumerate(CHARACTERS):
            a = stable[i * RECORD_SIZE:(i + 1) * RECORD_SIZE]
            b = ptable[i * RECORD_SIZE:(i + 1) * RECORD_SIZE]
            character_equal.append({"character": name, "identical": a == b})

        # Ground kernel entry points + selected branch refs/constants.
        require_bytes(sdata, sib, ssecs, STEAM["ground_va"],
                      "558bec83ec50", "Steam PGetAcceleration")
        require_bytes(pdata, pib, psecs, PC2004["ground_va"],
                      "83ec30578b7c2438", "2004 PGetAcceleration")
        for va, hx in STEAM_GROUND_REFS.items():
            require_bytes(sdata, sib, ssecs, va, hx, "Steam ground")
        for va, hx in PC_GROUND_REFS.items():
            require_bytes(pdata, pib, psecs, va, hx, "2004 ground")

        ground_values = {}
        for name in STEAM_GROUND_CONSTANTS:
            sva, sexpected, sfmt = STEAM_GROUND_CONSTANTS[name]
            pva, pexpected, pfmt = PC_GROUND_CONSTANTS[name]
            sv, ss = read_constant(sdata, sib, ssecs, sva, sfmt)
            pv, ps = read_constant(pdata, pib, psecs, pva, pfmt)
            if sv != sexpected or pv != pexpected:
                raise ValueError(f"ground constant mismatch: {name}")
            # The Steam literals are equivalent to the 2004 f32 values after
            # the compiler's promotion to double.
            if sv != float(pv):
                raise ValueError(f"ground semantic value differs: {name}: {sv} != {pv}")
            ground_values[name] = {
                "value": sv,
                "steam_va": sva,
                "steam_section": ss,
                "pc2004_va": pva,
                "pc2004_section": ps,
            }

        # Air kernel entry points and paired semantic checks.
        require_bytes(sdata, sib, ssecs, STEAM["air_va"],
                      "558bec83ec20", "Steam PGetAccelerationAir")
        require_bytes(pdata, pib, psecs, PC2004["air_va"],
                      "83ec10568b742418", "2004 PGetAccelerationAir")
        air_checks = []
        for label, (scheck, pcheck) in AIR_PAIRS.items():
            require_bytes(sdata, sib, ssecs, scheck[0], scheck[1], f"Steam air {label}")
            require_bytes(pdata, pib, psecs, pcheck[0], pcheck[1], f"2004 air {label}")
            air_checks.append(label)

        result = {
            "schema": "sa3ds-pc2004-steam-physics-parity-v1",
            "steam": {**STEAM, **sprof},
            "pc2004": {**PC2004, **pprof},
            "player_parameters": {
                "record_size": RECORD_SIZE,
                "record_count": RECORD_COUNT,
                "bytes_compared": TABLE_SIZE,
                "sha256": table_hash,
                "byte_identical": True,
                "steam_section": ssection,
                "pc2004_section": psection,
                "characters": character_equal,
            },
            "ground_kernel": {
                "semantic_constants_verified": len(ground_values),
                "steam_instruction_refs_verified": len(STEAM_GROUND_REFS),
                "pc2004_instruction_refs_verified": len(PC_GROUND_REFS),
                "constants": ground_values,
            },
            "air_kernel": {
                "semantic_pairs_verified": len(air_checks),
                "checks": air_checks,
            },
            "conclusion": (
                "The exact 2004 and Steam profiles share a byte-identical 8x0x84 "
                "player parameter table and strongly matching core ground/air "
                "acceleration evidence. This supports keeping the current SA3DS "
                "core movement parameter/kernel baseline while making 2004 PC the "
                "primary mod-support source profile."
            ),
            "boundary": (
                "This is a static parity audit, not proof that every collision, "
                "input, camera, frame-cadence, Sonic action, or Spin Dash path is "
                "identical between releases."
            ),
        }

        if args.json:
            print(json.dumps(result, indent=2))
        else:
            print("SADX 2004 <-> Steam core physics parity audit: PASS")
            print(f"  player parameter bytes: {TABLE_SIZE} / {TABLE_SIZE} identical")
            print(f"  parameter table SHA-256: {table_hash}")
            print(f"  ground semantic constants: {len(ground_values)} / {len(ground_values)}")
            print(f"  ground instruction refs: Steam {len(STEAM_GROUND_REFS)}, 2004 {len(PC_GROUND_REFS)}")
            print(f"  air semantic pairs: {len(air_checks)} / {len(air_checks)}")
            print("  result: current core SA3DS movement baseline can be retained")
        return 0
    except (OSError, ValueError, struct.error) as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
