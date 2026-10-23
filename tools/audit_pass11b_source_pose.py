#!/usr/bin/env python3
"""Independent PC2004 ACTION pose oracle for the Pass11B audit.

This deliberately does not import the SA3DS CHRMODELS decoder or SA3C writer.
It reads the exact owner-owned DLL and the generated pointer-free package as
raw binary structures, then compares a selected source ACTION against the C pose
runtime via the Pass11 node-dump executable.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import math
from pathlib import Path
import struct
import subprocess
import tempfile


EXPECTED_SIZE = 7_647_232
EXPECTED_SHA256 = "76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5"
IMAGE_BASE = 0x10000000
ACTION_COUNT = 149
OBJECT_SIZE = 0x34
BAMS_TO_RAD = (2.0 * math.pi) / 65536.0
RAD_TO_BAMS = 65536.0 / (2.0 * math.pi)
INVALID_U16 = 0xFFFF
TOLERANCE = 4.0e-5


def u16(data: bytes, off: int) -> int:
    return struct.unpack_from("<H", data, off)[0]


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def round_away(value: float) -> float:
    return float(math.floor(value + 0.5) if value >= 0.0 else math.ceil(value - 0.5))


class RawPE:
    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        digest = hashlib.sha256(self.data).hexdigest()
        if len(self.data) != EXPECTED_SIZE or digest != EXPECTED_SHA256:
            raise ValueError(
                f"unsupported CHRMODELS source: size={len(self.data)} sha256={digest}"
            )
        pe = u32(self.data, 0x3C)
        if self.data[:2] != b"MZ" or self.data[pe:pe + 4] != b"PE\0\0":
            raise ValueError("source is not PE32")
        coff = pe + 4
        section_count = u16(self.data, coff + 2)
        optional_size = u16(self.data, coff + 16)
        opt = coff + 20
        if u16(self.data, opt) != 0x10B or u32(self.data, opt + 28) != IMAGE_BASE:
            raise ValueError("unexpected PE32 profile")
        self.header_size = u32(self.data, opt + 60)
        self.export_rva = u32(self.data, opt + 96)
        section_off = opt + optional_size
        self.sections = []
        for index in range(section_count):
            off = section_off + index * 40
            virtual_size, rva, raw_size, raw_off = struct.unpack_from(
                "<IIII", self.data, off + 8
            )
            self.sections.append((rva, max(virtual_size, raw_size), raw_size, raw_off))

    def rva_off(self, rva: int) -> int:
        if rva < self.header_size:
            return rva
        for start, span, raw_size, raw_off in self.sections:
            if start <= rva < start + span:
                delta = rva - start
                if delta >= raw_size:
                    raise ValueError(f"RVA {rva:#x} is zero-fill")
                return raw_off + delta
        raise ValueError(f"unmapped RVA {rva:#x}")

    def va_off(self, va: int) -> int:
        if va < IMAGE_BASE:
            raise ValueError(f"invalid VA {va:#x}")
        return self.rva_off(va - IMAGE_BASE)

    def read_va(self, va: int, size: int) -> bytes:
        off = self.va_off(va)
        if off + size > len(self.data):
            raise ValueError("VA read exceeds file")
        return self.data[off:off + size]

    def exports(self) -> dict[str, int]:
        off = self.rva_off(self.export_rva)
        fields = struct.unpack_from("<IIHHIIIIIII", self.data, off)
        function_count, name_count = fields[6], fields[7]
        functions_off = self.rva_off(fields[8])
        names_off = self.rva_off(fields[9])
        ordinals_off = self.rva_off(fields[10])
        functions = [u32(self.data, functions_off + i * 4) for i in range(function_count)]
        result = {}
        for i in range(name_count):
            name_off = self.rva_off(u32(self.data, names_off + i * 4))
            end = self.data.index(0, name_off)
            name = self.data[name_off:end].decode("ascii")
            ordinal = u16(self.data, ordinals_off + i * 2)
            result[name] = IMAGE_BASE + functions[ordinal]
        return result


def source_object(pe: RawPE, va: int) -> dict:
    raw = pe.read_va(va, OBJECT_SIZE)
    return {
        "va": va,
        "flags": u32(raw, 0),
        "position": struct.unpack_from("<fff", raw, 8),
        "rotation": struct.unpack_from("<iii", raw, 20),
        "scale": struct.unpack_from("<fff", raw, 32),
        "child": u32(raw, 44),
        "sibling": u32(raw, 48),
    }


def hierarchy(pe: RawPE, root: int) -> list[dict]:
    if not root:
        return []
    out, seen, stack = [], set(), [root]
    while stack:
        va = stack.pop()
        if va in seen:
            raise ValueError(f"cycle/repeated hierarchy node {va:#x}")
        seen.add(va)
        obj = source_object(pe, va)
        out.append(obj)
        if obj["sibling"]:
            stack.append(obj["sibling"])
        if obj["child"]:
            stack.append(obj["child"])
    return out


def source_actions(pe: RawPE) -> list[tuple[int, int] | None]:
    table = pe.exports()["___SONIC_ACTIONS"]
    pointers = struct.unpack("<" + "I" * ACTION_COUNT, pe.read_va(table, ACTION_COUNT * 4))
    actions = []
    for action_va in pointers:
        if not action_va:
            actions.append(None)
        else:
            actions.append(struct.unpack("<II", pe.read_va(action_va, 8)))
    return actions


def parse_sa3c(path: Path) -> tuple[bytes, dict[bytes, tuple[int, int, int]]]:
    data = path.read_bytes()
    if data[:4] != b"SA3C" or u16(data, 4) != 0 or u16(data, 6) != 0:
        raise ValueError("unsupported SA3C")
    if u32(data, 20) != len(data) or data[24:56].hex() != EXPECTED_SHA256:
        raise ValueError("SA3C size/source fingerprint mismatch")
    sections = {}
    for index in range(u32(data, 12)):
        off = 64 + index * 20
        tag = data[off:off + 4]
        count, data_off, stride, size = struct.unpack_from("<IIII", data, off + 4)
        if size != count * stride or data_off + size > len(data):
            raise ValueError(f"invalid SA3C section {tag!r}")
        sections[tag] = (count, data_off, stride)
    return data, sections


def source_key_vec(pe: RawPE, va: int, count: int, angles: bool) -> list[tuple]:
    if not count:
        if va:
            raise ValueError("nonnull key pointer with zero keys")
        return []
    raw = pe.read_va(va, count * 16)
    keys = []
    for index in range(count):
        if angles:
            frame, x, y, z = struct.unpack_from("<Iiii", raw, index * 16)
            keys.append((frame, f32(x * BAMS_TO_RAD), f32(y * BAMS_TO_RAD), f32(z * BAMS_TO_RAD)))
        else:
            frame, x, y, z = struct.unpack_from("<Ifff", raw, index * 16)
            keys.append((frame, x, y, z))
    return keys


def normalize_frame(frame: float, frame_count: int) -> float:
    value = f32(frame)
    if value < 0.0:
        return 0.0
    ceiling = f32(frame_count - 1)
    return ceiling if value > ceiling else value


def sample_keys(keys: list[tuple], frame_count: int, frame: float, angles: bool) -> tuple[float, float, float]:
    if not keys:
        return (0.0, 0.0, 0.0)
    if len(keys) == 1:
        return keys[0][1:]
    prev = next_index = None
    for index, key in enumerate(keys):
        key_frame = f32(key[0])
        if key_frame == frame:
            return key[1:]
        if key_frame < frame:
            prev = index
        if key_frame > frame:
            next_index = index
            break
    if prev is None:
        prev = len(keys) - 1
        af = f32(keys[prev][0] - frame_count)
    else:
        af = f32(keys[prev][0])
    if next_index is None:
        next_index = 0
        bf = f32(keys[0][0] + frame_count)
    else:
        bf = f32(keys[next_index][0])
    t = f32(f32(frame - af) / f32(bf - af)) if bf != af else 0.0
    result = []
    for component in range(1, 4):
        a = f32(keys[prev][component])
        b = f32(keys[next_index][component])
        if angles:
            a_bams = round_away(f32(a * f32(RAD_TO_BAMS)))
            b_bams = round_away(f32(b * f32(RAD_TO_BAMS)))
            value = round_away(f32(a_bams + f32(f32(b_bams - a_bams) * t)))
            result.append(f32(value * f32(BAMS_TO_RAD)))
        else:
            result.append(f32(a + f32(f32(b - a) * t)))
    return tuple(result)


def expected_pose(pe: RawPE, actions, objects: dict[int, dict], node_index: dict[int, int], frame: float, action_slot: int = 1):
    root_va, motion_va = actions[action_slot]
    local_hierarchy = hierarchy(pe, root_va)
    motion = pe.read_va(motion_va, 12)
    data_va, frame_count, packed = struct.unpack("<III", motion)
    type_bits = packed & 0xFFFF
    record_size = {3: 16, 7: 24}.get(type_bits)
    if record_size is None or motion_va - data_va != len(local_hierarchy) * record_size:
        raise ValueError(f"unexpected ACTION {action_slot} motion layout")
    pose = {}
    for va, obj in objects.items():
        pose[node_index[va]] = {
            "position": tuple(f32(v) for v in obj["position"]),
            "rotation": tuple(f32(v * BAMS_TO_RAD) for v in obj["rotation"]),
            "scale": tuple(f32(v) for v in obj["scale"]),
        }
    frame = normalize_frame(frame, frame_count)
    for local_i, obj in enumerate(local_hierarchy):
        raw = pe.read_va(data_va + local_i * record_size, record_size)
        if type_bits == 3:
            pos_va, ang_va, pos_count, ang_count = struct.unpack("<IIII", raw)
            scale_va = scale_count = 0
        else:
            pos_va, ang_va, scale_va, pos_count, ang_count, scale_count = struct.unpack("<IIIIII", raw)
        item = pose[node_index[obj["va"]]]
        if pos_count:
            item["position"] = sample_keys(source_key_vec(pe, pos_va, pos_count, False), frame_count, frame, False)
        if ang_count:
            item["rotation"] = sample_keys(source_key_vec(pe, ang_va, ang_count, True), frame_count, frame, True)
        if scale_count:
            item["scale"] = sample_keys(source_key_vec(pe, scale_va, scale_count, False), frame_count, frame, False)
    return node_index[root_va], frame_count, pose


def read_runtime_pose(dumper: Path, sa3c: Path, frame: float, output: Path, action_slot: int = 1) -> tuple[int, dict[int, dict]]:
    proc = subprocess.run(
        [str(dumper), str(sa3c), str(action_slot), format(frame, ".9g"), "0", "--nodes", str(output)],
        check=True,
        text=True,
        capture_output=True,
    )
    rows = list(csv.DictReader(output.open(newline="", encoding="utf-8")))
    if not rows:
        raise ValueError(f"pose dumper returned no rows: {proc.stdout}")
    parsed = {}
    for row in rows:
        index = int(row["node"])
        parsed[index] = {
            "root": int(row["root"]),
            "child": int(row["child"]),
            "sibling": int(row["sibling"]),
            "flags": int(row["flags"], 0),
            "position": tuple(float(row[k]) for k in ("local_x", "local_y", "local_z")),
            "rotation": tuple(float(row[k]) for k in ("rot_x_rad", "rot_y_rad", "rot_z_rad")),
            "scale": tuple(float(row[k]) for k in ("scale_x", "scale_y", "scale_z")),
        }
    return int(rows[0]["root"]), parsed


def compare_vec(label: str, expected, actual, frame: float, node: int) -> float:
    errors = [abs(a - b) for a, b in zip(expected, actual)]
    maximum = max(errors)
    if maximum > TOLERANCE:
        raise AssertionError(
            f"{label} mismatch frame={frame:g} node={node}: expected={expected} actual={actual} error={maximum}"
        )
    return maximum


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("chrmodels", type=Path)
    parser.add_argument("sa3c", type=Path)
    parser.add_argument("pose_dumper", type=Path)
    parser.add_argument("--action", type=int, default=1)
    args = parser.parse_args()

    pe = RawPE(args.chrmodels)
    actions = source_actions(pe)
    if not 0 <= args.action < len(actions) or not actions[args.action]:
        parser.error("--action must select a non-null owner ACTION")
    all_objects = {}
    for action in actions:
        if action:
            for obj in hierarchy(pe, action[0]):
                all_objects[obj["va"]] = obj
    node_vas = sorted(all_objects)
    node_index = {va: index for index, va in enumerate(node_vas)}
    package, sections = parse_sa3c(args.sa3c)
    node_count, node_off, node_stride = sections[b"NODE"]
    if node_count != len(node_vas):
        raise AssertionError(f"node inventory mismatch: source={len(node_vas)} SA3C={node_count}")

    # Topology/flags are compared before motion, so a coincidental pose match
    # cannot hide a wrong source-node mapping.
    for va in node_vas:
        index = node_index[va]
        raw = package[node_off + index * node_stride:node_off + (index + 1) * node_stride]
        obj = all_objects[va]
        expected_child = node_index[obj["child"]] if obj["child"] else INVALID_U16
        expected_sibling = node_index[obj["sibling"]] if obj["sibling"] else INVALID_U16
        if (u16(raw, 2), u16(raw, 4), u32(raw, 8)) != (
            expected_child, expected_sibling, obj["flags"]
        ):
            raise AssertionError(f"topology/flags mismatch at source node {va:#x}")

    root, frame_count, _ = expected_pose(pe, actions, all_objects, node_index, 0.0, args.action)
    frames = (-3.0, 0.0, 0.5, 8.0, frame_count / 2.0 + 0.25, frame_count - 1.0, frame_count + 5.0)
    maximum = 0.0
    comparisons = 0
    with tempfile.TemporaryDirectory(prefix="sa3ds-pass11b-") as tmp:
        for frame in frames:
            expected_root, _, expected = expected_pose(pe, actions, all_objects, node_index, frame, args.action)
            runtime_root, runtime = read_runtime_pose(
                args.pose_dumper, args.sa3c, frame, Path(tmp) / f"pose-{comparisons}.csv", args.action
            )
            if expected_root != root or runtime_root != root or len(runtime) != node_count:
                raise AssertionError("root/node-count mismatch")
            for node in range(node_count):
                actual = runtime[node]
                obj = all_objects[node_vas[node]]
                expected_child = node_index[obj["child"]] if obj["child"] else INVALID_U16
                expected_sibling = node_index[obj["sibling"]] if obj["sibling"] else INVALID_U16
                if (actual["child"], actual["sibling"], actual["flags"]) != (
                    expected_child, expected_sibling, obj["flags"]
                ):
                    raise AssertionError(f"runtime topology mismatch frame={frame:g} node={node}")
                for field in ("position", "rotation", "scale"):
                    maximum = max(maximum, compare_vec(field, expected[node][field], actual[field], frame, node))
                    comparisons += 3

    print(
        f"Pass11B raw CHRMODELS ACTION {args.action} -> C runtime pose oracle: PASS "
        f"({node_count} nodes, {len(frames)} frames, {comparisons} scalar comparisons, max_error={maximum:.9g})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
