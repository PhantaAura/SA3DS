#!/usr/bin/env python3
"""Convert exact-owner PC2004 direct-ring resources into external native packages.

No owner data is embedded in this source. The identity visual instance is a
resource container; actor placement/rotation belongs to the native runtime.
Only direct SET type-zero rings are included. Line/group expansion, behavior,
collision timing and full-stage playability are not established by conversion.
"""
from __future__ import annotations

import argparse
import configparser
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sa3dspak, sa3t
from tools.build_pc2004_emerald_coast_visual import (
    build_visual_chunk, build_material_chunk, parse_visual_chunk,
    parse_material_chunk, read_source,
)
from tools.importers.pc2004_stage import PC2004StageDecoder, ObjectRecord, CollisionRecord
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.importers.pc2004_set import read_set_source, SOURCE_ROW
from tools.verify_pc2004_ec_route import verified_reader, PROFILE_SHA256

RING_MODEL = 0x8B4808
RING_REGISTRY = 0x1036420
HEADER = struct.Struct('<8sHHIIIII')
ROW = struct.Struct('<II32s')
SPEC = struct.Struct('<ffffffII')
assert HEADER.size == 32 and ROW.size == 40 and SPEC.size == 32

# Instruction witnesses, not copied executable/decompiled implementations.
SIGNATURES = {
    'ring texture selector': (0x44F70F, 'e87c18fdff'),
    'ring direct Basic model draw': (0x44F77B, '6808488b00e8eb81fbff'),
    'ring queued Basic model draw': (0x44F749, '6808488b00'),
    'regular texture selector global': (0x420F90, 'a1b090b20385c0'),
    'ring collision initializer': (0x4503B0, '68106b910053e835c7fcff'),
    'ring task-yaw advance': (0x450586, '056c010000894618'),
    'source class low12 mask': (0x46BB0A, '81e1ff0f0000'),
    'source unload range addition': (0x46BCB4, 'd805e0057e00d95e0c'),
    'normal SET mode selects character suffix': (0x422961, 'a0b48db10384c07407a144f49000eb0c'),
    'normal SET suffix index': (0x422971, 'e86a1bffff8b048524f49000'),
    'current character getter': (0x4144E0, '0fb705c02db203c3'),
    'normal Sonic suffix pointer': (0x90F424, 'bcde7d00'),
    'normal Sonic suffix string': (0x7DDEBC, '5300'),
}


class _IdentityModelAdapter:
    """Feed one source Basic model through the established visual converter."""
    def __init__(self, decoder, texture_archive):
        self.decoder = decoder
        self.texture_archive = texture_archive
        model = decoder.model(RING_MODEL)
        self.instance = CollisionRecord(0, model.center, model.radius, 0, 0, 0, 0, 0x80000000)
        self.identity = ObjectRecord(0, 0, RING_MODEL, (0., 0., 0.), (0, 0, 0), (1., 1., 1.), 0, 0)

    def __getattr__(self, name):
        return getattr(self.decoder, name)

    def landtable(self, level, act):
        return SimpleNamespace(texture_archive=self.texture_archive)

    def collisions(self, table):
        return [self.instance]

    def object(self, address):
        if address != 0:
            raise ValueError('identity resource container has no source object address')
        return self.identity


def owner_clip_level(exe_path):
    """Read the owner's graphics clip level, retaining the source fallback1."""
    ini = Path(exe_path).parent / 'sonicDX.ini'
    if not ini.exists():
        return 1, {'clip_level': 1, 'selection': 'source fallback; sonicDX.ini absent'}
    raw = ini.read_bytes()
    try:
        text = raw.decode('utf-8-sig')
    except UnicodeDecodeError:
        text = raw.decode('cp1252')
    config = configparser.ConfigParser(interpolation=None, strict=False)
    config.read_string(text)
    section = next((s for s in config.sections() if s.lower() == 'sonicdx'), None)
    value = config.get(section, 'cliplevel', fallback='1') if section else '1'
    try:
        clip = int(value.strip(), 10)
    except ValueError as exc:
        raise ValueError('unsupported non-integer owner graphics clip level') from exc
    if not 0 <= clip <= 2:
        raise ValueError('owner graphics clip level must be0..2')
    return clip, {'clip_level': clip, 'ini_sha256': hashlib.sha256(raw).hexdigest(),
                  'selection': 'owner sonicDX.ini[sonicDX] cliplevel; missing key uses source fallback1'}


def verified_specs(exe_path):
    read = verified_reader(exe_path)
    for label, (address, expected_hex) in SIGNATURES.items():
        expected = bytes.fromhex(expected_hex)
        if read(address, len(expected)) != expected:
            raise ValueError('exact-owner ring witness mismatch: ' + label)
    flags, priority, range_flags, distance_squared, default_data, task, _name = struct.unpack(
        '<BBHfIII', read(RING_REGISTRY, 20))
    if (flags, priority, range_flags, default_data, task) != (2, 3, 1, 0, 0x450370):
        raise ValueError('unexpected source ring registry semantics')
    if distance_squared <= 0:
        raise ValueError('invalid source ring activation range')
    center = struct.unpack('<fff', read(0x916B10 + 8, 12))
    radius, = struct.unpack('<f', read(0x916B10 + 20, 4))
    unload_margin, = struct.unpack('<f', read(0x7E05E0, 4))
    turn, = struct.unpack('<I', read(0x450587, 4))
    if not all(math.isfinite(v) for v in (*center, radius, unload_margin)) or radius <= 0:
        raise ValueError('invalid source ring collision/range fields')
    # Read the float used by46BA40 and the immediate added byRing_Main.
    clip_level, _selection = owner_clip_level(exe_path)
    return (*center, radius, distance_squared, distance_squared + unload_margin, turn, clip_level)


