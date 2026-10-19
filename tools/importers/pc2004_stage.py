#!/usr/bin/env python3
"""Strict Sonic Adventure DX 2004 stage/LandTable decoder.

This adapter is intentionally tied to the exact supported 2004 `sonic.exe`
profile.  It reads owner-supplied bytes and exposes source-neutral records for
SA3DS tooling.  It never ships Sega payloads.

0.0.9.1 evidence recovered from the exact executable:
  * LandTable pointer table base: 0x0097DA08
  * table index: level * 8 + act
  * Emerald Coast level id: 1, acts 0..2
  * LandTable size: 0x1C
  * LandTable +0x00: signed 16-bit collision-entry count
  * LandTable +0x0C: collision-list pointer
  * LandTable +0x18: texture-list pointer
  * collision entry size: 0x24
  * collision entry +0x18: NJS_OBJECT pointer
  * collision entry +0x20: surface/land flags consumed by retail code
  * collision objects use the same 0x34 NJS_OBJECT-compatible layout already
    proven for the character pipeline
  * attached models use the same validated 0x2C Basic-model layout

Fields not yet semantically proven keep neutral names rather than guessed Ninja
names.
"""
from __future__ import annotations

from dataclasses import dataclass, asdict
from pathlib import Path
import hashlib
import math
import struct
from typing import Iterable

from tools.importers.pc2004_chrmodels import PE32, DecodeError

EXPECTED_EXE_SIZE = 55_549_952
EXPECTED_EXE_SHA256 = "6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"
EXPECTED_IMAGE_BASE = 0x00400000

LANDTABLE_TABLE_VA = 0x0097DA08
LEVEL_STRIDE = 8
EMERALD_COAST_LEVEL_ID = 1
EMERALD_COAST_ACTS = (0, 1, 2)

LANDTABLE_SIZE = 0x1C
COLLISION_ENTRY_SIZE = 0x24
OBJECT_SIZE = 0x34
MODEL_SIZE = 0x2C
MESHSET_SIZE = 0x1C
MATERIAL_SIZE = 0x14


@dataclass(frozen=True)
class LandTableRecord:
    level_id: int
    act: int
    va: int
    collision_count: int
    aux_count: int
    load_flags: int
    load_refcount: int
    radius: float
    collision_list_va: int
    aux_list_va: int
    hook_va: int
    texture_list_va: int
    texture_archive: str | None


@dataclass(frozen=True)
class CollisionRecord:
    index: int
    center: tuple[float, float, float]
    radius: float
    field_10: int
    field_14: int
    object_va: int
    field_1c: int
    surface_flags: int


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
class ModelRecord:
    va: int
    points_va: int
    normals_va: int
    point_count: int
    meshsets_va: int
    materials_va: int
    meshset_count: int
    material_count: int
    center: tuple[float, float, float]
    radius: float
    original_texture_id: int


@dataclass(frozen=True)
class MaterialRecord:
    diffuse: int
    specular: int
    exponent: float
    texture_id: int
    flags: int


@dataclass(frozen=True)
class MeshSetRecord:
    va: int
    type_material: int
    primitive_class: int
    material_field: int
    polygon_count: int
    polygon_data_va: int
    normal_data_va: int
    color_data_va: int
    uv_data_va: int


