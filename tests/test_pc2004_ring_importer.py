"""Source-neutral ring importer regression; no owner resources required."""
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.build_pc2004_ring_assets import (
    HEADER, ROW, SPEC, build_ring_placements, compact_textures, owner_clip_level,
)
from tools.importers.pc2004_set import read_set_source
from tools import sa3t


def source(act, rows):
    return read_set_source(f'SET010{act}S.BIN', struct.pack('<I', len(rows)) + bytes(28) + b''.join(rows))


def placement(type_word, position):
    return struct.pack('<Hhhhffffff', type_word, -32768, 100, 32767, *position, 9.25, -2.5, 7.)


raw0 = placement(0, (1., 2., 3.))
raw_high = placement(0xA000, (100., -50., -20.))
raw_other = placement(1, (7., 8., 9.))
raw1 = placement(0, (-9., 15., 31.))
spec = (1., 2., 3., 7., 1600., 1700., 256, 2)
body, rows = build_ring_placements([source(1, [raw_other, raw1]), source(0, [raw0, raw_high, raw_other])], spec)
magic, version, flags, acts, count, offset, spec_offset, size = HEADER.unpack_from(body)
assert (magic, version, flags, acts, count, offset) == (b'SA3DRING', 1, 0, 2, 3, 32)
assert size == len(body) == spec_offset + 32 and spec_offset == offset + count * 40
# Original source indices and all32bytes must survive, including highbits,
# signed angles and parameters that are not a guessed object scale.
assert [ROW.unpack_from(body, offset + i * ROW.size) for i in range(count)] == [
    (0, 0, raw0), (0, 1, raw_high), (1, 1, raw1),
]
assert SPEC.unpack_from(body, spec_offset) == spec
for sources in ([source(0, [raw0])], [source(0, [raw0]), source(0, [raw1])]):
    try:
        build_ring_placements(sources, spec)
    except ValueError:
        pass
    else:
        raise AssertionError('invalid act source set accepted')


class FakePVM:
    texture_count = 4
    entries = [SimpleNamespace(width=8, height=8, pixel_format=0, data_format=9, global_index=i) for i in range(4)]
    def sha256(self):
        return '00' * 32
    def decode_base_rgba(self, source):
        return bytes([source, 20, 30, 255]) * 64


materials = [(0x11223344, 0xAABBCCDD, .5, 3, 17), (0x55667788, 0, 1., 1, 19), (7, 8, 2., 3, 23)]
remapped, texture_blob, used = compact_textures(materials, FakePVM())
assert used == [1, 3]
assert remapped == [(0x11223344, 0xAABBCCDD, .5, 1, 17), (0x55667788, 0, 1., 0, 19), (7, 8, 2., 1, 23)]
parsed = sa3t.parse(texture_blob)
assert len(parsed['textures']) == 2
for local, source_index in enumerate(used):
    d = parsed['textures'][local]
    assert texture_blob[d.data_offset:d.data_offset+d.data_size] == bytes([source_index, 20, 30, 255]) * 64
try:
    compact_textures([(0, 0, 1., 5, 0)], FakePVM())
except ValueError:
    pass
else:
    raise AssertionError('out-of-range texture accepted')
print('Ring importer: PASS (raw direct rows/highbits/act identity, source indices, compact texture remap/material preservation; source-neutral)')

with tempfile.TemporaryDirectory() as tmp:
    executable = Path(tmp) / 'sonic.exe'
    assert owner_clip_level(executable)[0] == 1
    ini = executable.parent / 'sonicDX.ini'
    ini.write_text('[SONICDX]\ncliplevel=0\n')
    level, evidence = owner_clip_level(executable)
    assert level == 0 and len(evidence['ini_sha256']) == 64
    ini.write_text('[sonicDX]\ncliplevel=2\n')
    assert owner_clip_level(executable)[0] == 2
    ini.write_text('[sonicDX]\n')
    assert owner_clip_level(executable)[0] == 1
    for bad in ('3', '-1', 'maybe'):
        ini.write_text('[sonicDX]\ncliplevel=' + bad + '\n')
        try:
            owner_clip_level(executable)
        except ValueError:
            pass
        else:
            raise AssertionError('unsupported clip setting accepted')
print('Ring importer owner configuration: PASS (case-insensitive section, level0/2, missing fallback1, invalid rejection; synthetic)')