def compact_textures(materials, pvm):
    """Preserve source material values and map only referenced textures locally."""
    used = sorted({row[3] for row in materials})
    if not used or any(i < 0 or i >= pvm.texture_count for i in used):
        raise ValueError('ring material texture outside source archive')
    mapping = {source: local for local, source in enumerate(used)}
    remapped = [(*row[:3], mapping[row[3]], row[4]) for row in materials]
    entries = []
    for source in used:
        e = pvm.entries[source]
        entries.append({
            'width': e.width, 'height': e.height,
            'source_pixel_format': e.pixel_format, 'source_data_format': e.data_format,
            'global_index': e.global_index, 'rgba': pvm.decode_base_rgba(source),
        })
    textures = sa3t.build(entries, pvm.sha256())
    sa3t.parse(textures)
    return remapped, textures, used


def build_ring_placements(sources, spec):
    if len(sources) != 2 or {(s.level_id, s.act) for s in sources} != {(1, 0), (1, 1)}:
        raise ValueError('direct-ring package requires one Emerald Coast source per Sonic act')
    rows = []
    for source in sorted(sources, key=lambda s: s.act):
        for index, placement in enumerate(source.records):
            if placement.type_word & 0xFFF == 0:
                rows.append((source.act, index, placement.raw))
    if len(rows) > 2046:
        raise ValueError('direct-ring package exceeds two original1023-row act limits')
    spec_offset = HEADER.size + len(rows) * ROW.size
    result = bytearray(spec_offset + SPEC.size)
    HEADER.pack_into(result, 0, b'SA3DRING', 1, 0, 2, len(rows), HEADER.size,
                     spec_offset, len(result))
    for index, row in enumerate(rows):
        ROW.pack_into(result, HEADER.size + index * ROW.size, *row)
    SPEC.pack_into(result, spec_offset, *spec)
    return bytes(result), rows


def build(exe_path, system_source):
    spec = verified_specs(exe_path)
    _clip_level, clip_selection = owner_clip_level(exe_path)
    decoder = PC2004StageDecoder(exe_path)
    archive = decoder.resolve_texture_archive(0x8B1CD8)
    if archive != 'OBJ_REGULAR':
        raise ValueError('regular object texture archive did not resolve to expected source archive')
    pvm = PVMArchive(read_source(system_source, archive + '.PVM'), archive + '.PVM')
    visual, materials, geometry_meta = build_visual_chunk(
        _IdentityModelAdapter(decoder, archive), 0, pvm.texture_count)
    materials, textures, used = compact_textures(materials, pvm)
    material_blob = build_material_chunk(materials)
    parse_visual_chunk(visual); parse_material_chunk(material_blob)
    metadata = {
        'format': 'SA3DS direct-ring identity visual resource',
        'source_profile': 'SADX_PC_2004_CANONICAL', 'source_exe_sha256': PROFILE_SHA256,
        'source_pvm_sha256': pvm.sha256(), 'source_texture_indices': used,
        'texture_archive': archive,
        'counts': geometry_meta['counts'],
        'authority': 'EXACT OWNER SOURCE RESOURCE; NOT GAMEPLAY OR RETAIL TRACE VERIFIED',
        'identity_instance_is_resource_container': True,
        'runtime_actor_placement_and_rotation_are_separate': True,
    }
    visual_package = sa3dspak.build([
        ('METADATA', json.dumps(metadata, sort_keys=True).encode(), 0),
        ('GEOMETRY', visual, 0), ('MATERIALS', material_blob, 0), ('TEXTURES', textures, 0),
    ])
    # verified_specs pins the normal-mode character0 suffix selection chain.
    # Keep each original source identity and all placement bytes in metadata.
    sources = [read_set_source(f'SET010{act}S.BIN', read_source(system_source, f'SET010{act}S.BIN'))
               for act in (0, 1)]
    placements, rows = build_ring_placements(sources, spec)
    placement_meta = {
        'format': 'SA3DS direct-ring placements', 'source_exe_sha256': PROFILE_SHA256,
        'sources': [s.summary() for s in sources],
        'counts_by_act': {str(act): sum(row[0] == act for row in rows) for act in (0, 1)},
        'scope': 'DIRECT TYPE0 ONLY; line/group rings remain separate; source clip-level highbits preserved',
        'parameters_preserved_not_model_scale': True,
        'normal_sonic_suffix': 'S',
        'clip_high_bits_preserved': True,
        'graphics_clip_selection': clip_selection,
        'authority': 'EXACT OWNER SOURCE DATA; NOT GAMEPLAY OR RETAIL TRACE VERIFIED',
    }
    placement_package = sa3dspak.build([
        ('METADATA', json.dumps(placement_meta, sort_keys=True).encode(), 0),
        ('RINGS', placements, 0),
    ])
    for package in (visual_package, placement_package):
        sa3dspak.parse(package)
    return visual_package, placement_package, metadata, placement_meta


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sonic_exe', type=Path)
    parser.add_argument('system_source', type=Path)
    parser.add_argument('output_dir', type=Path)
    args = parser.parse_args()
    if args.output_dir.resolve().is_relative_to(ROOT):
        raise SystemExit('owner-generated ring data must remain outside the code tree')
    visual, placements, metadata, placement_meta = build(args.sonic_exe, args.system_source)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, data in [('ring_visual.sa3dspak', visual), ('ring_placements.sa3dspak', placements)]:
        output = args.output_dir / name
        output.write_bytes(data)
        print(f'{output}: bytes={len(data)} sha256={hashlib.sha256(data).hexdigest()}')
    print(json.dumps({'visual_counts': metadata['counts'],
                      'direct_rings_by_act': placement_meta['counts_by_act']}, sort_keys=True))


if __name__ == '__main__':
    main()
