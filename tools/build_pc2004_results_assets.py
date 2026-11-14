#!/usr/bin/env python3
"""Stage the owner's ordinary Sonic results textures and label positions."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sa3dspak, sa3t
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.verify_pc2004_ec_route import verified_reader


def build(exe, system, english):
    read = verified_reader(exe)
    suffix = '_E' if english else ''
    names = ('SCORE_ACT' + suffix, 'SCORE_RESULT' + suffix, 'SCORE_BACK', 'CON_REGULAR' + suffix)
    counts = (7, 6, 4, 11)
    entries = []
    hashes = {}
    for name, count in zip(names, counts):
        digits = name.startswith("CON_REGULAR")
        archive = PVMArchive(system / (name + '.PVM'), pixel_format_from_pvrt=True,
                             texture_prefix=count if digits else None, allow_twiddled=digits)
        if len(archive.entries) != count:
            raise ValueError(f'unexpected texture count for {name}')
        hashes[name] = archive.sha256()
        for e in archive.entries:
            if digits:
                expected = f"snum_{e.index}" if e.index < 10 else "snum_co"
                if (e.name, e.width, e.height, e.data_format) != (expected, 32, 32, 1):
                    raise ValueError(f"unexpected score digit {e.name}")
            if any(n < 8 or n > 256 or n & (n - 1) for n in (e.width, e.height)):
                raise ValueError(f'unsupported texture size for {name}/{e.name}')
            entries.append(dict(width=e.width, height=e.height,
                                source_pixel_format=e.pixel_format,
                                source_data_format=e.data_format,
                                global_index=e.global_index,
                                rgba=archive.decode_base_rgba(e.index)))

    # 427430 submits six label sprites with these consecutive descriptors.
    # The seventh is Big's different score label.
    labels = bytearray()
    for index in range(7):
        x, y, z, sx, sy, angle, _flags, texture_list = struct.unpack(
            '<6fII', read(0x9135c0 + 32 * index, 32))
        if (z, sx, sy, angle, texture_list) != (0, 1, 1, 0, 0x913530):
            raise ValueError('unexpected score label descriptor')
        sprite = struct.unpack('<10h', read(0x913530 + 20 * index, 20))
        if sprite[2:4] != (0, 16) or sprite[8] != index:
            raise ValueError('unexpected score label anchor')
        labels.extend(struct.pack('<4f', x, y, sx, sy))

    source_hash = hashlib.sha256(b''.join(bytes.fromhex(hashes[name]) for name in names)).hexdigest()
    textures = sa3t.build(entries, source_hash)
    sa3t.parse(textures)
    metadata = json.dumps({'archives': hashes, 'english': english,
                           'texture_starts': dict(zip(names, (0, 7, 13, 17)))},
                          sort_keys=True).encode()
    package = sa3dspak.build([('METADATA', metadata, 0), ('TEXTURES', textures, 0),
                             ('SCORE_LABELS', bytes(labels), 0)])
    sa3dspak.parse(package)
    return package


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('system', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--japanese', action='store_true')
    args = parser.parse_args()
    if args.output.resolve().is_relative_to(ROOT):
        parser.error('owner-generated assets must stay outside source tree')
    package = build(args.exe, args.system, not args.japanese)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(package)
    print(f'Wrote {len(package)} bytes of owner results assets')
