#!/usr/bin/env python3
"""Offline user-owned CAM source bank importer; preserves raw records, not camera AI.

Numeric CAMnnnnX files are grouped by numeric level/act. Named CAM files
(Adventure Fields, missions, kart, Chao, etc) are packaged individually to
avoid fabricating stage association from filenames alone.
"""
from __future__ import annotations
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_cam import list_cam_sources, camera_group, build_cam_chunk, parse_cam_chunk, CamDecodeError
from tools import sa3dspak

def build_cam_packages(system_source:Path, output_dir:Path,level_id:int|None=None)->dict:
    sources=list_cam_sources(system_source)
    if level_id is not None:
        sources=[s for s in sources if camera_group(s.name) and camera_group(s.name)[0]==level_id]
    if not sources:raise CamDecodeError('no matching CAM source files')
    groups=defaultdict(list)
    for src in sources:
        tag=camera_group(src.name)
        group=('numeric',tag[0],tag[1]) if tag else ('named',src.name)
        groups[group].append(src)
    output_dir.mkdir(parents=True,exist_ok=True)
    report={'version':'0.0.9.1D','source_profile':'SADX_PC_CAM_64BYTE',
            'source':str(system_source),'total_files':len(sources),
            'total_records':sum(len(s.records) for s in sources),'groups':[],
            'camera_behavior_implemented':False,
            'warning':'CAM data only: no camera algorithm, trigger semantics, or variant priorities proven.'}
    for group,files in sorted(groups.items()):
        chunk=build_cam_chunk(files);check=parse_cam_chunk(chunk)
        meta={'source_profile':'SADX_PC_CAM_64BYTE','source_files':[s.summary() for s in files],
              'numeric_group':group if group[0]=='numeric' else None,
              'warning':'Raw camera setup: no runtime camera rules inferred.'}
        out=sa3dspak.build([('METADATA',json.dumps(meta,sort_keys=True,separators=(',',':')).encode()+b'\n',0),
                            ('CAMERA',chunk,0)])
        validated=sa3dspak.parse(out)
        if [c['type'] for c in validated['chunks']]!=['METADATA','CAMERA']:
            raise CamDecodeError('CAM .sa3dspak integration failure')
        name=(f'cam_level{group[1]:02d}_act{group[2]:02d}.sa3dspak' if group[0]=='numeric'
              else group[1].removesuffix('.BIN').lower()+'.sa3dspak')
        (output_dir/name).write_bytes(out)
        report['groups'].append({'group':group,'files':[s.name for s in files],
                    'records':check['records'],'output':name,'bytes':len(out),
                    'sha256':hashlib.sha256(out).hexdigest()})
    report['total_groups']=len(groups)
    return report

def main(argv=None)->int:
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('system_source',type=Path)
    ap.add_argument('output_dir',type=Path)
    ap.add_argument('--level-id',type=int)
    ap.add_argument('--report',type=Path)
    a=ap.parse_args(argv)
    try:
        if a.level_id is not None and not 0<=a.level_id<=99:
            raise CamDecodeError('level ID must be 0..99')
        result=build_cam_packages(a.system_source,a.output_dir,a.level_id)
        if a.report:
            a.report.parent.mkdir(parents=True,exist_ok=True)
            a.report.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
        print(json.dumps({k:result[k] for k in ('version','total_files','total_records','total_groups')},sort_keys=True))
        return 0
    except (OSError,ValueError,CamDecodeError) as exc:
        print(f'error: {exc}',file=sys.stderr);return 1
if __name__=='__main__':raise SystemExit(main())
