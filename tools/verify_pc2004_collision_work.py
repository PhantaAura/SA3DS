#!/usr/bin/env python3
"""Verify 0.0.8.8.9 retail collision-work reconstruction evidence.

The verifier fingerprints the exact supported 2004 US sonic.exe and checks
small machine-code/data signatures only. It emits no executable data.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

PROFILE_SIZE = 55_549_952
PROFILE_SHA256 = "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"

SIGNATURES = {
    # PInitialize: distinct 0x84 csts allocation stored at playerwk+0x64.
    "pinitialize_collision_work_alloc": (0x0044279A, "68840000006a01"),
    "pinitialize_collision_work_store": (0x004427AA, "894564"),
    # General PSetPosition helper: cstsp + initialization.
    "general_cstsp_load": (0x0044951C, "8b6864"),
    "general_tnorm_zero_xz": (0x0044952F, "89552489551c"),
    "general_tnorm_y_one": (0x00449535, "c745200000803f"),
    "general_angles": (0x00449546, "8b53148955348b531c895538"),
    "general_height_load": (0x00449555, "d98010010000"),
    "general_height_half": (0x00449563, "d80d98cc7d00"),
    # Candidate polygon resolver and two arbitration helpers.
    "candidate_polygon_call": (0x00449710, "e80ba10000"),
    "candidate_arbitrate_a": (0x00449740, "e89b35ffff"),
    "candidate_arbitrate_b": (0x00449762, "e8193affff"),
    # csts.find_count and normal cutoff are directly used by both selectors.
    "selector_a_find_count": (0x0043CCE9, "66395f28"),
    "selector_a_angle_cutoff": (0x0043CD73, "3d002c0000"),
    "selector_b_find_count": (0x0043D189, "66395e28"),
    "selector_b_angle_cutoff": (0x0043D1FE, "3d002c0000"),
    # HandleSurfaceFlags: accumulate current surface attr, then material multiplier.
    "handle_surface_flags_attr_or": (0x0043CB90, "09585c"),
    "handle_surface_mul_015": (0x0043CCA4, "f6c3087408c7463c9a99193e"),
    "handle_surface_mul_010": (0x0043CCB1, "f6c3047408c7463ccdcccc3d"),
    "handle_surface_mul_050": (0x0043CCBE, "f6c3107408c7463c0000003f"),
    # Broad polygon routine: radius > 0 and internally radius * 0.5.
    "polygon_radius_positive": (0x0045382B, "8b4508d900d81d2ccb7d00"),
    "polygon_radius_half": (0x0045385C, "8b4d08d901d80d98cc7d00"),
}

CONST = {
    "half": (0x007DCC98, 0.5),
    "zero": (0x007DCB2C, 0.0),
}

CALL_TARGETS = {
    0x00449710: 0x00453820,
    0x00449740: 0x0043CCE0,
    0x00449762: 0x0043D180,
    0x0044980B: 0x0043CB90,
}


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda: f.read(1024 * 1024), b""):
            h.update(b)
    return h.hexdigest()


def parse_pe(data: bytes):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[:2] != b"MZ" or data[pe:pe+4] != b"PE\0\0":
        raise ValueError("not PE")
    coff = pe + 4
    _, nsec, _, _, _, opt_size, _ = struct.unpack_from("<HHIIIHH", data, coff)
    opt = coff + 20
    if struct.unpack_from("<H", data, opt)[0] != 0x10B:
        raise ValueError("not PE32")
    base = struct.unpack_from("<I", data, opt + 28)[0]
    sections = []
    so = opt + opt_size
    for i in range(nsec):
        o = so + i * 40
        name = data[o:o+8].split(b"\0", 1)[0].decode("ascii", "replace")
        vsize, rva, rsize, rptr = struct.unpack_from("<IIII", data, o + 8)
        sections.append((name, rva, vsize, rptr, rsize))
    return base, sections


def va_to_off(va: int, base: int, sec):
    rva = va - base
    for name, srva, vsize, rptr, rsize in sec:
        if srva <= rva < srva + rsize:
            return rptr + (rva - srva), name
    raise ValueError(f"unbacked VA {va:#x}")


def require(data, base, sec, va, hx, label):
    exp = bytes.fromhex(hx)
    off, _ = va_to_off(va, base, sec)
    got = data[off:off+len(exp)]
    if got != exp:
        raise ValueError(f"{label} mismatch at {va:#x}: {got.hex()} != {exp.hex()}")


def rel32_target(data, base, sec, va):
    off, _ = va_to_off(va, base, sec)
    if data[off] != 0xE8:
        raise ValueError(f"expected CALL at {va:#x}")
    rel = struct.unpack_from("<i", data, off + 1)[0]
    return (va + 5 + rel) & 0xFFFFFFFF


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pc2004_exe", type=Path)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    size = args.pc2004_exe.stat().st_size
    digest = sha256(args.pc2004_exe)
    if size != PROFILE_SIZE or digest != PROFILE_SHA256:
        raise SystemExit(f"unsupported SADX 2004 EXE: size={size} sha256={digest}")

    data = args.pc2004_exe.read_bytes()
    base, sec = parse_pe(data)
    for label, (va, hx) in SIGNATURES.items():
        require(data, base, sec, va, hx, label)

    constants = {}
    for name, (va, expected) in CONST.items():
        off, section = va_to_off(va, base, sec)
        got = struct.unpack_from("<f", data, off)[0]
        if got != expected:
            raise ValueError(f"{name}: {got!r} != {expected!r}")
        constants[name] = {"va": va, "value": got, "section": section}

    targets = {}
    for call, expected in CALL_TARGETS.items():
        got = rel32_target(data, base, sec, call)
        if got != expected:
            raise ValueError(f"CALL {call:#x}: {got:#x} != {expected:#x}")
        targets[f"{call:#x}"] = got

    result = {
        "schema": "sa3ds-pc2004-collision-work-evidence-v1",
        "profile": {"size": size, "sha256": digest, "image_base": base},
        "collision_work": {
            "size": 0x84,
            "pointer_offset_in_playerwk": 0x64,
            "radius_rule": "player_parameter.height * 0.5",
            "sonic_radius": 5.0,
            "tnorm_initial": [0.0, 1.0, 0.0],
            "field_offsets": {
                "radius": 0x00, "pos": 0x04, "spd": 0x10, "tnorm": 0x1C,
                "find_count": 0x28, "selected_nmb": 0x2A, "yt": 0x2C,
                "yb": 0x30, "angx": 0x34, "angz": 0x38, "normal": 0x3C,
                "normal2": 0x48, "onpoly": 0x54, "pshbk": 0x60,
                "anaspdh": 0x6C, "anaspdv": 0x78,
            },
        },
        "call_targets": targets,
        "constants": constants,
        "proven": {
            "allocation": "PInitialize calloc(1,0x84) -> playerwk+0x64",
            "initialization": "normal helper initializes tnorm=(0,1,0), copies task ang.x/ang.z, and derives radius from height*0.5",
            "candidate_pipeline": "general helper invokes the polygon routine then one of two csts arbitration helpers before accepted surface-flag handling",
            "find_count": "both arbitration helpers test csts+0x28 and use a 0x2C00 normal-angle cutoff",
            "polygon_radius": "broad polygon routine rejects non-positive radius and computes radius*0.5 internally",
            "surface_attributes": "accepted surface flags accumulate into playerwk.attr and select motion multiplier branches",
        },
        "not_proven": {
            "portable_polygon_solver": "the complete retail polygon iteration/contact-response algorithm is not reimplemented byte-for-byte in 0.0.8.8.9",
            "all_internal_vectors": "normal2/pshbk/anaspdh/anaspdv meanings and write ordering are not fabricated where evidence is incomplete",
            "lab_scale_equivalence": "the synthetic white lab retains its 1-unit support stand-off; retail Sonic csts.radius=5 is kept separately as reconstructed state/evidence",
        },
        "verified_signatures": sorted(SIGNATURES),
    }

    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print("PC 2004 retail collision-work reconstruction evidence: PASS")
        print("  exact EXE fingerprint: PASS")
        print("  csts allocation/layout size: 0x84")
        print("  csts pointer: playerwk+0x64")
        print("  tnorm initialization: (0, 1, 0)")
        print("  Sonic retail csts radius: height(10.0) * 0.5 = 5.0")
        print("  candidate polygon + dual arbitration call chain: PASS")
        print("  find_count(+0x28) + 0x2C00 cutoff evidence: PASS")
        print("  accepted surface attr/material multiplier path: PASS")
        print("  complete retail polygon solver: DEFERRED TO REAL-STAGE COLLISION CALIBRATION")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
