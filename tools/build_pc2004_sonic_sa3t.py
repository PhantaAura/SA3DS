#!/usr/bin/env python3
"""Build generated SA3T from the owner's exact SONIC.PVM."""
from pathlib import Path
import argparse,hashlib,json,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_pvm import SonicPVM
from tools import sa3t

def build_package(path:Path):
    p=SonicPVM(path); entries=[]
    for e in p.entries:
        entries.append({'width':e.width,'height':e.height,'source_pixel_format':e.pixel_format,'source_data_format':e.data_format,'global_index':e.global_index,'rgba':p.decode_base_rgba(e.index)})
    package=sa3t.build(entries,p.sha256())
    return package,{'schema':'sa3ds-pc2004-sonic-sa3t-build-v1','source_sonic_pvm_sha256':p.sha256(),'texture_count':len(entries),'rgba8_pixel_bytes':sum(len(e['rgba']) for e in entries),'package_bytes':len(package),'package_sha256':hashlib.sha256(package).hexdigest()}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('pvm',type=Path);ap.add_argument('output',type=Path);ap.add_argument('--report',type=Path);a=ap.parse_args()
    pkg,rep=build_package(a.pvm);a.output.write_bytes(pkg)
    if a.report:a.report.write_text(json.dumps(rep,indent=2)+'\n')
    print(json.dumps(rep,indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
