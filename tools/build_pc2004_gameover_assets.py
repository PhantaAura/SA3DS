#!/usr/bin/env python3
"""Convert owner game-over menu art; output must remain outside source tree."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools import sa3t,sa3dspak
from tools.verify_pc2004_ec_route import verified_reader

def build(exe,archive):
 verified_reader(exe)
 #Exact source routines: PVRT format/size loads, PVMH count/global-index path.
 from tools.verify_pc2004_gameover import verify
 verify(exe)
 p=PVMArchive(archive,pixel_format_from_pvrt=True)
 if p.texture_count!=7:raise ValueError('expected original seven-image menu')
 entries=[]
 for i,e in enumerate(p.entries):
  if e.width>256 or e.height>256:raise ValueError('menu image exceeds native profile')
  entries.append(dict(width=e.width,height=e.height,source_pixel_format=e.pixel_format,source_data_format=e.data_format,global_index=e.global_index,rgba=p.decode_base_rgba(i)))
 if [(e.width,e.height) for e in p.entries[:5]]!=[(8,8),(256,32),(128,64),(128,64),(128,64)]:raise ValueError('unexpected menu layout')
 t=sa3t.build(entries,p.sha256());sa3t.parse(t)
 meta={'scope':'private original game-over art; PVRT pixel format authority','source_exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'source_pvm_sha256':p.sha256()}
 return sa3dspak.build([('METADATA',json.dumps(meta,sort_keys=True).encode(),0),('TEXTURES',t,0)])
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('pvm',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise SystemExit('owner-generated assets must stay outside source tree')
 data=build(a.exe,a.pvm);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(json.dumps({'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}))
