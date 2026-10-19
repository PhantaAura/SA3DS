#!/usr/bin/env python3
"""Exact SADX 2004 CHRMODELS Sonic decoder.

This module is intentionally source-profile strict.  It decodes the owner-
supplied original 2004 CHRMODELS.DLL into validated neutral Python records.
It does not ship or embed Sega payloads.

Evidence status used by this decoder:
  CONFIRMED by exact DLL structure + executable/mod-loader witnesses:
    * Sonic export counts: MODELS=11, OBJECTS=79, ACTIONS=149
    * NJS_OBJECT-compatible 0x34 layout through child/sibling
    * ACTION { object*, motion* } at +0/+4
    * MOTION 12-byte header
    * type 3 motion data records are 16 bytes/node (pos/ang)
    * type 7 motion data records are 24 bytes/node (pos/ang/scale)
    * position/scale keys are {u32 frame, float x,y,z}
    * angle keys are {u32 frame, s32 x,y,z}
    * SADX model records are 0x2C bytes and self-consistent
    * mesh-set records are 0x1C bytes and, for Sonic, use field +0x14 as UV
    * 0xC000 primitive class is strip lists
    * 0x4000 primitive class is quad lists

The exact historical Ninja field *names* for a few mesh-set pointer slots are
not promoted where this Sonic dataset leaves them null.  They remain reserved.
"""
from __future__ import annotations

from dataclasses import dataclass, asdict
from pathlib import Path
import argparse
import hashlib
import json
import math
import struct
from typing import Iterable, Optional

EXPECTED_SIZE = 7_647_232
EXPECTED_SHA256 = "76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5"
EXPECTED_IMAGE_BASE = 0x10000000

SONIC_EXPORT_COUNTS = {
    "___SONIC_MODELS": 11,
    "___SONIC_OBJECTS": 79,
    "___SONIC_ACTIONS": 149,
}
SONIC_EXPORT_NAMES = (
    "___SONIC_MODELS",
    "___SONIC_OBJECTS",
    "___SONIC_ACTIONS",
    "___SONIC_MOTIONS",
)

OBJECT_SIZE = 0x34
MODEL_SIZE = 0x2C
MESHSET_SIZE = 0x1C
MATERIAL_SIZE = 0x14
ACTION_SIZE = 0x08
MOTION_SIZE = 0x0C
KEY_SIZE = 0x10


class DecodeError(ValueError):
    pass