class PC2004StageDecoder:
    def __init__(self, exe_path: Path):
        self.path = Path(exe_path)
        self.pe = PE32(self.path)
        digest = hashlib.sha256(self.pe.data).hexdigest()
        if len(self.pe.data) != EXPECTED_EXE_SIZE or digest != EXPECTED_EXE_SHA256:
            raise DecodeError(
                "unsupported 2004 sonic.exe: expected "
                f"size={EXPECTED_EXE_SIZE} sha256={EXPECTED_EXE_SHA256}; got "
                f"size={len(self.pe.data)} sha256={digest}"
            )
        if self.pe.image_base != EXPECTED_IMAGE_BASE:
            raise DecodeError(f"unexpected image base {self.pe.image_base:#x}")
        if not self.pe.is_mapped_va(LANDTABLE_TABLE_VA, 4):
            raise DecodeError("LandTable table base is not mapped")

    @property
    def sha256(self) -> str:
        return EXPECTED_EXE_SHA256

    def _cstring_va(self, va: int, max_len: int = 64) -> str:
        if not self.pe.is_mapped_va(va):
            raise DecodeError(f"unmapped cstring VA {va:#x}")
        off = self.pe.va_to_offset(va)
        raw = self.pe.data[off:off + max_len]
        end = raw.find(b"\0")
        if end < 0:
            raise DecodeError("unterminated short cstring")
        return raw[:end].decode("ascii", "strict")

    def resolve_texture_archive(self, texture_list_va: int) -> str | None:
        """Resolve a PVM archive stem from the exact executable's name/list pairs.

        We deliberately do not hard-code BEACH01/02/03.  The exact profile is
        scanned for a `<name_ptr, texture_list_ptr>` pair and the candidate name
        must be a short printable archive stem.
        """
        needle = struct.pack("<I", texture_list_va)
        start = 0
        candidates: list[str] = []
        while True:
            off = self.pe.data.find(needle, start)
            if off < 0:
                break
            start = off + 1
            if off < 4:
                continue
            name_va = struct.unpack_from("<I", self.pe.data, off - 4)[0]
            try:
                name = self._cstring_va(name_va, 32)
            except (DecodeError, UnicodeError):
                continue
            if name and all(ch.isupper() or ch.isdigit() or ch == "_" for ch in name):
                candidates.append(name)
        unique = sorted(set(candidates))
        if len(unique) == 1:
            return unique[0]
        return None

    def landtable_pointer(self, level_id: int, act: int) -> int:
        if not (0 <= level_id < 256 and 0 <= act < LEVEL_STRIDE):
            raise DecodeError("invalid level/act index")
        slot = level_id * LEVEL_STRIDE + act
        return self.pe.read_u32(LANDTABLE_TABLE_VA + slot * 4)

    def landtable(self, level_id: int, act: int) -> LandTableRecord:
        va = self.landtable_pointer(level_id, act)
        if not va:
            raise DecodeError(f"null LandTable level={level_id} act={act}")
        raw = self.pe.read_bytes(va, LANDTABLE_SIZE)
        (collision_count, aux_count, load_flags, load_refcount, radius,
         collision_list_va, aux_list_va, hook_va, texture_list_va) = struct.unpack(
            "<hhHHfIIII", raw
        )
        if collision_count <= 0:
            raise DecodeError("non-positive collision count")
        if aux_count < 0:
            raise DecodeError("negative auxiliary count")
        if not math.isfinite(radius) or radius <= 0.0:
            raise DecodeError("invalid LandTable radius")
        if not self.pe.is_mapped_va(collision_list_va, collision_count * COLLISION_ENTRY_SIZE):
            raise DecodeError("collision list does not fit mapped source")
        if texture_list_va and not self.pe.is_mapped_va(texture_list_va, 4):
            raise DecodeError("texture list pointer is not mapped")
        if aux_count and (not aux_list_va or not self.pe.is_mapped_va(aux_list_va, aux_count * 0x18)):
            raise DecodeError("auxiliary list does not fit mapped source")
        return LandTableRecord(
            level_id, act, va, collision_count, aux_count, load_flags,
            load_refcount, radius, collision_list_va, aux_list_va, hook_va,
            texture_list_va, self.resolve_texture_archive(texture_list_va),
        )

    def emerald_coast_landtables(self) -> list[LandTableRecord]:
        return [self.landtable(EMERALD_COAST_LEVEL_ID, act) for act in EMERALD_COAST_ACTS]

    def collision(self, table: LandTableRecord, index: int) -> CollisionRecord:
        if not (0 <= index < table.collision_count):
            raise DecodeError("collision index out of range")
        va = table.collision_list_va + index * COLLISION_ENTRY_SIZE
        raw = self.pe.read_bytes(va, COLLISION_ENTRY_SIZE)
        cx, cy, cz, radius, field_10, field_14, object_va, field_1c, surface_flags = struct.unpack(
            "<ffffIIIII", raw
        )
        if not all(math.isfinite(v) for v in (cx, cy, cz, radius)) or radius < 0.0:
            raise DecodeError(f"invalid collision bounds at index {index}")
        if not object_va or not self.pe.is_mapped_va(object_va, OBJECT_SIZE):
            raise DecodeError(f"collision entry {index} has invalid object pointer")
        return CollisionRecord(
            index, (cx, cy, cz), radius, field_10, field_14,
            object_va, field_1c, surface_flags,
        )

    def collisions(self, table: LandTableRecord) -> list[CollisionRecord]:
        return [self.collision(table, i) for i in range(table.collision_count)]

    def object(self, va: int) -> ObjectRecord:
        raw = self.pe.read_bytes(va, OBJECT_SIZE)
        flags, model_va = struct.unpack_from("<II", raw, 0)
        position = struct.unpack_from("<fff", raw, 8)
        rotation = struct.unpack_from("<iii", raw, 20)
        scale = struct.unpack_from("<fff", raw, 32)
        child_va, sibling_va = struct.unpack_from("<II", raw, 44)
        if not all(math.isfinite(v) for v in (*position, *scale)):
            raise DecodeError(f"non-finite object transform at {va:#x}")
        for ptr in (model_va, child_va, sibling_va):
            if ptr and not self.pe.is_mapped_va(ptr):
                raise DecodeError(f"object {va:#x} has unmapped pointer {ptr:#x}")
        return ObjectRecord(va, flags, model_va, position, rotation, scale, child_va, sibling_va)

    def model(self, va: int) -> ModelRecord:
        raw = self.pe.read_bytes(va, MODEL_SIZE)
        (points_va, normals_va, point_count, meshsets_va, materials_va,
         meshset_count, material_count, cx, cy, cz, radius,
         original_texture_id) = struct.unpack("<IIIIIHHffffi", raw)
        if point_count <= 0 or meshset_count <= 0:
            raise DecodeError(f"empty Basic model at {va:#x}")
        if point_count >= 65536:
            raise DecodeError(f"point count exceeds u16 source index range at {va:#x}")
        if not all(math.isfinite(v) for v in (cx, cy, cz, radius)) or radius < 0.0:
            raise DecodeError(f"invalid model bounds at {va:#x}")
        self.pe.read_bytes(points_va, point_count * 12)
        self.pe.read_bytes(normals_va, point_count * 12)
        self.pe.read_bytes(meshsets_va, meshset_count * MESHSET_SIZE)
        self.pe.read_bytes(materials_va, material_count * MATERIAL_SIZE)
        return ModelRecord(
            va, points_va, normals_va, point_count, meshsets_va, materials_va,
            meshset_count, material_count, (cx, cy, cz), radius,
            original_texture_id,
        )

    def meshset(self, model: ModelRecord, index: int) -> MeshSetRecord:
        if not (0 <= index < model.meshset_count):
            raise DecodeError("meshset index out of range")
        va = model.meshsets_va + index * MESHSET_SIZE
        raw = self.pe.read_bytes(va, MESHSET_SIZE)
        type_material, polygon_count = struct.unpack_from("<HH", raw, 0)
        p0, p1, p2, _p3, p4, _p5 = struct.unpack_from("<IIIIII", raw, 4)
        primitive_class = type_material & 0xC000
        # The high two bits are proven Basic primitive-class bits.  The
        # remaining 14 bits are preserved as an opaque stage material field;
        # unlike the Sonic character set, not every stage value is a direct
        # zero-based material index.
        material_field = type_material & 0x3FFF
        if primitive_class not in (0xC000, 0x4000):
            raise DecodeError(f"unsupported Basic primitive class {primitive_class:#x} at {va:#x}")
        if not p0 or not self.pe.is_mapped_va(p0, 2):
            raise DecodeError("mesh polygon stream is not mapped")
        return MeshSetRecord(
            va, type_material, primitive_class, material_field, polygon_count,
            p0, p1, p2, p4,
        )

    def material(self, model: ModelRecord, index: int) -> MaterialRecord:
        if not (0 <= index < model.material_count):
            raise DecodeError("material index out of range")
        raw = self.pe.read_bytes(model.materials_va + index * MATERIAL_SIZE, MATERIAL_SIZE)
        diffuse, specular, exponent, texture_id, flags = struct.unpack("<IIfII", raw)
        if not math.isfinite(exponent):
            raise DecodeError("non-finite stage material exponent")
        return MaterialRecord(diffuse, specular, exponent, texture_id, flags)

    def model_points(self, model: ModelRecord) -> list[tuple[float, float, float]]:
        raw = self.pe.read_bytes(model.points_va, model.point_count * 12)
        return [struct.unpack_from("<fff", raw, i * 12) for i in range(model.point_count)]

    def model_normals(self, model: ModelRecord) -> list[tuple[float, float, float]]:
        raw = self.pe.read_bytes(model.normals_va, model.point_count * 12)
        normals = [struct.unpack_from("<fff", raw, i * 12) for i in range(model.point_count)]
        if not all(math.isfinite(v) for n in normals for v in n):
            raise DecodeError("non-finite stage normal")
        return normals

    def mesh_corner_indices(self, model: ModelRecord, mesh: MeshSetRecord) -> list[tuple[bool, tuple[int, ...]]]:
        """Return source-point corner streams, preserving strip reverse bits.

        For quad lists each returned tuple is one four-corner quad and reverse is
        always false.  The representation is intentionally source-level; the
        neutral package builder performs deterministic triangulation.
        """
        out: list[tuple[bool, tuple[int, ...]]] = []
        if mesh.primitive_class == 0xC000:
            off = self.pe.va_to_offset(mesh.polygon_data_va)
            for _ in range(mesh.polygon_count):
                if off + 2 > len(self.pe.data):
                    raise DecodeError("strip header out of bounds")
                raw_count = struct.unpack_from("<H", self.pe.data, off)[0]
                off += 2
                count = raw_count & 0x7FFF
                reverse = bool(raw_count & 0x8000)
                if count < 3 or off + count * 2 > len(self.pe.data):
                    raise DecodeError("invalid strip corner stream")
                inds = struct.unpack_from("<" + "H" * count, self.pe.data, off)
                off += count * 2
                if any(i >= model.point_count for i in inds):
                    raise DecodeError("strip source index exceeds point count")
                out.append((reverse, tuple(inds)))
        else:
            raw = self.pe.read_bytes(mesh.polygon_data_va, mesh.polygon_count * 8)
            inds = struct.unpack("<" + "H" * (mesh.polygon_count * 4), raw)
            if any(i >= model.point_count for i in inds):
                raise DecodeError("quad source index exceeds point count")
            for i in range(mesh.polygon_count):
                out.append((False, tuple(inds[i * 4:(i + 1) * 4])))
        return out

    def mesh_uvs(self, model: ModelRecord, mesh: MeshSetRecord) -> list[tuple[int, int]] | None:
        corner_count = sum(len(corners) for _reverse, corners in self.mesh_corner_indices(model, mesh))
        if not mesh.uv_data_va:
            return None
        if not self.pe.is_mapped_va(mesh.uv_data_va, corner_count * 4):
            raise DecodeError("stage mesh UV stream is not mapped/too short")
        raw = self.pe.read_bytes(mesh.uv_data_va, corner_count * 4)
        return [struct.unpack_from("<hh", raw, i * 4) for i in range(corner_count)]

    def mesh_triangles(self, model: ModelRecord, mesh: MeshSetRecord) -> list[tuple[int, int, int]]:
        triangles: list[tuple[int, int, int]] = []
        if mesh.primitive_class == 0xC000:
            off = self.pe.va_to_offset(mesh.polygon_data_va)
            for _ in range(mesh.polygon_count):
                if off + 2 > len(self.pe.data):
                    raise DecodeError("strip header out of bounds")
                raw_count = struct.unpack_from("<H", self.pe.data, off)[0]
                off += 2
                count = raw_count & 0x7FFF
                reverse = bool(raw_count & 0x8000)
                if count < 3:
                    raise DecodeError("strip has fewer than three vertices")
                if off + count * 2 > len(self.pe.data):
                    raise DecodeError("strip indices out of bounds")
                inds = struct.unpack_from("<" + "H" * count, self.pe.data, off)
                off += count * 2
                if any(i >= model.point_count for i in inds):
                    raise DecodeError("strip source index exceeds point count")
                for t in range(count - 2):
                    a, b, c = inds[t:t + 3]
                    flip = ((t & 1) != 0) ^ reverse
                    triangles.append((b, a, c) if flip else (a, b, c))
        else:  # Basic quad list
            raw = self.pe.read_bytes(mesh.polygon_data_va, mesh.polygon_count * 8)
            inds = struct.unpack("<" + "H" * (mesh.polygon_count * 4), raw)
            if any(i >= model.point_count for i in inds):
                raise DecodeError("quad source index exceeds point count")
            for i in range(mesh.polygon_count):
                q = inds[i * 4:(i + 1) * 4]
                triangles.extend(((q[0], q[1], q[2]), (q[2], q[1], q[3])))
        return triangles

    def model_triangles(self, model: ModelRecord) -> list[tuple[int, int, int]]:
        out: list[tuple[int, int, int]] = []
        for i in range(model.meshset_count):
            out.extend(self.mesh_triangles(model, self.meshset(model, i)))
        return out

    def emerald_coast_inventory(self) -> dict:
        acts = []
        global_objects: set[int] = set()
        global_models: set[int] = set()
        primitive_counts: dict[str, int] = {}
        for table in self.emerald_coast_landtables():
            rows = self.collisions(table)
            act_objects: list[ObjectRecord] = []
            for row in rows:
                obj = self.object(row.object_va)
                if obj.child_va or obj.sibling_va:
                    raise DecodeError(
                        "0.0.9.1 EC collision-object assumption broken: "
                        f"root {obj.va:#x} has child/sibling"
                    )
                if not obj.model_va:
                    raise DecodeError("EC collision object has no model")
                act_objects.append(obj)
            act_model_vas = sorted({o.model_va for o in act_objects})
            models = [self.model(v) for v in act_model_vas]
            triangles = 0
            meshsets = 0
            materials = 0
            points = 0
            for model in models:
                points += model.point_count
                meshsets += model.meshset_count
                materials += model.material_count
                for mi in range(model.meshset_count):
                    ms = self.meshset(model, mi)
                    key = f"0x{ms.primitive_class:04x}"
                    primitive_counts[key] = primitive_counts.get(key, 0) + 1
                    triangles += len(self.mesh_triangles(model, ms))
            global_objects.update(o.va for o in act_objects)
            global_models.update(act_model_vas)
            acts.append({
                "act": table.act,
                "landtable_va": f"0x{table.va:08x}",
                "collision_list_va": f"0x{table.collision_list_va:08x}",
                "texture_list_va": f"0x{table.texture_list_va:08x}",
                "texture_archive": table.texture_archive,
                "collision_count": table.collision_count,
                "aux_count": table.aux_count,
                "load_flags": f"0x{table.load_flags:04x}",
                "load_refcount": table.load_refcount,
                "radius": table.radius,
                "unique_objects": len({o.va for o in act_objects}),
                "unique_models": len(act_model_vas),
                "source_points": points,
                "meshsets": meshsets,
                "materials": materials,
                "triangles": triangles,
            })
        return {
            "source_profile": "SADX_PC_2004_CANONICAL",
            "exe_sha256": self.sha256,
            "landtable_pointer_table_va": f"0x{LANDTABLE_TABLE_VA:08x}",
            "level_stride": LEVEL_STRIDE,
            "emerald_coast_level_id": EMERALD_COAST_LEVEL_ID,
            "acts": acts,
            "totals": {
                "collision_instances": sum(a["collision_count"] for a in acts),
                "unique_objects": len(global_objects),
                "unique_models": len(global_models),
                "source_points": sum(a["source_points"] for a in acts),
                "meshsets": sum(a["meshsets"] for a in acts),
                "materials": sum(a["materials"] for a in acts),
                "triangles": sum(a["triangles"] for a in acts),
                "primitive_classes": primitive_counts,
            },
        }
