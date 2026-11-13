#!/usr/bin/env python3
"""Convert the original O JUMP display models into an external owner package."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools import sa3dspak
from tools.build_pc2004_emerald_coast_visual import build_material_chunk, build_visual_chunk, read_source
from tools.build_pc2004_ring_assets import compact_textures
from tools.build_pc2004_spring_visual import Adapter
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools.verify_pc2004_ec_route import verified_reader

ROOT_OBJECT = 0x10A298C
FIXED_PART = 0x10A15AC
ROTATING_PART = 0x10A1974
WITNESSES = {
    (0x4FFEC0, 179): '6a78b26ec4cedd6bd9dba728c35e96af6e2dbd435ec865ec46cfd3f3add50ad6',
    (0x500020, 448): '3640b41d6a3e580d9af56cd42290f10e34efb7cf6ac4f44cc06afd92d063a419',
    (0x500210, 56): '96ee2d93fec80fac11fed0aeebb3e20afeb27f4993aa5abb6105b7dd4cafd841',
}


def build(exe, system):
    read = verified_reader(exe)
    decoder = PC2004StageDecoder(exe)
    for (address, length), digest in WITNESSES.items():
        if hashlib.sha256(read(address, length)).hexdigest() != digest:
            raise ValueError(f'O JUMP display witness mismatch at {address:#x}')
    texture_table, = struct.unpack('<I', read(0x90EB6C, 4))
    name, texlist = struct.unpack('<2I', read(texture_table, 8))
    if read(name, 10) != b'OBJ_BEACH\0' or decoder.resolve_texture_archive(texlist) != 'OBJ_BEACH':
        raise ValueError('Emerald Coast object texture table mismatch')
    archive = PVMArchive(read_source(system, 'OBJ_BEACH.PVM'), 'OBJ_BEACH.PVM')

    parts = [decoder.object(address) for address in (ROOT_OBJECT, FIXED_PART, ROTATING_PART)]
    if (parts[0].child_va != ROTATING_PART or parts[2].sibling_va != FIXED_PART or
            any(part.rotation_bams != (0, 0, 0) or not part.model_va for part in parts)):
        raise ValueError('Unexpected O JUMP component hierarchy or rotation')

    # The callback draws the base model at task origin and explicitly applies
    # child translations, ignoring the object evaluator's UNIT_* flags.
    config = struct.pack('<8sI6f', b'SA3JVIS\0', 1, *parts[1].position, *parts[2].position)
    metadata = json.dumps({'source_exe_sha256': decoder.sha256,
                           'source_pvm_sha256': archive.sha256(),
                           'scope': 'O JUMP display components and base DynamicCOL mesh'}, sort_keys=True).encode()
    base = decoder.model(parts[0].model_va)
    points = decoder.model_points(base)
    triangles = decoder.model_triangles(base)
    if base.radius <= 20.0 or sum(value * value for value in base.center) < 225.0:
        raise ValueError('O JUMP DynamicCOL profile mismatch')
    if (len(points), len(triangles)) != (94, 120):
        raise ValueError('Unexpected O JUMP DynamicCOL mesh size')
    collision = struct.pack('<8s4I', b'SA3JCOL\0', 1, len(points), len(triangles), 1)
    collision += b''.join(struct.pack('<3f', *point) for point in points)
    collision += b''.join(struct.pack('<3H', *triangle) for triangle in triangles)
    chunks = [('METADATA', metadata, 0), ('JUMP_VISUAL', config, 0), ('JUMP_COL', collision, 0)]
    for index, part in enumerate(parts):
        geometry, materials, _ = build_visual_chunk(Adapter(decoder, part.model_va), 0, archive.texture_count)
        materials, textures, _ = compact_textures(materials, archive)
        visual = sa3dspak.build([('METADATA', b'{"scope":"owner O JUMP model part"}', 0),
                                ('GEOMETRY', geometry, 0),
                                ('MATERIALS', build_material_chunk(materials), 0),
                                ('TEXTURES', textures, 0)])
        chunks.append((f'VISUAL{index}', visual, 0))
    package = sa3dspak.build(chunks)
    sa3dspak.parse(package)
    return package


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sonic_exe', type=Path)
    parser.add_argument('system', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    if args.output.resolve().is_relative_to(ROOT):
        raise SystemExit('owner output must remain outside the source tree')
    package = build(args.sonic_exe, args.system)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(package)
    print(f'O JUMP display: {len(package)} bytes, SHA-256 {hashlib.sha256(package).hexdigest()}')
