#!/usr/bin/env python3
"""Offline neutral SA3DS SET bank importer, supports all recognized SADX SET files.

Emits per-stage/act object *placements* without assuming object gameplay types.
Each owner-generated .sa3dspak contains a METADATA and SET chunk, and is
intentionally excluded from redistributable source-only checkpoints.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.importers.pc2004_set import list_set_sources, build_set_chunk, parse_set_chunk, SetDecodeError
from tools import sa3dspak


def build_set_packages(system_source: Path, output_dir: Path, level_id: int | None = None) -> dict:
    sources = list_set_sources(system_source)
    if level_id is not None:
        sources = [s for s in sources if s.level_id == level_id]
    if not sources:
        raise SetDecodeError('no matching SET sources found')
    groups = defaultdict(list)
    for src in sources:
        groups[(src.level_id, src.act)].append(src)
    report = {
        'version': '0.0.9.1C', 'format': 'SA3DSET v1.0',
        'source_role': 'user-owned SADX PC system SET files',
        'source': str(system_source),
        'semantics': {
            'type_word': 'u16 raw; high bits and object class not asserted',
            'angles': '3 x signed int16 raw; no unproven angle interpretation',
            'position': '3 x float32',
            'parameters': '3 x float32, object-specific meanings unresolved; NOT assumed model scale',
            'file_variant': 'filename suffix preserved; no runtime selection meaning claimed',
            'object_spawning': 'not yet implemented',
        },
        'groups': [],
    }
    output_dir.mkdir(parents=True, exist_ok=True)
    for (level, act), files in sorted(groups.items()):
        chunk = build_set_chunk(files)
        check = parse_set_chunk(chunk)
        meta = {
            'version': report['version'],
            'source_profile': 'SADX_PC_SET_32BYTE',
            'level_id': level, 'act': act,
            'type': 'set_placements_only',
            'sources': [src.summary() for src in sorted(files, key=lambda x: x.name)],
            'validated': {'files': check['files'], 'placements': check['records']},
            'warning': 'Preserve original variant identity and all object-specific parameters; no object behaviors implemented.',
        }
        pak = sa3dspak.build([
            ('METADATA', json.dumps(meta, sort_keys=True, separators=(',', ':')).encode() + b'\n', 0),
            ('SET', chunk, 0),
        ])
        result = sa3dspak.parse(pak)
        if len(result['chunks']) != 2 or result['chunks'][1]['type'] != 'SET':
            raise SetDecodeError('SET package self-check failed')
        output_name = f'set_level{level:02d}_act{act:02d}.sa3dspak'
        dest = output_dir / output_name
        dest.write_bytes(pak)
        report['groups'].append({
            'level_id': level, 'act': act,
            'files': [f.name for f in files],
            'placements': check['records'],
            'output': output_name,
            'sha256': hashlib.sha256(pak).hexdigest(),
            'bytes': len(pak),
        })
    report['total_sources'] = len(sources)
    report['total_placements'] = sum(len(s.records) for s in sources)
    report['total_groups'] = len(groups)
    return report


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('system_source', type=Path)
    ap.add_argument('output_dir', type=Path)
    ap.add_argument('--level-id', type=int, help='only build this numeric level ID, e.g. 1 for Emerald Coast')
    ap.add_argument('--report', type=Path)
    args = ap.parse_args(argv)
    try:
        if args.level_id is not None and not (0 <= args.level_id <= 99):
            raise SetDecodeError('level ID must be 0..99')
        result = build_set_packages(args.system_source, args.output_dir, args.level_id)
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n')
        print(json.dumps({k: result[k] for k in ('version', 'total_sources', 'total_placements', 'total_groups')}, sort_keys=True))
        return 0
    except (OSError, SetDecodeError, ValueError) as exc:
        print(f'error: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
