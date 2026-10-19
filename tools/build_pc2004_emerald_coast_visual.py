#!/usr/bin/env python3
"""Build owner-side Emerald Coast visual packages from exact PC2004 sources.

Outputs contain Sega-derived geometry/material/texture pixels and are therefore
owner-side generated artifacts.  They MUST NOT be included in SA3DS code-only
checkpoint archives.
"""
from __future__ import annotations

from pathlib import Path
from zipfile import ZipFile, BadZipFile
import argparse
import hashlib
import json
import math
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools.importers.pc2004_chrmodels import DecodeError
from tools.importers.pc2004_stage import PC2004StageDecoder, EMERALD_COAST_LEVEL_ID
from tools.importers.pc2004_pvm_archive import PVMArchive, PVMArchiveError
from tools import sa3dspak, sa3t
from tools.importers.pc2004_set import (
    list_set_sources, build_set_chunk, parse_set_chunk, SetDecodeError,
)
from tools.importers.pc2004_cam import (
    list_cam_sources, build_cam_chunk, parse_cam_chunk, CamDecodeError,
)

VIS_MAGIC = b"SA3DVIS\0"
VIS_MAJOR = 1
VIS_MINOR = 0
VIS_HEADER = struct.Struct("<8sHHIIIIIIIIII")
# first_mesh, mesh_count, first_vertex, vertex_count, first_index, index_count,
# center.xyz, radius, original_texture_id, reserved
VIS_MODEL = struct.Struct("<IIIIIIffffiI")
# model_index, material_index, first_index, index_count, primitive,
# type_material, polygon_count, flags
VIS_MESH = struct.Struct("<HHIIHHII")
# model_index, collision_index, object_flags, surface_flags, field10, field14,
# field1c, reserved, position.xyz, rotation.xyz(rad), scale.xyz, center.xyz,radius
VIS_INSTANCE = struct.Struct("<IIIIIIIIfffffffffffff")
# pos.xyz normal.xyz uv.xy
VIS_VERTEX = struct.Struct("<ffffffff")

MAT_MAGIC = b"SA3DMATL"
MAT_MAJOR = 1
MAT_MINOR = 0
MAT_HEADER = struct.Struct("<8sHHIIII")
MAT_RECORD = struct.Struct("<IIfII")

MESH_FLAG_HAS_UV = 1 << 0
INVALID_U16 = 0xFFFF
UV_DIVISOR = 255.0
BAMS_TO_RAD = (2.0 * math.pi) / 65536.0


def align16(v: int) -> int:
    return (v + 15) & ~15


def read_source(source: Path, name: str) -> bytes:
    if source.is_dir():
        matches = [p for p in source.rglob("*") if p.is_file() and p.name.upper() == name.upper()]
        if len(matches) != 1:
            raise ValueError(f"expected exactly one {name} in {source}, found {len(matches)}")
        return matches[0].read_bytes()
    if source.is_file() and source.suffix.lower() == ".zip":
        with ZipFile(source) as z:
            matches = [n for n in z.namelist() if not n.endswith("/") and Path(n).name.upper() == name.upper()]
            if len(matches) != 1:
                raise ValueError(f"expected exactly one {name} in {source}, found {len(matches)}")
            return z.read(matches[0])
    raise ValueError("system source must be a directory or ZIP")


def triangulate_strip(corners: list[int], reverse: bool) -> list[int]:
    out: list[int] = []
    for t in range(len(corners) - 2):
        a, b, c = corners[t:t + 3]
        flip = ((t & 1) != 0) ^ reverse
        out.extend((b, a, c) if flip else (a, b, c))
    return out


def triangulate_quad(q: list[int]) -> list[int]:
    if len(q) != 4:
        raise ValueError("quad must contain four corners")
    return [q[0], q[1], q[2], q[2], q[1], q[3]]


def build_material_chunk(materials: list[tuple[int, int, float, int, int]]) -> bytes:
    record_off = align16(MAT_HEADER.size)
    size = record_off + len(materials) * MAT_RECORD.size
    blob = bytearray(size)
    MAT_HEADER.pack_into(blob, 0, MAT_MAGIC, MAT_MAJOR, MAT_MINOR, len(materials), MAT_RECORD.size, record_off, size)
    for i, row in enumerate(materials):
        MAT_RECORD.pack_into(blob, record_off + i * MAT_RECORD.size, *row)
    return bytes(blob)


