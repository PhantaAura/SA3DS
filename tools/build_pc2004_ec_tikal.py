#!/usr/bin/env python3
"""Build the eight original Tikal orb frames from the owner's HINT archive."""
import argparse
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sa3dspak, sa3t
from tools.build_pc2004_emerald_coast_visual import read_source
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.verify_pc2004_ec_route import verified_reader


def build(exe, system):
    read = verified_reader(exe)
    name, texlist = struct.unpack('<2I', read(0x90EC30, 8))
    sprite_texlist, = struct.unpack('<I', read(0x94A268, 4))
    if read(name, 5) != b'HINT\0' or texlist != sprite_texlist:
        raise ValueError('Tikal sprite texture binding mismatch')
    archive = PVMArchive(read_source(system, 'HINT.PVM'), 'HINT.PVM')
    frames = []
    for index, entry in enumerate(archive.entries[:8]):
        if (entry.name.lower(), entry.width, entry.height) != (f'tical{index+1:02d}', 64, 64):
            raise ValueError('Unexpected Tikal orb frame')
        frames.append(dict(width=64, height=64, source_pixel_format=entry.pixel_format,
                           source_data_format=entry.data_format, global_index=entry.global_index,
                           rgba=archive.decode_base_rgba(index)))
    if len(frames) != 8:
        raise ValueError('Incomplete Tikal orb sequence')
    textures = sa3t.build(frames, archive.sha256())
    sa3t.parse(textures)
    # English stage hint table, level 1, message 8 (O TIKAL SET parameter Z).
    table, = struct.unpack('<I', read(0x9BED60 + 4 * (5 + 1), 4))
    script, pages = struct.unpack('<2I', read(table + 8 * 8, 8))
    command, voice, end = struct.unpack('<3h', read(script, 6))
    if (command, end) != (-4, -1) or not 0 <= voice <= 9999:
        raise ValueError('Unexpected Act 2 Tikal voice script')
    hint = struct.pack('<8sHH', b'SA3HINT\0', 1, voice)
    for address, ticks in struct.iter_unpack('<2I', read(pages, 16)):
        text = read(address, 96).split(b'\0', 1)[0]
        if not text.startswith(b'\x07') or not 0 < ticks <= 600:
            raise ValueError('Unexpected Act 2 Tikal message page')
        lines = text[1:].split(b'\n')
        if len(lines) != 2 or any(not 0 < len(line) < 40 or
                                  any(not 32 <= c <= 126 for c in line) for line in lines):
            raise ValueError('Tikal hint exceeds native text bounds')
        hint += struct.pack('<H40s40s', ticks, *lines)
    if read(pages + 16, 8) != bytes(8):
        raise ValueError('Expected exactly two Tikal message pages')
    return sa3dspak.build([('METADATA', b'{"scope":"original Tikal orb frames"}', 0),
                           ('TEXTURES', textures, 0), ('TIKAL_HINT', hint, 0)])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('system', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    if args.output.resolve().is_relative_to(ROOT):
        raise SystemExit('owner output must remain outside the source tree')
    package = build(args.exe, args.system)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(package)
    print(f'Tikal orb: eight owner sprite frames, {len(package)} bytes')
