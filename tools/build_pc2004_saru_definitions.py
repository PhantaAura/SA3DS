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
 (0x4abb90,0x13f0):'1e5169fd5fa6f2338f1ae7cfb355599a235e2c5e79948895fab1ce4576c9f065',
 (0x4cdf60,0x79):'2a3fa14d739028e9c3160d55d133d98609f3139c9b86dfe33dad94b0a0c0db24',
 (0x4cd610,0x60):'d5a87912f0abdf1497f7e56f381a30151fc470c36fd8336a10cab6aa48ff7f41',
 (0x4cd670,0x75):'c72a50fac64acdee9767742f19e8ba66817286b399980a9aab55e0d4b40541dd',
 (0x438350,0x42):'f06c90f9b9d76ea93112938f0a02d1b641ab73b96a847038b1e5eceffe2dee15',
 (0x406090,0x168):'5438c8be404a80093ada88a3ecf77a0c53bcc1505ea04ae3a4ab57d84c97de4c',
 (0x406680,0x1cf):'ca4797a5d82ed379460b412a02bd2334ebcb5682182a41cb7c9d9b101a2b2883',
 (0x406c40,0x27c):'577f852b6b0b4ee0af6f6798daab7333b768043ad1b5aae9e2bada938c7ca87b',
 (0x406850,0x146):'5227f3edde2813c72048007ca2432e94f7fce259dad57c43953b4e229071bb5a',
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
 co=64+len(rows)*40;blob=bytearray(co+48+112)
 struct.pack_into('<8s9I4fI',blob,0,b'SA3SARU\0',2,len(rows),64,40,co,48,len(blob),3,2,
     registry[3],struct.unpack('<f',r(0x4acfa1,4))[0],struct.unpack('<f',r(0x4ab2e6,4))[0],struct.unpack('<f',r(0x7dcb30,4))[0],co+48)
 for i,row in enumerate(rows):struct.pack_into('<II32s',blob,64+i*40,*row)
 blob[co:co+48]=r(0x96c9e8,48)
 blob[co+48:co+144]=r(0x91cbf0,96)
 for index,category in enumerate((0,3)):
  struct.pack_into('<BBB x I',blob,co+144+index*8,
      r(0x90e3bc+category,1)[0],r(0x90e3b0+category,1)[0],r(0x90e3c8+category,1)[0],struct.unpack('<I',r(0x90e3d4+category*4,4))[0])
 metadata={'source_profile':'PC2004-Saru-definitions-v2','source_exe_sha256':PROFILE_SHA256,
  'source_set_sha256':hashes,'counts_by_act':counts,'scope':'original placements/CCL/init and player collision profile; runtime/assets remain separate'}
 data=sa3dspak.build([('METADATA',json.dumps(metadata,sort_keys=True).encode(),0),('SARU',blob,0)])
 sa3dspak.parse(data);return data,counts
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('sonic_exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);args=ap.parse_args()
 if args.output.resolve().is_relative_to(ROOT):raise SystemExit('owner output must remain outside source tree')
 data,counts=build(args.sonic_exe,args.system);args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(data)
 print(json.dumps({'counts':counts,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}))