def parse_material_chunk(blob: bytes) -> dict:
    if len(blob) < MAT_HEADER.size:
        raise DecodeError("truncated SA3DMATL")
    magic, major, minor, count, stride, off, size = MAT_HEADER.unpack_from(blob)
    if magic != MAT_MAGIC or (major, minor) != (MAT_MAJOR, MAT_MINOR):
        raise DecodeError("bad SA3DMATL magic/version")
    if stride != MAT_RECORD.size or size != len(blob) or off < MAT_HEADER.size or off + count * stride != len(blob):
        raise DecodeError("bad SA3DMATL layout")
    return {"count": count, "record_size": stride, "bytes": size}


def build_visual_chunk(dec: PC2004StageDecoder, act: int, texture_count: int):
    table = dec.landtable(EMERALD_COAST_LEVEL_ID, act)
    rows = dec.collisions(table)
    objects = [(r, dec.object(r.object_va)) for r in rows]
    model_vas = sorted({o.model_va for _r, o in objects if o.model_va})
    models = [dec.model(v) for v in model_vas]
    model_index = {m.va: i for i, m in enumerate(models)}

    material_keys: list[tuple[int, int, float, int, int]] = []
    material_index: dict[tuple[int, int, float, int, int], int] = {}
    model_material_map: dict[int, list[int]] = {}
    for model in models:
        row: list[int] = []
        for mi in range(model.material_count):
            m = dec.material(model, mi)
            if not 0 <= m.texture_id < texture_count:
                raise DecodeError(
                    f"act {act} model {model.va:#x} material {mi}: texture id {m.texture_id} "
                    f"outside archive count {texture_count}"
                )
            key = (m.diffuse, m.specular, m.exponent, m.texture_id, m.flags)
            if key not in material_index:
                material_index[key] = len(material_keys)
                material_keys.append(key)
            row.append(material_index[key])
        model_material_map[model.va] = row

    vertices: list[tuple[float, float, float, float, float, float, float, float]] = []
    indices: list[int] = []
    meshes: list[tuple[int, int, int, int, int, int, int, int]] = []
    model_records: list[tuple] = []
    uv_min = [None, None]
    uv_max = [None, None]
    meshes_without_uv = 0

    for model in models:
        points = dec.model_points(model)
        normals = dec.model_normals(model)
        first_mesh = len(meshes)
        first_vertex = len(vertices)
        first_index = len(indices)
        vmap: dict[tuple[int, int | None, int | None], int] = {}

        for mesh_i in range(model.meshset_count):
            mesh = dec.meshset(model, mesh_i)
            corner_streams = dec.mesh_corner_indices(model, mesh)
            uvs = dec.mesh_uvs(model, mesh)
            has_uv = uvs is not None
            if has_uv:
                uv_flat = uvs
            else:
                uv_flat = [(0, 0)] * sum(len(c) for _r, c in corner_streams)
                meshes_without_uv += 1

            if model.material_count:
                if mesh.material_field >= model.material_count:
                    raise DecodeError("stage mesh material index outside model material array")
                global_material = model_material_map[model.va][mesh.material_field]
            else:
                if mesh.material_field != 0:
                    raise DecodeError("material-less model uses nonzero mesh material field")
                global_material = INVALID_U16

            mesh_first_index = len(indices)
            cursor = 0
            for reverse, source_corners in corner_streams:
                corner_vertices: list[int] = []
                for source_index in source_corners:
                    raw_u, raw_v = uv_flat[cursor]
                    cursor += 1
                    key = (source_index, raw_u if has_uv else None, raw_v if has_uv else None)
                    if key not in vmap:
                        if len(vertices) >= INVALID_U16:
                            raise DecodeError("act visual vertex bank exceeds u16 range")
                        pos = points[source_index]
                        nrm = normals[source_index]
                        u = raw_u / UV_DIVISOR if has_uv else 0.0
                        v = raw_v / UV_DIVISOR if has_uv else 0.0
                        if has_uv:
                            uv_min[0] = raw_u if uv_min[0] is None else min(uv_min[0], raw_u)
                            uv_min[1] = raw_v if uv_min[1] is None else min(uv_min[1], raw_v)
                            uv_max[0] = raw_u if uv_max[0] is None else max(uv_max[0], raw_u)
                            uv_max[1] = raw_v if uv_max[1] is None else max(uv_max[1], raw_v)
                        vmap[key] = len(vertices)
                        vertices.append((*pos, *nrm, u, v))
                    corner_vertices.append(vmap[key])
                if mesh.primitive_class == 0xC000:
                    indices.extend(triangulate_strip(corner_vertices, reverse))
                elif mesh.primitive_class == 0x4000:
                    indices.extend(triangulate_quad(corner_vertices))
                else:
                    raise DecodeError(f"unsupported primitive {mesh.primitive_class:#x}")

            mesh_flags = MESH_FLAG_HAS_UV if has_uv else 0
            meshes.append((
                model_index[model.va], global_material,
                mesh_first_index, len(indices) - mesh_first_index,
                mesh.primitive_class, mesh.type_material,
                mesh.polygon_count, mesh_flags,
            ))

        model_records.append((
            first_mesh, len(meshes) - first_mesh,
            first_vertex, len(vertices) - first_vertex,
            first_index, len(indices) - first_index,
            *model.center, model.radius, model.original_texture_id, 0,
        ))

    instances = []
    for r, obj in objects:
        if not obj.model_va:
            continue
        rot = tuple(v * BAMS_TO_RAD for v in obj.rotation_bams)
        instances.append((
            model_index[obj.model_va], r.index, obj.flags, r.surface_flags,
            r.field_10, r.field_14, r.field_1c, 0,
            *obj.position, *rot, *obj.scale, *r.center, r.radius,
        ))

    if len(vertices) >= INVALID_U16:
        raise DecodeError("act visual vertex bank exceeds u16 range")
    if any(i >= len(vertices) for i in indices):
        raise DecodeError("visual index outside vertex bank")

    model_off = align16(VIS_HEADER.size)
    mesh_off = align16(model_off + len(model_records) * VIS_MODEL.size)
    instance_off = align16(mesh_off + len(meshes) * VIS_MESH.size)
    vertex_off = align16(instance_off + len(instances) * VIS_INSTANCE.size)
    index_off = align16(vertex_off + len(vertices) * VIS_VERTEX.size)
    size = index_off + len(indices) * 2
    blob = bytearray(size)
    VIS_HEADER.pack_into(
        blob, 0, VIS_MAGIC, VIS_MAJOR, VIS_MINOR,
        len(model_records), len(meshes), len(instances), len(vertices), len(indices),
        model_off, mesh_off, instance_off, vertex_off, index_off,
    )
    for i, rec in enumerate(model_records):
        VIS_MODEL.pack_into(blob, model_off + i * VIS_MODEL.size, *rec)
    for i, rec in enumerate(meshes):
        VIS_MESH.pack_into(blob, mesh_off + i * VIS_MESH.size, *rec)
    for i, rec in enumerate(instances):
        VIS_INSTANCE.pack_into(blob, instance_off + i * VIS_INSTANCE.size, *rec)
    for i, v in enumerate(vertices):
        VIS_VERTEX.pack_into(blob, vertex_off + i * VIS_VERTEX.size, *v)
    for i, index in enumerate(indices):
        struct.pack_into("<H", blob, index_off + i * 2, index)

    meta = {
        "act": act,
        "texture_archive": table.texture_archive,
        "counts": {
            "models": len(model_records),
            "meshes": len(meshes),
            "instances": len(instances),
            "vertices": len(vertices),
            "indices": len(indices),
            "triangles": len(indices) // 3,
            "materials_deduplicated": len(material_keys),
            "meshes_without_uv": meshes_without_uv,
        },
        "uv": {
            "source_storage": "signed int16 per corner",
            "divisor": UV_DIVISOR,
            "raw_min": uv_min,
            "raw_max": uv_max,
            "runtime_storage": "float32 source/255.0; values intentionally may tile outside [0,1]",
        },
        "material_semantics": {
            "record_size": 20,
            "fields": ["diffuse", "specular", "exponent", "texture_id", "flags"],
            "flags": "preserved raw; bit semantics not guessed in 0.0.9.1B",
            "texture_id": "range-checked local index into act PVM",
        },
        "source_addresses_in_runtime_payload": False,
    }
    return bytes(blob), material_keys, meta


