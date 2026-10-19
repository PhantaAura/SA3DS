#!/usr/bin/env python3
"""Verify ordinary Sonic Spin Dash evidence in the exact SADX 2004 US EXE.

The script fingerprints the supported executable, verifies the Sonic action
entry/dispatch/state-machine signatures, decodes the three charge constants,
and confirms the action-4/action-5 movement call order. It emits metadata only;
it never extracts or writes game data.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

PROFILE = {
    "label": "SADX 2004 US",
    "size": 55_549_952,
    "sha256": "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74",
}

# Verified 2004 addresses.
VA = {
    "spindash_transition": 0x00495080,
    "spindash_entry": 0x00496EE0,
    "sonic_main": 0x0049A9B0,
    "sonic_action4": 0x0049AE2F,
    "sonic_action5": 0x0049AEBB,
    "sonic_action_table": 0x0049BFBC,
    "initial_min": 0x007DCB94,
    "max_compare": 0x007DCB34,
    "increment": 0x007DD550,
    "zero": 0x007DCB2C,
}

EXPECTED_FLOATS = {
    "initial_min": 2.0,
    "max_compare": 10.0,
    "increment": 0.4000000059604645,
    "zero": 0.0,
}

# Exact byte signatures selected to prove semantics without embedding any
# substantial code/data payload.
SIGNATURES = {
    "entry_action4": (0x00496EF8, "c60704"),
    "entry_ball_attack_status": (0x00496F00, "804f0505"),
    "entry_compare_forward_to_2": (0x00496F04, "d94638d81d94cb7d00"),
    "entry_reset_timer": (0x00496F0D, "66c7467e0000"),
    "entry_copy_forward_charge": (0x00496F1A, "8b56388916"),
    "entry_min_2_immediate": (0x00496F21, "c70600000040"),
    "main_ball_timer_increment": (0x0049AAF2, "668b530481e2000100006685d2740466ff457e"),
    "timeout_300": (0x00495154, "663d2c017c4e"),
    "release_action5": (0x004951BE, "c60705"),
    "release_charge_to_forward": (0x004951C1, "8b0e"),
    "release_write_forward": (0x004951D5, "894e38"),
    "release_reset_timer": (0x004951F3, "66c7467e0000"),
    "release_clear_charge": (0x004951F9, "c70600000000"),
    "held_compare_10": (0x00495224, "d906d81d34cb7d00"),
    "held_add_0_4": (0x00495233, "d906d80550d57d00d91e"),
}

EXPECTED_CALLS = {
    "action4": [
        (0x0049AE32, 0x0044BB60, "PGetRotation"),
        (0x0049AE3A, 0x00448E50, "PGetBreak"),
        (0x0049AE42, 0x00443F50, "PGetSpeed"),
        (0x0049AE4A, 0x0044CDF0, "PSetPosition"),
        (0x0049AE52, 0x0043EE70, "PResetPosition"),
    ],
    "action5": [
        (0x0049AEBE, 0x0044BB60, "PGetRotation"),
        (0x0049BBE5, 0x00443650, "PGetInertia"),
        (0x0049BBED, 0x00443F50, "PGetSpeed"),
        (0x0049BBF5, 0x0044CDF0, "PSetPosition"),
        (0x0049BBFD, 0x0043EE70, "PResetPosition"),
    ],
}


def digest(path: Path) -> str:
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


def require_bytes(data: bytes, image_base: int, sections, va: int, hx: str, label: str):
    expected = bytes.fromhex(hx)
    off, _ = va_to_off(va, image_base, sections)
    actual = data[off:off + len(expected)]
    if actual != expected:
        raise ValueError(f"{label} signature mismatch at {va:#x}")


def read_f32(data: bytes, image_base: int, sections, va: int):
    off, section = va_to_off(va, image_base, sections)
    return struct.unpack_from("<f", data, off)[0], section


def read_u32(data: bytes, image_base: int, sections, va: int):
    off, section = va_to_off(va, image_base, sections)
    return struct.unpack_from("<I", data, off)[0], section


def rel32_call_target(data: bytes, image_base: int, sections, call_va: int):
    off, _ = va_to_off(call_va, image_base, sections)
    if data[off] != 0xE8:
        raise ValueError(f"expected CALL rel32 at {call_va:#x}")
    disp = struct.unpack_from("<i", data, off + 1)[0]
    return (call_va + 5 + disp) & 0xFFFFFFFF


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("pc2004_exe", type=Path)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args()

    size = args.pc2004_exe.stat().st_size
    sha = digest(args.pc2004_exe)
    if size != PROFILE["size"] or sha != PROFILE["sha256"]:
        raise SystemExit(
            f"unsupported {PROFILE['label']}: size={size} sha256={sha}"
        )

    data = args.pc2004_exe.read_bytes()
    image_base, sections = parse_pe(data)

    for label, (va, hx) in SIGNATURES.items():
        require_bytes(data, image_base, sections, va, hx, label)

    constants = {}
    for name, expected in EXPECTED_FLOATS.items():
        value, section = read_f32(data, image_base, sections, VA[name])
        if value != expected:
            raise ValueError(f"{name}: {value!r} != {expected!r}")
        constants[name] = {"value": value, "va": VA[name], "section": section}

    # Action dispatch table entries 4 and 5 are direct code pointers.
    action4_ptr, _ = read_u32(data, image_base, sections, VA["sonic_action_table"] + 4 * 4)
    action5_ptr, _ = read_u32(data, image_base, sections, VA["sonic_action_table"] + 5 * 4)
    if action4_ptr != VA["sonic_action4"] or action5_ptr != VA["sonic_action5"]:
        raise ValueError("Sonic action table 4/5 mismatch")

    calls = {}
    for action, entries in EXPECTED_CALLS.items():
        calls[action] = []
        for call_va, expected_target, name in entries:
            target = rel32_call_target(data, image_base, sections, call_va)
            if target != expected_target:
                raise ValueError(
                    f"{action} {name}: call {call_va:#x} -> {target:#x}, "
                    f"expected {expected_target:#x}"
                )
            calls[action].append({"call_va": call_va, "target_va": target, "name": name})

    result = {
        "schema": "sa3ds-pc2004-sonic-spindash-evidence-v1",
        "profile": {**PROFILE, "image_base": image_base},
        "constants": constants,
        "state_semantics": {
            "entry_action": 4,
            "roll_action": 5,
            "initial_charge": "max(local_forward_speed, 2.0f); no 10.0 entry clamp",
            "held_charge": "if charge < 10.0f: charge += 0.4f; no post-add clamp",
            "timeout_frames": 300,
            "release": "Action=5; Speed.x=SpindashSpeed; timer=0; SpindashSpeed=0",
            "timeout": "timer=0; leave charge accumulator untouched; leave ball/attack state",
        },
        "action_dispatch": {
            "table_va": VA["sonic_action_table"],
            "action4_target": action4_ptr,
            "action5_target": action5_ptr,
        },
        "movement_call_order": calls,
        "verified_signatures": sorted(SIGNATURES),
    }

    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print("PC 2004 Sonic Spin Dash evidence: PASS")
        print(f"  profile: {sha}")
        print("  exact constants: initial-min=2.0 compare-max=10.0 increment=0.4 timeout=300")
        print("  entry: Action 4, charge=max(forward,2.0), no 10.0 entry clamp")
        print("  held: if charge<10.0 add 0.4 (no post-add clamp)")
        print("  release: Action 5, forward=charge, timer/charge cleared")
        print("  action 4 calls: PGetRotation -> PGetBreak -> PGetSpeed -> PSetPosition -> PResetPosition")
        print("  action 5 calls: PGetRotation -> PGetInertia -> PGetSpeed -> PSetPosition -> PResetPosition")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
