#!/usr/bin/env python3
"""Compare runtime whale trigger fields with independently parsed owner SET rows."""
from pathlib import Path
import argparse,struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.build_pc2004_whale_triggers import build
from tools.build_pc2004_emerald_coast_visual import read_source
from tools.importers.pc2004_set import read_set_source
def audit(exe,system):
 data=build(exe,system);payload=next(c['data'] for c in sa3dspak.parse(data)['chunks'] if c['type']=='WHALE_TRIG')
 with tempfile.TemporaryDirectory(prefix='sa3ds-owner-whale-triggers-') as directory:
  root=Path(directory);good=root/'triggers.sa3dspak';out=root/'rows.bin';good.write_bytes(data);bad=[]
  count,_,spec=struct.unpack_from('<3I',payload,12)
  changes=[('version',8,struct.pack('<H',2)),('minor',10,struct.pack('<H',1)),('count',12,struct.pack('<I',65)),('offset',16,struct.pack('<I',28)),('spec',20,struct.pack('<I',32)),('specsize',24,struct.pack('<I',12)),('total',28,struct.pack('<I',len(payload)-4)),('act',32,struct.pack('<I',2)),('type',40,struct.pack('<H',69)),('nan',48,struct.pack('<I',0x7fc00000)),('radius',spec,struct.pack('<f',44)),('distance',spec+4,struct.pack('<f',160001)),('reserved',spec+8,struct.pack('<I',1)),('roworder',72+4,payload[36:40])]
  for name,offset,value in changes:
   p=bytearray(payload);p[offset:offset+len(value)]=value;path=root/(name+'.sa3dspak')
   path.write_bytes(sa3dspak.build([('METADATA',b'{}',0),('WHALE_TRIG',bytes(p),0)]));bad.append(path)
  crc=root/'crc.sa3dspak';p=bytearray(data);p[-1]^=1;crc.write_bytes(p);bad.append(crc)
  subprocess.run(['sh',str(ROOT/'tests/run_whale_trigger_assets.sh'),str(good),str(out),*map(str,bad)],check=True,stdout=subprocess.DEVNULL)
  expected=[]
  for act in (0,1):
   s=read_set_source(f'SET010{act}S.BIN',read_source(system,f'SET010{act}S.BIN'))
   expected.extend(struct.pack('<II',act,index)+row.raw for index,row in enumerate(s.records) if row.type_word&4095 in (67,68))
  assert len(expected)==count==4 and out.read_bytes()==b''.join(expected)
 print('Whale trigger assets owner oracle: PASS4 complete decoded SET rows/angles/parameter bits against original files,14 malformed+CRC rejection; native spawn/clip scheduling separate')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);a=ap.parse_args();audit(a.exe,a.system)
