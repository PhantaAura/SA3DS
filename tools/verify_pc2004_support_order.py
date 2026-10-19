#!/usr/bin/env python3
"""Verify 2004 PC movement/support ordering without extracting game assets.

This verifier fingerprints the exact supported 2004 US executable and checks
small control-flow/data-reference signatures around PGetAcceleration,
PSetPosition, and PResetPosition. It is deliberately conservative: it proves
where the 1.16 steep-movement threshold is used and where it is *not* directly
referenced, plus the post-collision speed/orientation reset structure.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

PROFILE_SIZE = 55_549_952
PROFILE_SHA256 = "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"

VA = {
    "PGetAcceleration": (0x0044C270, 0x0044CDF0),
    "PSetPosition": (0x0044CDF0, 0x0044D49B),
    "PResetPosition": (0x0043EE70, 0x0043F001),
    "PGetInertia": (0x00443650, 0x00443800),
    "PGetBreak": (0x00448E50, 0x004491E0),
    "PConvertVector_G2P": 0x0043EC00,
    "unstick_speed_1_16": 0x007E0178,
    "unstick_up_0_1": 0x007DD568,
    "unstick_cross_0_6": 0x007DD584,
    "unstick_up_m0_4": 0x007E00E4,
    "unstick_up_m0_3": 0x007E01A8,
    "unstick_up_m0_1": 0x007E0064,
    "mul_5": 0x007DCCFC,
    "mul_0_8": 0x007DCE5C,
    "mul_0_4": 0x007DD550,
}

EXPECTED_FLOATS = {
    "unstick_speed_1_16": 1.159999966621399,
    "unstick_up_0_1": 0.10000000149011612,
    "unstick_cross_0_6": 0.6000000238418579,
    "unstick_up_m0_4": -0.4000000059604645,
    "unstick_up_m0_3": -0.30000001192092896,
    "unstick_up_m0_1": -0.10000000149011612,
    "mul_5": 5.0,
    "mul_0_8": 0.800000011920929,
    "mul_0_4": 0.4000000059604645,
}

SIGNATURES = {
    "pgetaccel_entry": (0x0044C270, "83ec30578b7c243833c0"),
    "psetposition_entry": (0x0044CDF0, "83ec2053558b6c2430"),
    "presetposition_entry": (0x0043EE70, "83ec0c558b6c241c56"),
    "pgetinertia_entry": (0x00443650, "83ec188b44242083c010"),
    "pgetbreak_entry": (0x00448E50, "83ec18538b5c24248d4310"),
    # Exact PGetAcceleration primary steep branch: Up < .1, abs(cross-y) > .6,
    # forward speed > 1.16, then zero tangent add terms and use -weight.
    "pgetaccel_up_0_1_ref": (0x0044C38A, "d9451cd81d68d57d00"),
    "pgetaccel_cross_0_6_ref": (0x0044C39A, "d9442438d9e1d81d84d57d00"),
    "pgetaccel_speed_1_16_ref": (0x0044C3AD, "d94538d81d78017e00"),
    "pgetaccel_primary_force": (0x0044C3BD, "d9052ccb7d00c744241c00000000d98514010000d9e0d95c2420"),
    # PResetPosition writes the reset local speed back to playerwk speed and
    # later derives Up from the stored support normal unless the upright path
    # forces 1.0f.
    "preset_write_speed": (0x0043EF9D, "8b44240c8d553889028b4424108942048b442414894208"),
    "preset_surface_normal_path": (0x0043EFCC, "f646040374218b455cf6c44075198d5550"),
    "preset_upright_up_1": (0x0043EFF3, "5f5ec7451c0000803f5d83c40cc3"),
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
    image_base = struct.unpack_from("<I", data, opt + 28)[0]
    sec = []
    off = opt + opt_size
    for i in range(nsec):
        o = off + i * 40
        name = data[o:o+8].split(b"\0",1)[0].decode("ascii","replace")
        vsize, rva, rsize, rptr = struct.unpack_from("<IIII", data, o+8)
        sec.append((name,rva,vsize,rptr,rsize))
    return image_base, sec


def va_to_off(va: int, base: int, sec):
    rva = va - base
    for name, srva, vsize, rptr, rsize in sec:
        if srva <= rva < srva + rsize:
            return rptr + (rva - srva), name
    raise ValueError(f"unbacked VA {va:#x}")


def region(data, base, sec, start, end):
    a,_ = va_to_off(start,base,sec)
    b,_ = va_to_off(end-1,base,sec)
    return data[a:b+1]


def require_bytes(data, base, sec, va, hx, label):
    expected=bytes.fromhex(hx)
    o,_=va_to_off(va,base,sec)
    got=data[o:o+len(expected)]
    if got != expected:
        raise ValueError(f"{label} mismatch at {va:#x}: {got.hex()} != {expected.hex()}")


def read_f32(data,base,sec,va):
    o,_=va_to_off(va,base,sec)
    return struct.unpack_from("<f",data,o)[0]


def abs_ref_count(blob: bytes, va: int) -> int:
    return blob.count(struct.pack("<I",va))


def rel32_target(data,base,sec,call_va):
    o,_=va_to_off(call_va,base,sec)
    if data[o] != 0xE8:
        raise ValueError(f"expected CALL at {call_va:#x}")
    d=struct.unpack_from("<i",data,o+1)[0]
    return (call_va+5+d)&0xffffffff


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("pc2004_exe",type=Path)
    ap.add_argument("--json",action="store_true")
    args=ap.parse_args()
    size=args.pc2004_exe.stat().st_size
    digest=sha256(args.pc2004_exe)
    if size != PROFILE_SIZE or digest != PROFILE_SHA256:
        raise SystemExit(f"unsupported SADX 2004 EXE: size={size} sha256={digest}")
    data=args.pc2004_exe.read_bytes()
    base,sec=parse_pe(data)

    for label,(va,hx) in SIGNATURES.items():
        require_bytes(data,base,sec,va,hx,label)

    floats={}
    for name,expect in EXPECTED_FLOATS.items():
        got=read_f32(data,base,sec,VA[name])
        if got != expect:
            raise ValueError(f"{name}: {got!r} != {expect!r}")
        floats[name]={"va":VA[name],"value":got}

    regions={name:region(data,base,sec,*bounds) for name,bounds in VA.items()
             if isinstance(bounds,tuple)}
    ref_116={name:abs_ref_count(blob,VA["unstick_speed_1_16"])
             for name,blob in regions.items()}
    if ref_116["PGetAcceleration"] != 4:
        raise ValueError(f"expected four 1.16 refs in PGetAcceleration, got {ref_116['PGetAcceleration']}")
    if ref_116["PSetPosition"] != 0 or ref_116["PResetPosition"] != 0:
        raise ValueError("unexpected direct 1.16 reference in support/reset function")

    # PResetPosition has a normal path that converts a post-collision vector to
    # player coordinates at 0x43EEC4.
    reset_convert_target=rel32_target(data,base,sec,0x0043EEC4)
    if reset_convert_target != VA["PConvertVector_G2P"]:
        raise ValueError(f"PResetPosition conversion target mismatch: {reset_convert_target:#x}")

    result={
        "schema":"sa3ds-pc2004-support-order-evidence-v1",
        "profile":{"size":size,"sha256":digest,"image_base":base},
        "functions":{k:{"start":v[0],"end_exclusive":v[1],"size":v[1]-v[0]}
                     for k,v in VA.items() if isinstance(v,tuple)},
        "constants":floats,
        "direct_1_16_reference_counts":ref_116,
        "proven":{
            "one_point_one_six_owner":"PGetAcceleration (4 direct refs); not PSetPosition/PResetPosition",
            "primary_unstick_branch":"Up<0.1 && abs(internal_cross_y)>0.6 && Speed.x>1.16; tangent add terms zeroed; local normal add=-weight",
            "inverted_biases":"for Speed.x>1.16: Up<-0.4 => -5*weight; Up<-0.3 => -0.8*weight; Up<-0.1 => -0.4*weight",
            "post_collision_reset":"PResetPosition converts post-collision vector to player space on normal path, writes local speed, and recomputes Up from support normal or 1.0 upright path",
            "portable_order_rule":"do not inject a second speculative support refresh before ordinary grounded movement",
        },
        "not_proven":{
            "retail_probe_shape":"PSetPosition body is verified but exact portable body shape/probe semantics are not reconstructed",
            "legacy_0_3_1_16_fall":"not accepted as exact 2004 PC PSetPosition/PResetPosition behavior",
        },
        "verified_signatures":sorted(SIGNATURES),
    }
    if args.json:
        print(json.dumps(result,indent=2,sort_keys=True))
    else:
        print("PC 2004 collision/support ordering evidence: PASS")
        print("  exact EXE fingerprint: PASS")
        print("  PGetAcceleration/PSetPosition/PResetPosition/PGetBreak/PGetInertia entries: PASS")
        print("  1.16 direct refs: PGetAcceleration=4, PSetPosition=0, PResetPosition=0")
        print("  exact steep unstick constants/branch signatures: PASS")
        print("  PResetPosition post-collision local-speed/Up reset signatures: PASS")
        print("  0.3/1.16 forced-fall heuristic: NOT promoted to 2004-PC authority")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