def parse_visual_chunk(blob: bytes) -> dict:
    if len(blob) < VIS_HEADER.size:
        raise DecodeError("truncated SA3DVIS")
    (magic, major, minor, models, meshes, instances, vertices, indices,
     model_off, mesh_off, instance_off, vertex_off, index_off) = VIS_HEADER.unpack_from(blob)
    if magic != VIS_MAGIC or (major, minor) != (VIS_MAJOR, VIS_MINOR):
        raise DecodeError("bad SA3DVIS magic/version")
    regions = [
        (model_off, models * VIS_MODEL.size),
        (mesh_off, meshes * VIS_MESH.size),
        (instance_off, instances * VIS_INSTANCE.size),
        (vertex_off, vertices * VIS_VERTEX.size),
        (index_off, indices * 2),
    ]
    last = VIS_HEADER.size
    for off, size in regions:
        if off < last or off + size > len(blob):
            raise DecodeError("SA3DVIS region out of bounds/order")
        last = off + size
    if index_off + indices * 2 != len(blob):
        raise DecodeError("SA3DVIS trailing or truncated index data")
    if vertices >= INVALID_U16:
        raise DecodeError("SA3DVIS exceeds u16 vertex bank")
    for i in range(indices):
        idx = struct.unpack_from("<H", blob, index_off + i * 2)[0]
        if idx >= vertices:
            raise DecodeError("SA3DVIS index outside vertex bank")
    for i in range(meshes):
        rec = VIS_MESH.unpack_from(blob, mesh_off + i * VIS_MESH.size)
        model_i, material_i, first_index, index_count, primitive, type_material, polygon_count, flags = rec
        if model_i >= models:
            raise DecodeError("SA3DVIS mesh model index out of range")
        if first_index + index_count > indices or index_count % 3:
            raise DecodeError("SA3DVIS mesh index range invalid")
        if primitive not in (0xC000, 0x4000):
            raise DecodeError("SA3DVIS primitive invalid")
    return {
        "models": models, "meshes": meshes, "instances": instances,
        "vertices": vertices, "indices": indices, "triangles": indices // 3,
        "bytes": len(blob),
    }


