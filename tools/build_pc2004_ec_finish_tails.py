#!/usr/bin/env python3
"""Build Tails' original Emerald Coast finish actor from owner files."""
from pathlib import Path
import argparse
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sa3t
from tools.build_pc2004_emerald_coast_visual import read_source
from tools.build_pc2004_sonic_sa3c import build_decoded_package
from tools.importers.pc2004_chrmodels import ActionSummary, PE32, SonicDecoder
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.verify_pc2004_ec_route import verified_reader


class FinishImage:
    """The rig is in CHRMODELS. The finish motion is in sonic.exe."""
    def __init__(self, model, executable):
        self.model = model
        self.executable = executable
        self.data = model.data

    def va_to_offset(self, address):
        return self.model.va_to_offset(address)

    def sha256(self):
        return self.model.sha256()

    def is_mapped_va(self, address, size=1):
        return self.model.is_mapped_va(address, size) or self.executable.is_mapped_va(address, size)

    def read_bytes(self, address, size):
        image = self.model if self.model.is_mapped_va(address, size) else self.executable
        return image.read_bytes(address, size)


class FinishTailsDecoder(SonicDecoder):
    def actions(self):
        root = self.pointer_table('___MILES_OBJECTS', 1)[0]
        motion_address = 0x10C6298
        nodes = self.hierarchy(root)
        motion = self.motion(motion_address, len(nodes))
        if len(nodes) != 68 or (motion.frame_count, motion.type_bits, motion.interpolation) != (60, 3, 2):
            raise ValueError('unexpected Emerald Coast finish Tails rig or motion')
        return [ActionSummary(0, 0, root, motion_address, len(nodes), motion)]


def build(exe, models, system):
    read = verified_reader(exe)
    # 7D23C0 assembles the action from MILES_OBJECTS[0] and this motion.
    if read(0x7D23C0, 23) != bytes.fromhex(
        'a124d1aa038b08890d18e4c503c7051ce4c50398620c01'
    ):
        raise ValueError('unexpected finish action initializer')
    if read(0x4F9E7B, 5) != bytes.fromhex('6818e4c503'):
        raise ValueError('unexpected O BZ actor display')
    decoder = FinishTailsDecoder(models)
    decoder.pe = FinishImage(decoder.pe, PE32(exe))
    pvm = PVMArchive(read_source(system, 'MILES.PVM'), 'MILES.PVM')
    character, report = build_decoded_package(decoder, pvm, allow_enemy_primitives=True)
    textures = sa3t.build([
        dict(width=e.width, height=e.height, source_pixel_format=e.pixel_format,
             source_data_format=e.data_format, global_index=e.global_index,
             rgba=pvm.decode_base_rgba(e.index))
        for e in pvm.entries
    ], pvm.sha256())
    return character, textures, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sonic_exe', type=Path)
    parser.add_argument('chrmodels', type=Path)
    parser.add_argument('system', type=Path)
    parser.add_argument('output_dir', type=Path)
    args = parser.parse_args()
    if args.output_dir.resolve().is_relative_to(ROOT):
        raise SystemExit('owner-generated character data must stay outside the code tree')
    character, textures, report = build(args.sonic_exe, args.chrmodels, args.system)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / 'ec_finish_tails.sa3c').write_bytes(character)
    (args.output_dir / 'ec_finish_tails.sa3t').write_bytes(textures)
    print(f'Finish Tails: {report["counts"]}, {len(character)} and {len(textures)} bytes')


if __name__ == '__main__':
    main()