class PE32:
    def __init__(self, path: Path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        if self.data[:2] != b"MZ":
            raise DecodeError("source is not MZ")
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe:pe + 4] != b"PE\0\0":
            raise DecodeError("source is not PE")
        coff = pe + 4
        self.machine, self.section_count = struct.unpack_from("<HH", self.data, coff)
        optional_size = struct.unpack_from("<H", self.data, coff + 16)[0]
        opt = coff + 20
        if struct.unpack_from("<H", self.data, opt)[0] != 0x10B:
            raise DecodeError("source is not PE32")
        self.image_base = struct.unpack_from("<I", self.data, opt + 28)[0]
        self.size_of_headers = struct.unpack_from("<I", self.data, opt + 60)[0]
        self.data_directories = [
            struct.unpack_from("<II", self.data, opt + 96 + i * 8)
            for i in range(16)
        ]
        self.sections = []
        sec = opt + optional_size
        for i in range(self.section_count):
            off = sec + i * 40
            name = self.data[off:off + 8].rstrip(b"\0").decode("ascii", "replace")
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", self.data, off + 8)
            self.sections.append({
                "name": name,
                "virtual_size": virtual_size,
                "rva": rva,
                "raw_size": raw_size,
                "raw_offset": raw_offset,
            })

    def sha256(self) -> str:
        return hashlib.sha256(self.data).hexdigest()

    def validate_exact_chrmodels(self) -> None:
        if len(self.data) != EXPECTED_SIZE or self.sha256() != EXPECTED_SHA256:
            raise DecodeError(
                "unsupported CHRMODELS source: expected exact original 2004 DLL "
                f"size={EXPECTED_SIZE} sha256={EXPECTED_SHA256}; got "
                f"size={len(self.data)} sha256={self.sha256()}"
            )
        if self.image_base != EXPECTED_IMAGE_BASE:
            raise DecodeError(f"unexpected image base {self.image_base:#x}")

    def rva_to_offset(self, rva: int) -> int:
        if rva < self.size_of_headers:
            return rva
        for s in self.sections:
            span = max(s["virtual_size"], s["raw_size"])
            if s["rva"] <= rva < s["rva"] + span:
                delta = rva - s["rva"]
                if delta >= s["raw_size"]:
                    raise DecodeError(f"RVA {rva:#x} lies in zero-fill")
                return s["raw_offset"] + delta
        raise DecodeError(f"unmapped RVA {rva:#x}")

    def va_to_offset(self, va: int) -> int:
        if va == 0:
            raise DecodeError("null VA")
        return self.rva_to_offset(va - self.image_base)

    def cstring_rva(self, rva: int) -> str:
        off = self.rva_to_offset(rva)
        end = self.data.find(b"\0", off)
        if end < 0:
            raise DecodeError("unterminated export name")
        return self.data[off:end].decode("ascii", "replace")

    def named_exports(self) -> dict[str, int]:
        export_rva, _export_size = self.data_directories[0]
        if not export_rva:
            raise DecodeError("PE has no export directory")
        off = self.rva_to_offset(export_rva)
        (
            _characteristics, _timestamp, _major, _minor,
            _name_rva, base, number_functions, number_names,
            functions_rva, names_rva, ordinals_rva,
        ) = struct.unpack_from("<IIHHIIIIIII", self.data, off)
        functions_off = self.rva_to_offset(functions_rva)
        names_off = self.rva_to_offset(names_rva)
        ordinals_off = self.rva_to_offset(ordinals_rva)
        functions = [
            struct.unpack_from("<I", self.data, functions_off + i * 4)[0]
            for i in range(number_functions)
        ]
        out: dict[str, int] = {}
        for i in range(number_names):
            name_rva = struct.unpack_from("<I", self.data, names_off + i * 4)[0]
            ordinal_index = struct.unpack_from("<H", self.data, ordinals_off + i * 2)[0]
            if ordinal_index >= len(functions):
                raise DecodeError("export ordinal index out of range")
            name = self.cstring_rva(name_rva)
            out[name] = self.image_base + functions[ordinal_index]
        return out

    def read_u16(self, va: int, delta: int = 0) -> int:
        return struct.unpack_from("<H", self.data, self.va_to_offset(va) + delta)[0]

    def read_u32(self, va: int, delta: int = 0) -> int:
        return struct.unpack_from("<I", self.data, self.va_to_offset(va) + delta)[0]

    def read_s32(self, va: int, delta: int = 0) -> int:
        return struct.unpack_from("<i", self.data, self.va_to_offset(va) + delta)[0]

    def read_f32(self, va: int, delta: int = 0) -> float:
        return struct.unpack_from("<f", self.data, self.va_to_offset(va) + delta)[0]

    def read_bytes(self, va: int, size: int) -> bytes:
        off = self.va_to_offset(va)
        end = off + size
        if end > len(self.data):
            raise DecodeError("read extends beyond file")
        return self.data[off:end]

    def is_mapped_va(self, va: int, size: int = 1) -> bool:
        try:
            off = self.va_to_offset(va)
            return 0 <= off <= len(self.data) - size
        except DecodeError:
            return False


@dataclass(frozen=True)
class ObjectRecord:
    va: int
    flags: int
    model_va: int
    position: tuple[float, float, float]
    rotation_bams: tuple[int, int, int]
    scale: tuple[float, float, float]
    child_va: int
    sibling_va: int


@dataclass(frozen=True)
class MaterialRecord:
    diffuse: int
    specular: int
    exponent: float
    texture_id: int
    flags: int


@dataclass(frozen=True)
class MeshSetSummary:
    type_material: int
    primitive_class: int
    material_id: int
    polygon_count: int
    index_count: int
    triangle_count: int
    reverse_strip_count: int
    uv_count: int


@dataclass(frozen=True)
class ModelSummary:
    va: int
    point_count: int
    meshset_count: int
    material_count: int
    center: tuple[float, float, float]
    radius: float
    original_texture_id: int
    triangle_count: int
    meshsets: tuple[MeshSetSummary, ...]


