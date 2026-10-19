#!/usr/bin/env python3
"""Build pointer-free Emerald Coast collision data from exact owner sonic.exe.

Generated .sa3dspak files contain Sega-derived stage geometry and therefore are
owner-side build products.  Do NOT put them in distributable SA3DS source/code
archives.
"""
from __future__ import annotations

from pathlib import Path
import argparse
import json
import math
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools.importers.pc2004_chrmodels import DecodeError
from tools.importers.pc2004_stage import PC2004StageDecoder, EMERALD_COAST_ACTS
from tools import sa3dspak

MAGIC = b"SA3DECOL"
VERSION_MAJOR = 1
VERSION_MINOR = 0
HEADER = struct.Struct("<8sHHIIIIIIIIII")
ACT = struct.Struct("<IIII f III")           # 32 bytes
MODEL = struct.Struct("<IIII ffff")          # 32 bytes
INSTANCE = struct.Struct("<IIII ffff fffffffff III")  # 80 bytes
BAMS_TO_RAD = (2.0 * math.pi) / 65536.0


def align16(value: int) -> int:
    return (value + 15) & ~15


def build_collision_chunk(dec: PC2004StageDecoder):
    tables = dec.emerald_coast_landtables()
    act_rows = []
    instance_source = []
    model_vas: set[int] = set()

    for table in tables:
        first_instance = len(instance_source)
        for row in dec.collisions(table):
            obj = dec.object(row.object_va)
            if obj.child_va or obj.sibling_va:
                raise DecodeError("collision hierarchy unexpectedly non-flat")
            if not obj.model_va:
                raise DecodeError("collision object has null model")
            model_vas.add(obj.model_va)
            instance_source.append((table.act, row, obj))
        act_rows.append((
            table.act, first_instance, table.collision_count,
            table.load_flags, table.radius, 0, 0, 0,
        ))

    model_vas = sorted(model_vas)
    model_index = {va: i for i, va in enumerate(model_vas)}
    vertices: list[tuple[float, float, float]] = []
    indices: list[int] = []
    model_rows = []

    for va in model_vas:
        model = dec.model(va)
        first_vertex = len(vertices)
        first_index = len(indices)
        points = dec.model_points(model)
        if len(vertices) + len(points) >= 65536:
            raise DecodeError("Emerald Coast collision vertex bank exceeds u16 range")
        vertices.extend(points)
        for tri in dec.model_triangles(model):
            indices.extend(first_vertex + i for i in tri)
        model_rows.append((
            first_vertex, len(points), first_index, len(indices) - first_index,
            *model.center, model.radius,
        ))

    instance_rows = []
    for _act, row, obj in instance_source:
        rot = tuple(v * BAMS_TO_RAD for v in obj.rotation_bams)
        instance_rows.append((
            model_index[obj.model_va], row.surface_flags, row.field_10, row.field_14,
            *row.center, row.radius,
            *obj.position, *rot, *obj.scale,
            row.field_1c, obj.flags, 0,
        ))

    header_size = HEADER.size
    act_off = align16(header_size)
    model_off = align16(act_off + len(act_rows) * ACT.size)
    instance_off = align16(model_off + len(model_rows) * MODEL.size)
    vertex_off = align16(instance_off + len(instance_rows) * INSTANCE.size)
    index_off = align16(vertex_off + len(vertices) * 12)
    file_size = index_off + len(indices) * 2

    blob = bytearray(file_size)
    HEADER.pack_into(
        blob, 0, MAGIC, VERSION_MAJOR, VERSION_MINOR,
        len(act_rows), len(model_rows), len(instance_rows), len(vertices), len(indices),
        act_off, model_off, instance_off, vertex_off, index_off,
    )
    for i, row in enumerate(act_rows):
        ACT.pack_into(blob, act_off + i * ACT.size, *row)
    for i, row in enumerate(model_rows):
        MODEL.pack_into(blob, model_off + i * MODEL.size, *row)
    for i, row in enumerate(instance_rows):
        INSTANCE.pack_into(blob, instance_off + i * INSTANCE.size, *row)
    for i, v in enumerate(vertices):
        struct.pack_into("<fff", blob, vertex_off + i * 12, *v)
    for i, index in enumerate(indices):
        struct.pack_into("<H", blob, index_off + i * 2, index)

    meta = {
        "format": "SA3DECOL",
        "version": [VERSION_MAJOR, VERSION_MINOR],
        "source_profile": "SADX_PC_2004_CANONICAL",
        "level": "Emerald Coast",
        "acts": list(EMERALD_COAST_ACTS),
        "counts": {
            "models": len(model_rows),
            "instances": len(instance_rows),
            "vertices": len(vertices),
            "indices": len(indices),
            "triangles": len(indices) // 3,
        },
        "source_addresses_in_runtime_payload": False,
    }
    return bytes(blob), meta


def parse_collision_chunk(blob: bytes):
    if len(blob) < HEADER.size:
        raise DecodeError("truncated SA3DECOL header")
    (magic, major, minor, acts, models, instances, vertices, indices,
     act_off, model_off, instance_off, vertex_off, index_off) = HEADER.unpack_from(blob)
    if magic != MAGIC or major != VERSION_MAJOR:
        raise DecodeError("bad SA3DECOL magic/version")
    if acts != 3:
        raise DecodeError("unexpected Emerald Coast act count")
    regions = [
        (act_off, acts * ACT.size),
        (model_off, models * MODEL.size),
        (instance_off, instances * INSTANCE.size),
        (vertex_off, vertices * 12),
        (index_off, indices * 2),
    ]
    last = HEADER.size
    for off, size in regions:
        if off < last or off + size > len(blob):
            raise DecodeError("SA3DECOL region out of bounds/order")
        last = off + size
    if indices % 3:
        raise DecodeError("collision index count is not triangles")
    if vertices >= 65536:
        raise DecodeError("u16 collision index domain exceeded")
    for i in range(indices):
        idx = struct.unpack_from("<H", blob, index_off + i * 2)[0]
        if idx >= vertices:
            raise DecodeError("collision index outside vertex bank")
    return {
        "version": [major, minor],
        "acts": acts,
        "models": models,
        "instances": instances,
        "vertices": vertices,
        "indices": indices,
        "triangles": indices // 3,
    }


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("exe", type=Path, help="exact supported 2004 sonic.exe")
    ap.add_argument("output", type=Path, help="owner-side .sa3dspak output")
    ap.add_argument("--report", type=Path)
    args = ap.parse_args(argv)
    try:
        dec = PC2004StageDecoder(args.exe)
        collision, meta = build_collision_chunk(dec)
        # Self-validate the source-neutral chunk before packaging.
        neutral = parse_collision_chunk(collision)
        inventory = dec.emerald_coast_inventory()
        metadata = {
            **meta,
            "neutral": neutral,
            "inventory": inventory,
        }
        pak_meta = json.dumps(metadata, sort_keys=True, separators=(",", ":")).encode() + b"\n"
        pak = sa3dspak.build([
            ("METADATA", pak_meta, 0),
            ("COLLISION", collision, 0),
        ])
        # Validate package too.
        sa3dspak.parse(pak)
        args.output.write_bytes(pak)
        if args.report:
            args.report.write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n")
        print(json.dumps({
            "output": str(args.output),
            "pak_bytes": len(pak),
            "collision_bytes": len(collision),
            **neutral,
        }, sort_keys=True))
        return 0
    except (OSError, DecodeError, ValueError, struct.error) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
