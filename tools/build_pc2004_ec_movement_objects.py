#!/usr/bin/env python3
"""Prepare owner-only Emerald Coast movement and hint placements, not a spawner.

Preserves original SET slot numbers and raw object-specific parameters. Source
code/pointers are used only to verify the exact profile; none are serialized.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader, PROFILE_SHA256
from tools.importers.pc2004_set import read_set_source, parse_placement
from tools.build_pc2004_emerald_coast_visual import read_source

HEADER = struct.Struct('<8sHHIIIII')
ROW = struct.Struct('<II32s')
SPEC = struct.Struct('<HBBffI')
# class -> source list, flags, load-radius-squared, unload-radius-squared.
SPECS = {1: (2, 2, 360000., 360100.), 2: (2, 2, 360000., 360100.),
         3: (3, 3, 360000., 360100.), 18: (2, 2, 4000000., 4000100.),
         26: (2, 2, 360000., 360100.), 40: (3, 6, 360000., 360100.)}
TASKS = {1: 0x7A4C30, 2: 0x7A4E50, 3: 0x7A4450, 18: 0x4B8DC0, 26: 0x7A1AA0, 40: 0x5001E0}
FUNCTIONS = {
    (0x7A46C0, 0x2B3): '6347f6f636443998dcd65d122dd39bb00c4960140cf5f714de716b91c91c2d81',
    (0x441370, 0x70): '2faa7a0b64bd869e728c40648b2e4dbd1f4bc51a3a055331a42bbdb97daab46e',
    (0x4413E0, 0xB4): 'f718facbc5b09a906409e0ee839a6ef57a2f950902454997c840eb8e5cf80aa7',
}

def build_chunk(sources, *, legacy=False):
    specs = {kind: SPECS[kind] for kind in (1, 2, 3)} if legacy else SPECS
    if len(sources) != 2 or {(s.level_id, s.act, s.variant) for s in sources} != {(1, 0, 'S'), (1, 1, 'S')}:
        raise ValueError('requires both normal Sonic Emerald Coast SET banks')
    rows = []
    counts = [{k: 0 for k in specs} for _ in range(2)]
    for source in sorted(sources, key=lambda s: s.act):
        if len(source.records) > 1023:
            raise ValueError('source exceeds original SET bank capacity')
        if hashlib.sha256(source.source_bytes()).hexdigest() != source.sha256:
            raise ValueError('source changed after decoding')
        for index, p in enumerate(source.records):
            kind = p.type_word & 0xFFF
            if kind not in specs:
                continue
            parse_placement(p.raw)  # finite values, exact 32-byte row
            rows.append((source.act, index, p.raw))
            counts[source.act][kind] += 1
    off = HEADER.size
    spec = off + len(rows) * ROW.size
    total = spec + len(specs) * SPEC.size
    data = bytearray(total)
    HEADER.pack_into(data, 0, b'SA3DMOVE', 1, int(not legacy), len(rows), off, spec, len(specs), total)
    for i, row in enumerate(rows):
        ROW.pack_into(data, off + i * ROW.size, *row)
    for i, (kind, values) in enumerate(specs.items()):
        SPEC.pack_into(data, spec + i * SPEC.size, kind, *values, 0)
    return bytes(data), counts

def build(exe, system):
    read = verified_reader(exe)
    for (address, size), digest in FUNCTIONS.items():
        if hashlib.sha256(read(address, size)).hexdigest() != digest:
            raise ValueError('spring command function witness mismatch')
    for kind, (objlist, flags, load, _) in SPECS.items():
        registry = struct.unpack('<BBHfIII', read(0x1036420 + kind * 20, 20))
        if registry[:6] != (flags, objlist, 1, load, 0, TASKS[kind]):
            raise ValueError('movement object registry witness mismatch')
    if struct.unpack('<f', read(0x7DCCFC, 4))[0] != 5.:
        raise ValueError('spring launch bias mismatch')
    sources = [read_set_source(f'SET010{act}S.BIN', read_source(system, f'SET010{act}S.BIN')) for act in (0, 1)]
    chunk, counts = build_chunk(sources)
    metadata = {'source_exe_sha256': PROFILE_SHA256,
                'source_set_sha256': [s.sha256 for s in sources],
                'counts_by_act': counts,
                'scope': 'source definitions; O JUMP, O JPanel and O TIKAL gameplay integration pending'}
    data = sa3dspak.build([('METADATA', json.dumps(metadata, sort_keys=True).encode(), 0),
                          ('MOVE_OBJECTS', chunk, 0)])
    sa3dspak.parse(data)
    return data, counts

if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('sonic_exe', type=Path)
    ap.add_argument('system', type=Path)
    ap.add_argument('output', type=Path)
    args = ap.parse_args()
    if args.output.resolve().is_relative_to(Path(__file__).resolve().parents[1]):
        raise SystemExit('owner output must remain outside the source tree')
    data, counts = build(args.sonic_exe, args.system)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f'Owner movement object definitions: {counts}; {len(data)} bytes; SHA256={hashlib.sha256(data).hexdigest()}')
