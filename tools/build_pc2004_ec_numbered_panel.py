#!/usr/bin/env python3
"""Build the owner-only O JPanel base geometry package."""
import argparse
import hashlib
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
from tools.importers.pc2004_stage import CollisionRecord, PC2004StageDecoder
from tools.verify_pc2004_ec_route import verified_reader


class PanelBase(Adapter):
    def __init__(self, decoder, root, child):
        self.dec = decoder
        self.objects = {part.va: part for part in (root, child)}
        self.rows = []
        for index, part in enumerate((root, child)):
            model = decoder.model(part.model_va)
            self.rows.append(CollisionRecord(index, model.center, model.radius, 0, 0,
                                             part.va, 0, 0x80000000))

    def collisions(self, table):
        return self.rows

    def object(self, address):
        return self.objects[address]


def build(exe, system):
    read = verified_reader(exe)
    display = hashlib.sha256(read(0x4B8660, 0x165)).hexdigest()
    if display != '1b214ad7827f70ce2b47914c7a331e78795c0a87f7987bd4573e370caa8800b7':
        raise ValueError('O JPanel display callback mismatch')
    decoder = PC2004StageDecoder(exe)
    regular, = struct.unpack('<I', read(0x90EC74, 4))
    name, texlist = struct.unpack('<2I', read(regular, 8))
    if read(name, 12) != b'OBJ_REGULAR\0' or decoder.resolve_texture_archive(texlist) != 'OBJ_REGULAR':
        raise ValueError('O JPanel base texture archive mismatch')
    root = decoder.object(0x8C5174)
    child = decoder.object(root.child_va)
    if any(part.sibling_va or part.position != (0., 0., 0.) or
           part.rotation_bams != (0, 0, 0) or part.scale != (1., 1., 1.)
           for part in (root, child)) or child.child_va:
        raise ValueError('Unsupported O JPanel base hierarchy')
    archive = PVMArchive(read_source(system, 'OBJ_REGULAR.PVM'), 'OBJ_REGULAR.PVM')
    geometry, materials, _ = build_visual_chunk(PanelBase(decoder, root, child), 0, archive.texture_count)
    materials, textures, _ = compact_textures(materials, archive)
    return sa3dspak.build([
        ('METADATA', b'{"scope":"original O JPanel base hierarchy"}', 0),
        ('GEOMETRY', geometry, 0),
        ('MATERIALS', build_material_chunk(materials), 0),
        ('TEXTURES', textures, 0),
    ])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sonic_exe', type=Path)
    parser.add_argument('system', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    if args.output.resolve().is_relative_to(ROOT):
        raise SystemExit('owner output must remain outside the source tree')
    data = build(args.sonic_exe, args.system)
    sa3dspak.parse(data)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f'O JPanel base: {len(data)} bytes, SHA-256 {hashlib.sha256(data).hexdigest()}')