def build_sa3t(pvm: PVMArchive) -> bytes:
    entries = []
    for e in pvm.entries:
        entries.append({
            "width": e.width,
            "height": e.height,
            "source_pixel_format": e.pixel_format,
            "source_data_format": e.data_format,
            "global_index": e.global_index,
            "rgba": pvm.decode_base_rgba(e.index),
        })
    pkg = sa3t.build(entries, pvm.sha256())
    sa3t.parse(pkg)
    return pkg


def build_act_package(dec: PC2004StageDecoder, act: int, pvm_bytes: bytes, set_sources=None, cam_sources=None):
    table = dec.landtable(EMERALD_COAST_LEVEL_ID, act)
    if not table.texture_archive:
        raise DecodeError("act has no resolved texture archive")
    pvm = PVMArchive(pvm_bytes, table.texture_archive + ".PVM")
    visual, materials, vis_meta = build_visual_chunk(dec, act, pvm.texture_count)
    material_blob = build_material_chunk(materials)
    texture_blob = build_sa3t(pvm)
    visual_check = parse_visual_chunk(visual)
    material_check = parse_material_chunk(material_blob)
    texture_check = sa3t.parse(texture_blob)
    metadata = {
        "format": "SA3DS Emerald Coast visual package",
        "version": "0.0.9.1B",
        "source_profile": "SADX_PC_2004_CANONICAL",
        "source_exe_sha256": dec.sha256,
        "level_id": EMERALD_COAST_LEVEL_ID,
        "level": "Emerald Coast",
        **vis_meta,
        "pvm": pvm.metadata(),
        "neutral_validation": {
            "visual": visual_check,
            "materials": material_check,
            "textures": {
                "count": len(texture_check["textures"]),
                "bytes": texture_check["file_size"],
                "source_sha256": texture_check["source_sha256"],
            },
        },
        "note": "SA3T RGBA8 is a neutral validation/import representation, not the final Old-3DS residency format.",
    }
    chunks = [
        ("GEOMETRY", visual, 0),
        ("MATERIALS", material_blob, 0),
        ("TEXTURES", texture_blob, 0),
    ]
    if set_sources is not None:
        expected_names = {
            0: {"SET0100E.BIN", "SET0100S.BIN"},
            1: {"SET0101M.BIN", "SET0101S.BIN"},
            2: {"SET0102B.BIN", "SET0102S.BIN"},
        }[act]
        if {s.name for s in set_sources} != expected_names:
            raise SetDecodeError(f"act {act}: missing/extra SET file variant")
        set_chunk = build_set_chunk(set_sources)
        set_check = parse_set_chunk(set_chunk)
        metadata["version"] = "0.0.9.1C"
        metadata["set_placements"] = {
            "files": [s.summary() for s in sorted(set_sources, key=lambda s: s.name)],
            "count": set_check["records"],
            "spawn_behavior_implemented": False,
            "variant_selection_unknown": True,
        }
        chunks.append(("SET", set_chunk, 0))
    if cam_sources is not None:
        expected_cams = {
            0: {"CAM0100S.BIN", "CAM0100E.BIN"},
            1: {"CAM0101S.BIN"},
            2: {"CAM0102S.BIN", "CAM0102B.BIN"},
        }[act]
        if {s.name for s in cam_sources} != expected_cams:
            raise CamDecodeError(f"act {act}: missing/extra CAM variant")
        cam_chunk = build_cam_chunk(cam_sources)
        cam_check = parse_cam_chunk(cam_chunk)
        metadata["version"] = "0.0.9.1D"
        metadata["camera_setup"] = {
            "files": [s.summary() for s in sorted(cam_sources, key=lambda s: s.name)],
            "count": cam_check["records"],
            "camera_behavior_implemented": False,
            "trigger_and_variant_selection_unverified": True,
            "raw_64_byte_records_preserved": True,
        }
        chunks.append(("CAMERA", cam_chunk, 0))
    meta_blob = json.dumps(metadata, sort_keys=True, separators=(",", ":")).encode() + b"\n"
    pak = sa3dspak.build([("METADATA", meta_blob, 0)] + chunks)
    sa3dspak.parse(pak)
    return pak, metadata


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("exe", type=Path, help="exact supported PC2004 sonic.exe")
    ap.add_argument("system_source", type=Path, help="owner system directory or ZIP")
    ap.add_argument("output_dir", type=Path)
    ap.add_argument("--report", type=Path)
    ap.add_argument("--with-set", action="store_true", help="embed decoded SET placement bank for each act (0.0.9.1C)")
    ap.add_argument("--with-cam", action="store_true", help="embed lossless CAM setup records for each act (0.0.9.1D)")
    args = ap.parse_args(argv)
    try:
        dec = PC2004StageDecoder(args.exe)
        args.output_dir.mkdir(parents=True, exist_ok=True)
        reports = []
        set_by_act = None
        if args.with_set:
            found_sets = list_set_sources(args.system_source, {
                "SET0100E.BIN", "SET0100S.BIN", "SET0101M.BIN", "SET0101S.BIN",
                "SET0102B.BIN", "SET0102S.BIN",
            })
            set_by_act = {a: [s for s in found_sets if s.act == a] for a in (0, 1, 2)}
        cam_by_act = None
        if args.with_cam:
            found_cams = list_cam_sources(args.system_source, {
                "CAM0100S.BIN", "CAM0100E.BIN", "CAM0101S.BIN",
                "CAM0102S.BIN", "CAM0102B.BIN",
            })
            cam_by_act = {a: [s for s in found_cams if s.name[5:7] == f"{a:02d}"] for a in (0,1,2)}
        for act in (0, 1, 2):
            table = dec.landtable(EMERALD_COAST_LEVEL_ID, act)
            if not table.texture_archive:
                raise DecodeError(f"act {act}: texture archive unresolved")
            pvm_bytes = read_source(args.system_source, table.texture_archive + ".PVM")
            pak, report = build_act_package(dec, act, pvm_bytes, set_by_act[act] if set_by_act is not None else None,
                                            cam_by_act[act] if cam_by_act is not None else None)
            out = args.output_dir / f"emerald_coast_act{act}.sa3dspak"
            out.write_bytes(pak)
            report = {**report, "output": str(out), "package_bytes": len(pak), "package_sha256": hashlib.sha256(pak).hexdigest()}
            reports.append(report)
            print(json.dumps({
                "act": act, "output": str(out), "bytes": len(pak),
                "sha256": report["package_sha256"],
                **report["counts"],
                "textures": report["pvm"]["texture_count"],
            }, sort_keys=True))
        if args.report:
            args.report.write_text(json.dumps({"version": "0.0.9.1D" if args.with_cam else "0.0.9.1C" if args.with_set else "0.0.9.1B", "acts": reports}, indent=2, sort_keys=True) + "\n")
        return 0
    except (OSError, ValueError, DecodeError, PVMArchiveError, BadZipFile, struct.error, SetDecodeError, CamDecodeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
