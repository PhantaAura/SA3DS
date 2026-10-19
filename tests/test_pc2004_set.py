#!/usr/bin/env python3
"""Tests owner-provided SADX SET sources and adversarial synthetic inputs."""
from __future__ import annotations
from collections import Counter
import hashlib
from pathlib import Path
import struct
import sys
from tempfile import TemporaryDirectory
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools.importers.pc2004_set import (
    HEADER, DIRECTORY, SOURCE_ROW, SOURCE_HEADER_SIZE,
    SetDecodeError, read_set_source, list_set_sources, source_name_info,
    parse_placement, build_set_chunk, parse_set_chunk,
)
from tools.build_pc2004_set_packages import build_set_packages
from tools.build_pc2004_emerald_coast_visual import (
    read_source as read_visual_source, build_act_package,
)
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools import sa3dspak


def raises(fn, msg=None):
    try:
        fn()
    except (SetDecodeError, ValueError, UnicodeError):
        return
    raise AssertionError(msg or 'expected input rejection')


def fixture(name='SET0100S.BIN'):
    return struct.pack('<I', 2) + bytes(28) + \
        SOURCE_ROW.pack(0x203A, -100, 200, -300, 1.5, -2.5, 3.25, 0, 100, -1) + \
        SOURCE_ROW.pack(0x1001, 0, -32768, 32767, -10, 0, 10, -4, 0.5, 0)


def test_adversarial():
    raw = fixture()
    src = read_set_source('SET0100S.BIN', raw)
    assert src.source_bytes() == raw
    assert src.records[0].type_word == 0x203A
    assert src.records[0].parameters == (0, 100, -1)
    assert src.records[1].angle_words == (0, -32768, 32767)
    neutral = build_set_chunk([src])
    got = parse_set_chunk(neutral)
    assert got['records'] == 2 and got['files'] == 1
    assert got['sources'][0]['sha256'] == hashlib.sha256(raw).hexdigest()
    assert neutral == build_set_chunk([src])

    raises(lambda: read_set_source('WRONG.DAT', raw))
    raises(lambda: read_set_source('SET0100S.BIN', raw[:29]))
    raises(lambda: read_set_source('SET0100S.BIN', raw[:-1]))
    raises(lambda: read_set_source('SET0100S.BIN', struct.pack('<I', 500000) + raw[4:]))
    raises(lambda: read_set_source('SET0100S.BIN', raw[:4] + b'\x01' + raw[5:]))
    bad_nan = bytearray(raw)
    struct.pack_into('<I', bad_nan, 32 + 8, 0x7fc00000)
    raises(lambda: read_set_source('SET0100S.BIN', bytes(bad_nan)))
    raises(lambda: build_set_chunk([src, src]))
    for off in [0, 8, HEADER.size-1, len(neutral)-1]:
        b = bytearray(neutral)
        b[off] ^= 0x80
        raises(lambda b=b: parse_set_chunk(bytes(b)), f'undetected SET corruption at {off}')
    b = bytearray(neutral)
    # Corrupt directory source digest while leaving rows checksum intact.
    b[48 + 16 + 4 + 4] ^= 0x80
    raises(lambda: parse_set_chunk(bytes(b)))
    b = bytearray(neutral)
    _, _, _, _, _, _, _, row_off, _, _ = HEADER.unpack_from(b)
    b[row_off + 6] ^= 1
    raises(lambda: parse_set_chunk(bytes(b)))
    assert source_name_info('SET9999S.BIN') == (99,99,'S')
    with TemporaryDirectory() as d:
        with ZipFile(Path(d)/'dupe.zip','w') as z:
            z.writestr('system/SET0100S.BIN',raw)
            z.writestr('mods/SET0100S.BIN',raw)
        raises(lambda: list_set_sources(Path(d)/'dupe.zip'))
    print('Synthetic SET malformed-input and roundtrip tests: PASS')


def test_owner(system_source: Path, exe_path: Path | None):
    sources = list_set_sources(system_source)
    assert len(sources) == 71, len(sources)
    assert sum(len(s.records) for s in sources) == 30085
    expected = {
        'SET0100E.BIN': 567,
        'SET0100S.BIN': 789,
        'SET0101M.BIN': 969,
        'SET0101S.BIN': 996,
        'SET0102B.BIN': 342,
        'SET0102S.BIN': 1,
    }
    ec = [s for s in sources if s.name in expected]
    assert len(ec) == len(expected)
    for s in ec:
        assert len(s.records) == expected[s.name]
        assert read_visual_source(system_source, s.name) == s.source_bytes()
    # Every parsed SET passes lossless source reconstruction and float checks.
    for s in sources:
        assert hashlib.sha256(s.source_bytes()).hexdigest() == s.sha256
    for act, want in [(0, 1356), (1, 1965), (2, 343)]:
        chunk = build_set_chunk([s for s in ec if s.act == act])
        info = parse_set_chunk(chunk)
        assert info['records'] == want
        assert info['files'] == 2
        assert info['bytes'] <= 100000
    with TemporaryDirectory() as d:
        result = build_set_packages(system_source, Path(d)/'out', level_id=1)
        assert (result['total_groups'], result['total_sources'], result['total_placements']) == (3,6,3664)
        for item in result['groups']:
            pak = sa3dspak.parse((Path(d)/'out'/item['output']).read_bytes())
            assert [c['type'] for c in pak['chunks']] == ['METADATA', 'SET']
            assert parse_set_chunk(pak['chunks'][1]['data'])['records'] == item['placements']
        all_result = build_set_packages(system_source, Path(d)/'all')
        assert all_result['total_sources'] == 71
        assert all_result['total_placements'] == 30085
        assert all_result['total_groups'] >= 3
        assert all('SET' in [c['type'] for c in sa3dspak.parse((Path(d)/'all'/r['output']).read_bytes())['chunks']] for r in all_result['groups'])
    if exe_path is not None:
        dec = PC2004StageDecoder(exe_path)
        for act in range(3):
            table = dec.landtable(1, act)
            pvm = read_visual_source(system_source, table.texture_archive+'.PVM')
            act_sources = [s for s in ec if s.act == act]
            package, metadata = build_act_package(dec, act, pvm, act_sources)
            parsed = sa3dspak.parse(package)
            assert [c['type'] for c in parsed['chunks']] == ['METADATA', 'GEOMETRY', 'MATERIALS', 'TEXTURES', 'SET']
            assert parse_set_chunk(parsed['chunks'][-1]['data'])['records'] == sum(len(s.records) for s in act_sources)
            assert metadata['set_placements']['spawn_behavior_implemented'] is False
            assert metadata['set_placements']['variant_selection_unknown'] is True
    print('Owner SADX SET importer tests: 71 sources / 30,085 placements / 3 EC acts PASS')


def main():
    test_adversarial()
    if len(sys.argv) not in (2,3):
        raise SystemExit('usage: test_pc2004_set.py SYSTEM_DIR_OR_ZIP [EXACT_PC2004_SONIC_EXE]')
    test_owner(Path(sys.argv[1]), Path(sys.argv[2]) if len(sys.argv) == 3 else None)


if __name__ == '__main__':
    main()
