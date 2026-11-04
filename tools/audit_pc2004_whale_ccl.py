#!/usr/bin/env python3
"""Compare decoded CCI metadata with owner bytes and exercise actual contact rules."""
from pathlib import Path
import argparse,struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.build_pc2004_whale_ccl import build,SPANS
from tools.verify_pc2004_ec_route import verified_reader
def audit(exe):
 data=build(exe);r=verified_reader(exe)
 payload=next(c['data'] for c in sa3dspak.parse(data)['chunks'] if c['type']=='WHALE_CCL')
 with tempfile.TemporaryDirectory(prefix='sa3ds-owner-whale-ccl-') as d:
  root=Path(d);good=root/'good.sa3dspak';out=root/'decoded.bin';good.write_bytes(data)
  expected=b''.join(r(0x10c8790+i*48+1,3)+r(0x10c8790+i*48+4,4)+r(0x10c8790+i*48+20,4) for i in range(2))
  expected+=b''.join(r(a+2,1) for a in (0x90e3bc,0x90e3b0,0x90e3c8))+r(0x90e3dc,4)+r(0x502adb,4)
  changes=[('version',8,struct.pack('<I',2)),('count',12,struct.pack('<I',1)),('offset',16,struct.pack('<I',32)),('stride',20,struct.pack('<I',44)),('total',24,struct.pack('<I',143)),('reserved',28,struct.pack('<I',1)),('category',32,b'\x03'),('broad',40,struct.pack('<I',0x7fc00000)),('configreserved',44,struct.pack('<I',1)),('kind',48,b'\x01'),('form',49,b'\x03'),('localcenter',56,struct.pack('<f',1)),('radius',68,struct.pack('<f',-1)),('dimension',72,struct.pack('<f',1)),('tiprelative',52,struct.pack('<I',0x400)),('bodyabsolute',100,struct.pack('<I',0x20))]
  bad_paths=[]
  for name,offset,value in changes:
   p=bytearray(payload);p[offset:offset+len(value)]=value;bad=root/(name+'.sa3dspak')
   bad.write_bytes(sa3dspak.build([('METADATA',b'{}',0),('WHALE_CCL',bytes(p),0)]))
   bad_paths.append(bad)
  p=bytearray(data);p[-1]^=1;bad=root/'crc.sa3dspak';bad.write_bytes(p);bad_paths.append(bad)
  subprocess.run(['sh',str(ROOT/'tests/run_whale_ccl.sh'),str(good),str(out),*map(str,bad_paths)],check=True)
  assert out.read_bytes()==expected
 print(f'Whale CCL owner oracle: PASS {len(SPANS)} exact spans,33 decoded metadata bytes,two real shape contact cases,16 malformed profiles+CRC; native queue/chase separate')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);a=ap.parse_args();audit(a.exe)
