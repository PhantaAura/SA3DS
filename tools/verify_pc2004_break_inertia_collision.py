#!/usr/bin/env python3
"""Verify the 0.0.8.8.8 PC-2004 Break/Inertia + collision-layout evidence.

This script fingerprints the exact supported 2004 US sonic.exe and checks only
small instruction/data signatures. It does not extract or redistribute game
content. The goal is to keep the portable arithmetic kernel tied to exact
machine-code evidence while clearly separating the still-opaque retail body
probe implementation in PSetPosition/PResetPosition.
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
    "PInitialize": (0x00442750, 0x00442993),
    "PGetInertia": (0x00443650, 0x004437F8),
    "PGetSpeed": (0x00443F50, 0x004441AB),
    "PGetBreak": (0x00448E50, 0x004491DD),
    "PSetPosition": (0x0044CDF0, 0x0044D49B),
    "PResetPosition": (0x0043EE70, 0x0043F001),
    "const_zero": 0x007DCB2C,
    "const_one": 0x007DCB10,
    "inertia_neg8": 0x007DF1B4,
    "inertia_five_sevenths": 0x007E0108,
    "inertia_status_drag": 0x007E010C,
    "inertia_up_limit": 0x007E0110,
    "pgetspeed_idle_factor": 0x007E0124,
    "collision_height_half": 0x007DCC98,
    "air_turn_transformed": 0x007DFFF0,
    "air_turn_old_local": 0x007DD568,
}

EXPECTED = {
    "const_zero": ("f", 0.0),
    "const_one": ("f", 1.0),
    "inertia_neg8": ("f", -8.0),
    "inertia_five_sevenths": ("f", 0.7142857313156128),
    "inertia_status_drag": ("f", 0.00019999999494757503),
    "inertia_up_limit": ("d", 0.98),
    "pgetspeed_idle_factor": ("f", 0.8700000047683716),
    "collision_height_half": ("f", 0.5),
    "air_turn_transformed": ("f", 0.8999999761581421),
    "air_turn_old_local": ("f", 0.10000000149011612),
}

SIGNATURES = {
    "air_acceleration_general_turn_call": (0x0044BB22, "e82981ffff"),
    "general_turn_air_status_branch": (0x00443CCE, "8a460483c408a803754c"),
    "general_turn_air_forward_mix": (0x00443CD8, "d94424085dd80df0ff7d00d903d80d68d57d00dec1d91b"),
    "general_turn_air_normal_mix": (0x00443CF0, "d9473cd80d68d57d00d9442404d80df0ff7d00dec1d95f3c"),
    "general_turn_air_side_mix": (0x00443D08, "d94740d80d68d57d00d9442408d80df0ff7d00dec1d95f40"),
    "pinitialize_entry": (0x00442750, "518b44240c538b58208b4024555657"),
    "pinitialize_param_stride_0x84": (0x00442773, "69f684000000"),
    "pinitialize_copy_0x84_to_pwp_0x9c": (0x00442781, "8dbd9c000000b921000000f3a5"),
    "pinitialize_alloc_collision_work_0x84": (0x0044279A, "68840000006a01"),
    "pinitialize_store_collision_work_pwp_0x64": (0x004427AA, "894564"),
    "pinitialize_default_accel_multiplier_1": (0x00442944, "be0000803f89773c"),

    "pgetbreak_entry": (0x00448E50, "83ec18538b5c24248d4310"),
    "pgetbreak_air_resist_x": (0x00448EE5, "d987f000000083c408d84f38"),
    "pgetbreak_air_resist_y": (0x00448EF9, "d987f4000000d84f3c"),
    "pgetbreak_air_resist_z": (0x00448F0A, "d987f8000000d84f40"),
    "pgetbreak_run_break_x": (0x00448F2B, "d987e4000000d84b3c"),
    "pgetbreak_lim_frict_x": (0x00448FAB, "d98704010000d84b3cd84c2410"),
    "pgetbreak_grd_frict_x": (0x00448FF8, "d987fc000000d84b3cd84c2410"),
    "pgetbreak_grd_frict_z": (0x00449118, "d98700010000d84b3cd84c2410"),
    "pgetbreak_write_addvelocity": (0x004491CB, "83c72c890f895704894708"),

    "pgetinertia_entry": (0x00443650, "83ec188b44242083c010"),
    "pgetinertia_speed_vs_run": (0x004436D4, "d9463883c408d89ec4000000"),
    "pgetinertia_up_negative_neg8": (0x004436E7, "d9461cd81d2ccb7d00dfe0f6c4057a0ed944240cd80db4f17d00"),
    "pgetinertia_pending_impulse_scale": (0x0044370B, "d986140100008b4664d80d08017e00"),
    "pgetinertia_status_0x100": (0x0044377A, "668b4f0481e1000100006685c9741f"),
    "pgetinertia_up_0_98": (0x00443789, "d9461cdc1d10017e00"),
    "pgetinertia_status_drag_0_0002": (0x00443799, "d94638d80d0c017e00d86c2408"),
    "pgetinertia_normal_drag": (0x004437BD, "d986f400000083c62cd84e10"),
    "pgetinertia_side_drag": (0x004437D5, "d986cc000000d84e14"),

    "pgetspeed_entry": (0x00443F50, "83ec10538b5c24185657"),
    "pgetspeed_add_forward": (0x00443F5E, "d9472c8b4738d84738"),
    "pgetspeed_idle_factor_ref": (0x00443F70, "d987c0000000d80d24017e00"),
    "pgetspeed_forward_zero_cross_store": (0x00443F9E, "897738"),
    # Local-Z integration uses the same 0.87*jog threshold. Positive and
    # negative branches snap speed.z to zero when damping crosses the band.
    "pgetspeed_side_jog_deadband": (
        0x0044403A,
        "d94740d94734d84740d95c240c8b4c240cd8152ccb7d00894f40dfe0f6c441"
        "7535d8542428dfe0f6c4057a0bd854240cdfe0f6c4447b6bd85c2428dfe0f6"
        "c4017565d944240cd85c2428dfe0f6c4057a56897740eb51d8152ccb7d00dfe0"
        "f6c4057a42d9442428d9e0d95c2428d8542428dfe0f6c441750bd854240cdfe0"
        "f6c4447b1fd85c2428dfe0f6c4417a19d944240cd85c2428dfe0f6c441750a89"
        "7740eb05897740ddd8",
    ),

    "psetposition_entry": (0x0044CDF0, "83ec2053558b6c2430"),
    "psetposition_roll_surface_flags": (0x0044CE00, "8b4e5c578b7c243489464c894648894644894e6089465c"),
    "psetposition_collision_helper_general": (0x0044D39C, "565557e85cc1ffff"),
    "psetposition_collision_helper_alt": (0x0044D3A6, "565557e86289ffff"),
    "collision_helper_general_workptr": (0x0044951C, "8b6864894c246c668b4b0466894810806304fe"),
    "collision_helper_general_height_load": (0x00449555, "d98010010000"),
    "collision_helper_general_height_half_mul": (0x00449563, "d80d98cc7d00"),
    "collision_helper_alt_workptr": (0x00445D2E, "8b68645666895010806304fe"),
    "collision_helper_alt_height_load": (0x00445DC7, "d98610010000"),
    "collision_helper_alt_height_half_mul": (0x00445DD1, "d80d98cc7d00"),
    "presetposition_entry": (0x0043EE70, "83ec0c558b6c241c56"),
    "presetposition_write_local_speed": (0x0043EF9D, "8b44240c8d553889028b4424108942048b442414894208"),
}

# Calls in the ordinary grounded Sonic action path.  Checking the decoded
# rel32 targets makes the per-frame ordering evidence explicit instead of
# relying on nearby bytes or names imported from a secondary decompilation.
FLAT_TICK_CALLS = (
    (0x0049AD34, 0x0044BB60, "pre_acceleration_movement"),
    (0x0049AD3C, 0x0044C270, "PGetAcceleration"),
    (0x0049AD44, 0x00443F50, "PGetSpeed"),
    (0x0049AD4C, 0x0044CDF0, "PSetPosition"),
    (0x0049AD54, 0x0043EE70, "PResetPosition"),
)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def parse_pe(data: bytes):
    peoff = struct.unpack_from("<I", data, 0x3C)[0]
    if data[:2] != b"MZ" or data[peoff:peoff + 4] != b"PE\0\0":
        raise ValueError("not PE32")
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


def va_to_off(va: int, base: int, sections):
    rva = va - base
    for name, srva, vsize, rptr, rsize in sections:
        if srva <= rva < srva + rsize:
            return rptr + (rva - srva), name
    raise ValueError(f"VA {va:#x} not backed by file data")


def require_bytes(data: bytes, base: int, sections, va: int, hx: str, label: str):
    expected = bytes.fromhex(hx)
    off, _ = va_to_off(va, base, sections)
    actual = data[off:off + len(expected)]
    if actual != expected:
        raise ValueError(f"{label} mismatch at {va:#x}: {actual.hex()} != {expected.hex()}")


def require_rel32_call(data: bytes, base: int, sections, va: int, target: int, label: str):
    off, _ = va_to_off(va, base, sections)
    if data[off] != 0xE8:
        raise ValueError(f"{label} is not a rel32 call at {va:#x}")
    displacement = struct.unpack_from("<i", data, off + 1)[0]
    actual = va + 5 + displacement
    if actual != target:
        raise ValueError(f"{label} target mismatch at {va:#x}: {actual:#x} != {target:#x}")


def read_number(data: bytes, base: int, sections, va: int, kind: str):
    off, section = va_to_off(va, base, sections)
    if kind == "f":
        return struct.unpack_from("<f", data, off)[0], section
    if kind == "d":
        return struct.unpack_from("<d", data, off)[0], section
    raise ValueError(kind)


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
    base, sections = parse_pe(data)
    for label, (va, hx) in SIGNATURES.items():
        require_bytes(data, base, sections, va, hx, label)
    for va, target, label in FLAT_TICK_CALLS:
        require_rel32_call(data, base, sections, va, target, label)

    constants = {}
    for name, (kind, expected) in EXPECTED.items():
        value, section = read_number(data, base, sections, VA[name], kind)
        if value != expected:
            raise ValueError(f"{name}: {value!r} != {expected!r}")
        constants[name] = {"va": VA[name], "value": value, "section": section}

    result = {
        "schema": "sa3ds-pc2004-break-inertia-collision-evidence-v1",
        "profile": {"size": size, "sha256": digest, "image_base": base},
        "functions": {
            k: {"start": v[0], "end_exclusive": v[1], "size": v[1] - v[0]}
            for k, v in VA.items() if isinstance(v, tuple)
        },
        "constants": constants,
        "proven": {
            "player_parameter_copy": "PInitialize copies exactly 0x84 bytes into playerwk+0x9C using a 0x84-byte character stride",
            "collision_work_allocation": "PInitialize allocates a separate 0x84-byte work block and stores its pointer at playerwk+0x64",
            "default_acceleration_multiplier": "normal PInitialize path writes 1.0f to motionwk2+0x3C",
            "PGetBreak": "uses air_resist/air_resist_y/air_resist_z, run_break, lim_frict, grd_frict, grd_frict_z and zero-cross guards before writing playerwk AddVelocity",
            "PGetInertia": "uses run_speed/Up, -8.0 inverted branch, grounded pending-impulse path, status 0x100 Up<=0.98 0.0002 drag branch, and normal air_resist terms",
            "PGetSpeed": "integrates AddVelocity into Speed, contains forward zero-cross protection, and applies the 0.87*jog_speed deadband to forward and local-Z speed",
            "air_turn": "0x443C50 status&3==0 uses independently stored f32 constants: transformed local velocity * 0.9 + old local velocity * 0.1 on all three axes",
            "support_order": "exact calls at 0x0049AD34..0x0049AD54 execute movement, PGetAcceleration, PGetSpeed, PSetPosition, then PResetPosition",
            "surface_flag_rollover": "PSetPosition copies playerwk SurfaceFlags to SurfaceFlags_Old and clears current flags/scratch collision vectors before rebuilding contact state",
            "collision_work_setup": "both checked PSetPosition collision-helper paths consume the separate playerwk+0x64 work block and derive a scalar from player height * 0.5f before broad collision processing",
        },
        "not_proven": {
            "portable_body_probe_shape": "the exact helper-level PSetPosition/PResetPosition body probes are still not reconstructed into neutral 3DS geometry queries",
            "pending_collision_impulse_semantics": "PGetInertia consumes/clears an opaque vector through the playerwk+0x64 work block; static Movement Lab specialization assumes no pending dynamic-object impulse",
            "all_PGetSpeed_status_paths": "forward and local-Z deadbands are promoted; remaining action/status-dependent branches stay evidence-only",
        },
        "verified_signatures": sorted(SIGNATURES),
        "flat_tick_calls": [
            {"call_va": va, "target_va": target, "label": label}
            for va, target, label in FLAT_TICK_CALLS
        ],
    }

    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print("PC 2004 Break/Inertia + collision-layout evidence: PASS")
        print("  exact EXE fingerprint: PASS")
        print("  PInitialize 0x84 parameter copy + 0x84 collision-work allocation: PASS")
        print("  default motionwk2 acceleration multiplier 1.0f: PASS")
        print("  PGetBreak field/branch signatures: PASS")
        print("  PGetInertia field/branch signatures: PASS")
        print("  PGetSpeed AddVelocity integration/forward + local-Z deadband evidence: PASS")
        print("  general-turn airborne 0.9 transformed + 0.1 old-local vector: PASS")
        print("  grounded Sonic movement -> acceleration -> speed -> position -> reset order: PASS")
        print("  PSetPosition surface-flag rollover + helper work/height setup: PASS")
        print("  PResetPosition local-speed write: PASS")
        print("  exact portable body-probe shape: STILL UNRESOLVED")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