@dataclass(frozen=True)
class MotionSummary:
    va: int
    data_va: int
    frame_count: int
    type_bits: int
    # Legacy 0.0.9.0C field name retained for API compatibility.  This is
    # Ninja's high-16 pair-count/function word, not an interpolation enum.
    interpolation: int
    node_count: int
    position_channel_count: int
    rotation_channel_count: int
    scale_channel_count: int
    position_key_count: int
    rotation_key_count: int
    scale_key_count: int

    @property
    def pair_function_word(self) -> int:
        return self.interpolation

    @property
    def pair_count(self) -> int:
        return self.interpolation & 0x000F

    @property
    def interpolation_function_bits(self) -> int:
        return self.interpolation & 0x00C0


@dataclass(frozen=True)
class ActionSummary:
    index: int
    va: int
    root_object_va: int
    motion_va: int
    hierarchy_node_count: int
    motion: MotionSummary


class SonicDecoder:
    def __init__(self, path: Path):
        self.pe = PE32(path)
        self.pe.validate_exact_chrmodels()
        self.exports = self.pe.named_exports()
        missing = [name for name in SONIC_EXPORT_NAMES if name not in self.exports]
        if missing:
            raise DecodeError("missing Sonic exports: " + ", ".join(missing))

    def pointer_table(self, name: str, count: int) -> list[int]:
        base = self.exports[name]
        raw = self.pe.read_bytes(base, count * 4)
        return list(struct.unpack("<" + "I" * count, raw))

    def sonic_models_table(self) -> list[int]:
        return self.pointer_table("___SONIC_MODELS", 11)

    def sonic_objects_table(self) -> list[int]:
        return self.pointer_table("___SONIC_OBJECTS", 79)

    def sonic_actions_table(self) -> list[int]:
        return self.pointer_table("___SONIC_ACTIONS", 149)

    def object(self, va: int) -> ObjectRecord:
        raw = self.pe.read_bytes(va, OBJECT_SIZE)
        flags, model = struct.unpack_from("<II", raw, 0)
        position = struct.unpack_from("<fff", raw, 8)
        rotation = struct.unpack_from("<iii", raw, 20)
        scale = struct.unpack_from("<fff", raw, 32)
        child, sibling = struct.unpack_from("<II", raw, 44)
        values = (*position, *scale)
        if not all(math.isfinite(v) for v in values):
            raise DecodeError(f"non-finite object transform at {va:#x}")
        for ptr in (model, child, sibling):
            if ptr and not self.pe.is_mapped_va(ptr):
                raise DecodeError(f"object {va:#x} contains unmapped pointer {ptr:#x}")
        return ObjectRecord(va, flags, model, position, rotation, scale, child, sibling)

    def hierarchy(self, root_va: int) -> list[ObjectRecord]:
        if not root_va:
            return []
        result: list[ObjectRecord] = []
        seen: set[int] = set()

        # Stack order preserves recursive object/child/sibling preorder.
        stack = [root_va]
        while stack:
            va = stack.pop()
            if not va:
                continue
            if va in seen:
                raise DecodeError(f"cycle/repeated hierarchy node at {va:#x}")
            seen.add(va)
            obj = self.object(va)
            result.append(obj)
            if obj.sibling_va:
                stack.append(obj.sibling_va)
            if obj.child_va:
                stack.append(obj.child_va)
        return result

    def material(self, va: int) -> MaterialRecord:
        raw = self.pe.read_bytes(va, MATERIAL_SIZE)
        diffuse, specular = struct.unpack_from("<II", raw, 0)
        exponent = struct.unpack_from("<f", raw, 8)[0]
        texture_id, flags = struct.unpack_from("<II", raw, 12)
        if not math.isfinite(exponent):
            raise DecodeError("non-finite material exponent")
        return MaterialRecord(diffuse, specular, exponent, texture_id, flags)

    def _parse_strip_mesh(self, indices_va: int, strip_count: int, point_count: int) -> tuple[int, int, int]:
        off = self.pe.va_to_offset(indices_va)
        index_count = 0
        triangle_count = 0
        reverse_count = 0
        for _ in range(strip_count):
            if off + 2 > len(self.pe.data):
                raise DecodeError("strip header out of bounds")
            raw_count = struct.unpack_from("<H", self.pe.data, off)[0]
            off += 2
            count = raw_count & 0x7FFF
            if count < 3:
                raise DecodeError(f"invalid strip vertex count {count}")
            if raw_count & 0x8000:
                reverse_count += 1
            byte_count = count * 2
            if off + byte_count > len(self.pe.data):
                raise DecodeError("strip indices out of bounds")
            indices = struct.unpack_from("<" + "H" * count, self.pe.data, off)
            if any(index >= point_count for index in indices):
                raise DecodeError("strip index exceeds model point count")
            off += byte_count
            index_count += count
            triangle_count += count - 2
        return index_count, triangle_count, reverse_count

    def _parse_quad_mesh(self, indices_va: int, quad_count: int, point_count: int) -> tuple[int, int, int]:
        # Exact Sonic witness: the sole 0x4000-class mesh stores nbMesh * 4
        # contiguous u16 indices and its UV array begins immediately afterward.
        index_count = quad_count * 4
        raw = self.pe.read_bytes(indices_va, index_count * 2)
        indices = struct.unpack("<" + "H" * index_count, raw)
        if any(index >= point_count for index in indices):
            raise DecodeError("quad index exceeds model point count")
        return index_count, quad_count * 2, 0

    def model(self, va: int) -> ModelSummary:
        raw = self.pe.read_bytes(va, MODEL_SIZE)
        points_va, normals_va, point_count, meshsets_va, materials_va = struct.unpack_from("<IIIII", raw, 0)
        meshset_count, material_count = struct.unpack_from("<HH", raw, 20)
        center = struct.unpack_from("<fff", raw, 24)
        radius = struct.unpack_from("<f", raw, 36)[0]
        original_texture_id = struct.unpack_from("<i", raw, 40)[0]

        if point_count <= 0 or point_count > 100_000:
            raise DecodeError(f"implausible model point count {point_count}")
        if meshset_count > 4096 or material_count > 4096:
            raise DecodeError("implausible model table count")
        if not all(math.isfinite(x) for x in (*center, radius)) or radius < 0:
            raise DecodeError("invalid model bounds")
        if not self.pe.is_mapped_va(points_va, point_count * 12):
            raise DecodeError("model point array is unmapped")
        if normals_va and not self.pe.is_mapped_va(normals_va, point_count * 12):
            raise DecodeError("model normal array is unmapped")
        if material_count and not self.pe.is_mapped_va(materials_va, material_count * MATERIAL_SIZE):
            raise DecodeError("model material array is unmapped")
        if meshset_count and not self.pe.is_mapped_va(meshsets_va, meshset_count * MESHSET_SIZE):
            raise DecodeError("model meshset array is unmapped")

        # Validate points/normals are finite without retaining the asset payload.
        for array_va in (points_va, normals_va):
            if not array_va:
                continue
            arr = self.pe.read_bytes(array_va, point_count * 12)
            for i in range(point_count):
                xyz = struct.unpack_from("<fff", arr, i * 12)
                if not all(math.isfinite(v) for v in xyz):
                    raise DecodeError(f"non-finite model vector at {array_va + i*12:#x}")

        for i in range(material_count):
            self.material(materials_va + i * MATERIAL_SIZE)

        mesh_summaries: list[MeshSetSummary] = []
        total_triangles = 0
        for i in range(meshset_count):
            mraw = self.pe.read_bytes(meshsets_va + i * MESHSET_SIZE, MESHSET_SIZE)
            type_material, polygon_count = struct.unpack_from("<HH", mraw, 0)
            ptrs = struct.unpack_from("<IIIIII", mraw, 4)
            indices_va = ptrs[0]
            uv_va = ptrs[4]
            # Sonic leaves the other four pointer fields zero. Keep the
            # evidence boundary explicit instead of inventing semantics.
            reserved = (ptrs[1], ptrs[2], ptrs[3], ptrs[5])
            if any(reserved):
                raise DecodeError("unexpected nonzero reserved Sonic meshset pointer")
            primitive = type_material & 0xC000
            material_id = type_material & 0x3FFF
            if material_id >= material_count:
                raise DecodeError("meshset material index out of range")
            if not indices_va or not uv_va:
                raise DecodeError("Sonic meshset missing indices or UV array")
            if primitive == 0xC000:
                index_count, triangle_count, reverse_count = self._parse_strip_mesh(
                    indices_va, polygon_count, point_count)
            elif primitive == 0x4000:
                index_count, triangle_count, reverse_count = self._parse_quad_mesh(
                    indices_va, polygon_count, point_count)
            else:
                raise DecodeError(f"unsupported Sonic primitive class {primitive:#06x}")
            if not self.pe.is_mapped_va(uv_va, index_count * 4):
                raise DecodeError("meshset UV array is unmapped/too short")
            # UVs are signed 16-bit pairs in this exact dataset.
            uvraw = self.pe.read_bytes(uv_va, index_count * 4)
            _ = struct.unpack("<" + "h" * (index_count * 2), uvraw)
            summary = MeshSetSummary(
                type_material=type_material,
                primitive_class=primitive,
                material_id=material_id,
                polygon_count=polygon_count,
                index_count=index_count,
                triangle_count=triangle_count,
                reverse_strip_count=reverse_count,
                uv_count=index_count,
            )
            mesh_summaries.append(summary)
            total_triangles += triangle_count

        return ModelSummary(
            va=va,
            point_count=point_count,
            meshset_count=meshset_count,
            material_count=material_count,
            center=center,
            radius=radius,
            original_texture_id=original_texture_id,
            triangle_count=total_triangles,
            meshsets=tuple(mesh_summaries),
        )

    @staticmethod
    def motion_record_size(type_bits: int) -> int:
        # This exact Sonic dataset uses type 3 (position+angle) and type 7
        # (position+angle+scale) only.
        if type_bits == 3:
            return 16
        if type_bits == 7:
            return 24
        raise DecodeError(f"unsupported Sonic motion type {type_bits:#x}")

    def _validate_vec3_keys(self, va: int, count: int, frame_count: int, label: str) -> None:
        if count == 0:
            if va != 0:
                raise DecodeError(f"{label}: nonnull key pointer with zero count")
            return
        if not va or not self.pe.is_mapped_va(va, count * KEY_SIZE):
            raise DecodeError(f"{label}: invalid key array")
        raw = self.pe.read_bytes(va, count * KEY_SIZE)
        last = -1
        for i in range(count):
            frame, x, y, z = struct.unpack_from("<Ifff", raw, i * KEY_SIZE)
            if frame < last:
                raise DecodeError(f"{label}: keys are not ordered")
            if frame_count and frame >= frame_count:
                raise DecodeError(f"{label}: frame {frame} outside motion domain {frame_count}")
            if not all(math.isfinite(v) for v in (x, y, z)):
                raise DecodeError(f"{label}: non-finite key value")
            last = frame

    def _validate_angle_keys(self, va: int, count: int, frame_count: int, label: str) -> None:
        if count == 0:
            if va != 0:
                raise DecodeError(f"{label}: nonnull key pointer with zero count")
            return
        if not va or not self.pe.is_mapped_va(va, count * KEY_SIZE):
            raise DecodeError(f"{label}: invalid key array")
        raw = self.pe.read_bytes(va, count * KEY_SIZE)
        last = -1
        for i in range(count):
            frame, _x, _y, _z = struct.unpack_from("<Iiii", raw, i * KEY_SIZE)
            if frame < last:
                raise DecodeError(f"{label}: keys are not ordered")
            if frame_count and frame >= frame_count:
                raise DecodeError(f"{label}: frame {frame} outside motion domain {frame_count}")
            last = frame

    def motion(self, va: int, expected_node_count: int) -> MotionSummary:
        raw = self.pe.read_bytes(va, MOTION_SIZE)
        data_va, frame_count, packed = struct.unpack("<III", raw)
        type_bits = packed & 0xFFFF
        interpolation = (packed >> 16) & 0xFFFF
        record_size = self.motion_record_size(type_bits)
        if not data_va or data_va >= va:
            raise DecodeError("motion data pointer is null or not before header")
        span = va - data_va
        if span % record_size:
            raise DecodeError("motion data span is not record-aligned")
        node_count = span // record_size
        if node_count != expected_node_count:
            raise DecodeError(
                f"motion node count {node_count} != hierarchy count {expected_node_count}"
            )
        if frame_count <= 0 or frame_count > 1_000_000:
            raise DecodeError("implausible motion frame count")
        if not self.pe.is_mapped_va(data_va, node_count * record_size):
            raise DecodeError("motion data array is unmapped")

        pos_channels = rot_channels = scale_channels = 0
        pos_keys = rot_keys = scale_keys = 0
        for node in range(node_count):
            nraw = self.pe.read_bytes(data_va + node * record_size, record_size)
            if type_bits == 3:
                pos_va, ang_va, pos_count, ang_count = struct.unpack("<IIII", nraw)
                scl_va = scl_count = 0
            else:
                pos_va, ang_va, scl_va, pos_count, ang_count, scl_count = struct.unpack("<IIIIII", nraw)
            self._validate_vec3_keys(pos_va, pos_count, frame_count, f"motion {va:#x} node {node} pos")
            self._validate_angle_keys(ang_va, ang_count, frame_count, f"motion {va:#x} node {node} ang")
            self._validate_vec3_keys(scl_va, scl_count, frame_count, f"motion {va:#x} node {node} scale")
            if pos_count:
                pos_channels += 1
                pos_keys += pos_count
            if ang_count:
                rot_channels += 1
                rot_keys += ang_count
            if scl_count:
                scale_channels += 1
                scale_keys += scl_count

        return MotionSummary(
            va=va,
            data_va=data_va,
            frame_count=frame_count,
            type_bits=type_bits,
            interpolation=interpolation,
            node_count=node_count,
            position_channel_count=pos_channels,
            rotation_channel_count=rot_channels,
            scale_channel_count=scale_channels,
            position_key_count=pos_keys,
            rotation_key_count=rot_keys,
            scale_key_count=scale_keys,
        )

    def action(self, index: int, va: int) -> Optional[ActionSummary]:
        if not va:
            return None
        raw = self.pe.read_bytes(va, ACTION_SIZE)
        root_va, motion_va = struct.unpack("<II", raw)
        hierarchy = self.hierarchy(root_va)
        motion = self.motion(motion_va, len(hierarchy))
        return ActionSummary(index, va, root_va, motion_va, len(hierarchy), motion)

    def actions(self) -> list[Optional[ActionSummary]]:
        table = self.sonic_actions_table()
        return [self.action(i, va) for i, va in enumerate(table)]

    def report(self) -> dict:
        model_table = self.sonic_models_table()
        object_table = self.sonic_objects_table()
        action_table = self.sonic_actions_table()
        action_summaries = self.actions()

        roots = sorted({a.root_object_va for a in action_summaries if a})
        root_hierarchies = {root: self.hierarchy(root) for root in roots}
        referenced_model_vas = sorted({
            obj.model_va
            for hierarchy in root_hierarchies.values()
            for obj in hierarchy
            if obj.model_va
        })
        model_summaries = [self.model(va) for va in referenced_model_vas]
        normal_root = next(a.root_object_va for a in action_summaries if a and a.index == 0)
        normal_hierarchy = root_hierarchies[normal_root]
        normal_model_vas = sorted({o.model_va for o in normal_hierarchy if o.model_va})
        normal_models = [self.model(va) for va in normal_model_vas]

        motions = [a.motion for a in action_summaries if a]
        type_distribution: dict[str, int] = {}
        # Keep the old report key for 0.0.9.0C regression compatibility.
        # Correct names are emitted alongside it below.
        interpolation_distribution: dict[str, int] = {}
        pair_count_distribution: dict[str, int] = {}
        interpolation_function_bits_distribution: dict[str, int] = {}
        node_distribution: dict[str, int] = {}
        for m in motions:
            type_distribution[str(m.type_bits)] = type_distribution.get(str(m.type_bits), 0) + 1
            interpolation_distribution[str(m.interpolation)] = interpolation_distribution.get(str(m.interpolation), 0) + 1
            pair_count_distribution[str(m.pair_count)] = pair_count_distribution.get(str(m.pair_count), 0) + 1
            interpolation_function_bits_distribution[str(m.interpolation_function_bits)] = interpolation_function_bits_distribution.get(str(m.interpolation_function_bits), 0) + 1
            node_distribution[str(m.node_count)] = node_distribution.get(str(m.node_count), 0) + 1

        return {
            "schema": "sa3ds-pc2004-sonic-decode-report-v1",
            "source": {
                "size": len(self.pe.data),
                "sha256": self.pe.sha256(),
                "image_base": self.pe.image_base,
            },
            "exports": {name: self.exports[name] for name in SONIC_EXPORT_NAMES},
            "tables": {
                "models_entries": len(model_table),
                "models_nonnull": sum(bool(v) for v in model_table),
                "objects_entries": len(object_table),
                "objects_nonnull": sum(bool(v) for v in object_table),
                "actions_entries": len(action_table),
                "actions_nonnull": sum(bool(v) for v in action_table),
                "null_action_indices": [i for i, v in enumerate(action_table) if not v],
            },
            "hierarchies": {
                "distinct_action_roots": len(roots),
                "root_node_counts": {f"0x{root:08X}": len(root_hierarchies[root]) for root in roots},
                "normal_action0_root": f"0x{normal_root:08X}",
                "normal_action0_nodes": len(normal_hierarchy),
            },
            "models": {
                "distinct_action_hierarchy_models": len(model_summaries),
                "normal_hierarchy_distinct_models": len(normal_models),
                "normal_hierarchy_points": sum(m.point_count for m in normal_models),
                "normal_hierarchy_triangles": sum(m.triangle_count for m in normal_models),
                "all_action_hierarchy_points": sum(m.point_count for m in model_summaries),
                "all_action_hierarchy_triangles": sum(m.triangle_count for m in model_summaries),
                "exported_models_nonnull": sum(bool(v) for v in model_table),
            },
            "motions": {
                "count": len(motions),
                "type_distribution": type_distribution,
                "interpolation_distribution": interpolation_distribution,
                "pair_function_word_distribution": interpolation_distribution,
                "pair_count_distribution": pair_count_distribution,
                "interpolation_function_bits_distribution": interpolation_function_bits_distribution,
                "node_count_distribution": node_distribution,
                "frame_min": min(m.frame_count for m in motions),
                "frame_max": max(m.frame_count for m in motions),
                "position_channels": sum(m.position_channel_count for m in motions),
                "rotation_channels": sum(m.rotation_channel_count for m in motions),
                "scale_channels": sum(m.scale_channel_count for m in motions),
                "position_keys": sum(m.position_key_count for m in motions),
                "rotation_keys": sum(m.rotation_key_count for m in motions),
                "scale_keys": sum(m.scale_key_count for m in motions),
            },
            "evidence_boundary": {
                "confirmed": [
                    "exact supported original 2004 CHRMODELS DLL fingerprint",
                    "Sonic 11/79/149 table entry counts",
                    "0x34 object transform/child/sibling layout",
                    "8-byte action object/motion layout",
                    "12-byte motion header: low16 animation type bits + high16 Ninja pair-count/function word",
                    "type-3 16-byte and type-7 24-byte per-node motion layouts",
                    "16-byte frame+vec3-float and frame+angle3 key layouts",
                    "high16 low nibble is pair count; 0x00C0 function bits are clear for all 146 Sonic motions => linear",
                    "0x2C SADX model record and finite point/normal arrays",
                    "0x1C Sonic meshset record, strip and quad primitive encodings",
                    "20-byte materials and signed-16 UV pair storage",
                ],
                "not_yet_promoted": [
                    "historical names/semantics of unused meshset pointer slots",
                    "exact BAMS-to-render-axis/order semantics",
                    "exact runtime animation ACTION index semantic names",
                    "full retail interpolation implementation beyond the source-supported linear Sonic set",
                    "texture archive sampling/material flag runtime semantics",
                ],
            },
        }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("chrmodels", type=Path)
    ap.add_argument("--json", type=Path)
    args = ap.parse_args()
    dec = SonicDecoder(args.chrmodels)
    report = dec.report()
    text = json.dumps(report, indent=2)
    if args.json:
        args.json.write_text(text + "\n", encoding="utf-8")
    print(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
