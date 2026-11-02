#!/usr/bin/env python3
"""Preserve owner's original E SARU definitions/CCL outside the source tree."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader,PROFILE_SHA256
from tools.importers.pc2004_set import read_set_source
from tools.build_pc2004_emerald_coast_visual import read_source
RANGES={
 (0x4ab3f0,0x1e9):'11cc076c71ca7dc116271cf70562536b69e8f0ebbec10407b1cf081a48792778',
 (0x4ab5e0,0x1d0):'e79eb5b64c83e5919b90b8fadbb29347a9d75ac870da4563b6886569a216344e',
 (0x4acf80,0x18c):'347856f742fa766125b305576dd8c347ed44e0e1f08b350728ad268c08ffda80',
 (0x4ad140,0x176):'daff40d5f8c0fadc8ee6c366f5bb501671840ac0b158b7dabe492714ffe003a0',
 (0x4ab240,0x63):'0b53fbc95427e58ecaddf5b66d24fa9ea1035257d3b83881140d7536b34cec06',
 (0x4ab2b0,0x139):'0a251f6f379cf41ee098352adadd2efba4c37c5bb842b789855777a6eeb7294d',
}
def build(exe,system):
 r=verified_reader(exe)
 for (a,n),digest in RANGES.items():
  if hashlib.sha256(r(a,n)).hexdigest()!=digest:raise ValueError('Saru code witness mismatch')
 registry=struct.unpack('<BBHfIII',r(0x1036420+29*20,20))
 if registry[:6]!=(2,3,1,360000.,0,0x4ad140):raise ValueError('Saru registry mismatch')
 rows=[];counts=[];hashes=[]
 for act in (0,1):
  source=read_set_source(f'SET010{act}S.BIN',read_source(system,f'SET010{act}S.BIN'));n=0
  for index,p in enumerate(source.records):
   if p.type_word&4095==29:rows.append((act,index,p.raw));n+=1
  counts.append(n);hashes.append(source.sha256)
 co=64+len(rows)*40;blob=bytearray(co+48)
 struct.pack_into('<8s9I4fI',blob,0,b'SA3SARU\0',1,len(rows),64,40,co,48,len(blob),3,2,
     registry[3],struct.unpack('<f',r(0x4acfa1,4))[0],struct.unpack('<f',r(0x4ab2e6,4))[0],struct.unpack('<f',r(0x7dcb30,4))[0],0)
 for i,row in enumerate(rows):struct.pack_into('<II32s',blob,64+i*40,*row)
 blob[co:]=r(0x96c9e8,48)
 metadata={'source_profile':'PC2004-Saru-definitions-v1','source_exe_sha256':PROFILE_SHA256,
  'source_set_sha256':hashes,'counts_by_act':counts,'scope':'placements/CCL/init only; original AI/model clips/projectile/death integration pending'}
 data=sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),('SARU',blob,0)])
 sa3dspak.parse(data);return data,counts
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('sonic_exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);args=ap.parse_args()
 if args.output.resolve().is_relative_to(ROOT):raise SystemExit('owner output must remain outside source tree')
 data,counts=build(args.sonic_exe,args.system);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
 print(json.dumps({'counts':counts,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}))
