#!/usr/bin/env python3
"""Owner-only AO summon/kill trigger data; no whale art or native chase yet."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader,PROFILE_SHA256
from tools.importers.pc2004_set import read_set_source
from tools.build_pc2004_emerald_coast_visual import read_source
RANGES={
 (0x4fbda0,160):'677b45907ddfde950b8a29427f553c97e7bc90847f2323ef1c7af46492e71345',
 (0x4fbe40,32):'c64f0128a805f15495a2ab8fcdf53cfaf39037b8dedc3ca8c01fbcab42ba4615',
 (0x4fbc90,80):'e064264bae796f27e402e0704234ff8c469ddfa4012fba50f976e4718bf152da',
 (0x4fbce0,32):'91affabeec6922fd286b371860a2d3e5658fb9168b90f4eef8794c47f9f758cf',
 (0x4f88b0,96):'ae159807ed81c9733b6ba6ba28fb93069977f765de4db98f1d0c412eb49cb8ab',
 (0x4f8910,32):'01bf9cd8869cbe4a36fce4eea49c1c487e849a610734667c5d01dcac19856089',
 (0x441840,144):'cbde7fd5389ecceb377c2139835c788af9c8d3ef2f58982feae67b3b2e52daab',
 (0x5029f0,304):'bbab042a490ca6ad8da75f718a97f873ecca1b508f80730b7fdc491c6c85a70c',
}
def build(exe,system):
 r=verified_reader(exe)
 for (a,n),digest in RANGES.items():assert hashlib.sha256(r(a,n)).hexdigest()==digest,hex(a)
 for kind,callback in ((67,0x4fbe40),(68,0x4fbce0)):
  assert struct.unpack('<BBHfIII',r(0x1036420+kind*20,20))[:6]==(2,3,1,160000.,0,callback)
 assert r(0x4fbe20,5)==bytes.fromhex('6800003442') #radius45, not squared
 assert r(0x4fbe2e,3)==bytes.fromhex('83f801') #summon requires player0
 assert r(0x4fbcce,2)==bytes.fromhex('85c0') #kill accepts either player
 assert r(0x4f88c2,5)==bytes.fromhex('68f0295000') #actual AO constructor
 rows=[];sources=[]
 for act in (0,1):
  s=read_set_source(f'SET010{act}S.BIN',read_source(system,f'SET010{act}S.BIN'))
  sources.append({'name':s.name,'sha256':s.sha256})
  for index,p in enumerate(s.records):
   if p.type_word&0xfff in (67,68):rows.append((act,index,p.raw))
 if [(a,i,struct.unpack_from('<H',raw)[0]&0xfff) for a,i,raw in rows]!=[(0,435,67),(0,693,68),(0,705,67),(0,725,68)]:
  raise ValueError('unsupported exact owner trigger bank layout')
 head=struct.Struct('<8sHHIIIII');row=struct.Struct('<II32s');off=head.size;spec=off+len(rows)*row.size
 data=bytearray(spec+16);head.pack_into(data,0,b'SA3WTRIG',1,0,len(rows),off,spec,16,len(data))
 for i,values in enumerate(rows):row.pack_into(data,off+i*row.size,*values)
 #Radius and registry load distance are witnessed; caller clip service remains separate.
 struct.pack_into('<ffff',data,spec,45.,160000.,0.,0.)
 metadata=json.dumps({'source_exe_sha256':PROFILE_SHA256,'sources':sources,'status':'trigger-data-only; whale AI/art/camera/pier/native pending'},sort_keys=True).encode()
 package=sa3dspak.build([('METADATA',metadata,0),('WHALE_TRIG',bytes(data),0)])
 parsed=sa3dspak.parse(package);assert next(c['data'] for c in parsed['chunks'] if c['type']=='WHALE_TRIG')==bytes(data)
 print('Whale trigger owner witnesses: PASS8 exact spans,source registry/player distinction,4 raw SET rows; native chase absent')
 return package
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner packages must stay outside source tree')
 data=build(a.exe,a.system);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'Private package {len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
