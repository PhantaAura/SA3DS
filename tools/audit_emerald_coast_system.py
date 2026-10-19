#!/usr/bin/env python3
"""Inventory Emerald Coast companion files from an owner system folder/ZIP."""
from __future__ import annotations
from pathlib import Path
from zipfile import ZipFile, BadZipFile
import argparse, hashlib, json, sys

FILES={
 'BEACH01.PVM':'act0 stage textures','BEACH02.PVM':'act1 stage textures','BEACH03.PVM':'act2 stage textures',
 'OBJ_BEACH.PVM':'Emerald Coast object textures','BEACH_SEA.PVM':'sea textures','BG_BEACH.PVM':'background textures',
 'SET0100S.BIN':'act0 Sonic SET','SET0100E.BIN':'act0 enemy SET','SET0101S.BIN':'act1 Sonic SET',
 'SET0101M.BIN':'act1 alternate-character SET','SET0102S.BIN':'act2 Sonic SET','SET0102B.BIN':'act2 alternate-character SET',
 'CAM0100S.BIN':'act0 Sonic camera','CAM0100E.BIN':'act0 alternate camera','CAM0101S.BIN':'act1 Sonic camera',
 'CAM0102S.BIN':'act2 Sonic camera','CAM0102B.BIN':'act2 alternate camera',
}

def read_sources(path:Path):
    if path.is_dir():
        mapping={p.name.upper():p for p in path.rglob('*') if p.is_file()}
        def get(name):
            p=mapping.get(name.upper()); return None if p is None else p.read_bytes()
        return get
    if path.is_file() and path.suffix.lower()=='.zip':
        z=ZipFile(path)
        mapping={Path(n).name.upper():n for n in z.namelist() if not n.endswith('/')}
        def get(name):
            n=mapping.get(name.upper()); return None if n is None else z.read(n)
        return get
    raise ValueError('source must be a system directory or ZIP')

def main(argv=None):
    ap=argparse.ArgumentParser(); ap.add_argument('source',type=Path); ap.add_argument('--json',type=Path)
    args=ap.parse_args(argv)
    try:
        get=read_sources(args.source); rows=[]; missing=[]
        for name,role in FILES.items():
            data=get(name)
            if data is None: missing.append(name); continue
            rows.append({'name':name,'role':role,'size':len(data),'sha256':hashlib.sha256(data).hexdigest()})
        report={'source':str(args.source),'recognized_count':len(rows),'expected_count':len(FILES),'missing':missing,'files':rows}
        text=json.dumps(report,indent=2,sort_keys=True)
        if args.json: args.json.write_text(text+'\n')
        print(text)
        return 0 if not missing else 2
    except (OSError,ValueError,BadZipFile) as e:
        print('error:',e,file=sys.stderr); return 1
if __name__=='__main__': raise SystemExit(main())
