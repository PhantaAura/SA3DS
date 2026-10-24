#!/usr/bin/env python3
"""Verify the exact PC2004 instructions used by retail trace capture v2."""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

SHA = "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"
SIZE = 55_549_952
SIGS = {
    "sonic_main_entry": (0x0049A9B0, "83ec188b44241c538b582055568b70248b2e33c9891d60dfab03892d64dfab"),
    "sonic_act1_arguments_and_call": (0x0049AA31, "555653e817c5ffff83c40c8b433885"),
    "post_act1_dispatch_region": (0x0049AAF0, "33d2668b530481e2000100006685d2740466ff457e8a033c4b7c0b3c52c605455dc503017e07c605455dc503000fbe0383f8570f87db120000ff24"),
    "animation_effective_id": (0x00460C94, "6683be2001000002530fb79e2401000075070fb79e26010000"),
    "animation_frame": (0x00460EAB, "8b8e300100008b96400100008bebc1e5048b042a5150"),
    "landing_world_y_velocity": (0x0049796E, "e87dd6ffff83c40485c074148b4f1c894d148b572489551cd94708e9fb0200008b471c8945148b4f24894d1c8bc38bcdc6450002"),
}


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def parse_pe(data: bytes):
    peoff = struct.unpack_from("<I", data, 0x3C)[0]
    if data[peoff:peoff + 4] != b"PE\0\0":
        raise ValueError("not a PE executable")
    coff = peoff + 4
    _, count, _, _, _, opt_size, _ = struct.unpack_from("<HHIIIHH", data, coff)
    opt = coff + 20
    if struct.unpack_from("<H", data, opt)[0] != 0x10B:
        raise ValueError("expected PE32 executable")
    base = struct.unpack_from("<I", data, opt + 28)[0]
    sections = []
    section_table = opt + opt_size
    for index in range(count):
        offset = section_table + index * 40
        virtual_size, rva, raw_size, raw_pointer = struct.unpack_from("<IIII", data, offset + 8)
        sections.append((rva, virtual_size, raw_pointer, raw_size))
    return base, sections


def va_to_offset(va: int, base: int, sections) -> int:
    rva = va - base
    for section_rva, _virtual_size, raw_pointer, raw_size in sections:
        if section_rva <= rva < section_rva + raw_size:
            return raw_pointer + rva - section_rva
    raise ValueError(f"unbacked VA {va:#x}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("exe", type=Path)
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    if args.exe.stat().st_size != SIZE or digest(args.exe) != SHA:
        raise SystemExit("unsupported PC2004 executable identity")
    data = args.exe.read_bytes()
    base, sections = parse_pe(data)
    for label, (va, expected_hex) in SIGS.items():
        expected = bytes.fromhex(expected_hex)
        offset = va_to_offset(va, base, sections)
        if data[offset:offset + len(expected)] != expected:
            raise SystemExit(f"{label} mismatch at {va:#x}")
    result = {
        "sha256": SHA,
        "sonic_main": "0x0049a9b0",
        "sonic_act1": "0x00496f50",
        "tick_phase": "post_sonic_main",
        "motion_work_world_velocity_offsets": [4, 8, 12],
        "animation_transition_state_offset": 0x120,
        "animation_requested_id_offset": 0x124,
        "animation_transition_source_id_offset": 0x126,
        "animation_frame_offset": 0x130,
        "signature_count": len(SIGS),
    }
    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print("PC2004 retail capture evidence: PASS (post-main tick, velocity +4/+8/+C, animation state/frame)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
