#!/usr/bin/env python3
"""Compare all borrowed pitch words against separately addressed owner rows."""
from pathlib import Path
import argparse,struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.build_pc2004_whale_tip import build
from tools.verify_pc2004_ec_route import verified_reader
def audit(exe):
 data=build(exe);r=verified_reader(exe)
 with tempfile.TemporaryDirectory(prefix='sa3ds-owner-whale-tip-') as directory:
  root=Path(directory);good=root/'tip.sa3dspak';good.write_bytes(data)
  payload=next(c['data'] for c in sa3dspak.parse(data)['chunks'] if c['type']=='WHALE_TIP')
  invalid=[]
  for name,offset,value,flags in [('version',8,2,0),('count',12,31,0),('offset',16,28,0),('stride',20,8,0),('size',24,156,0),('reserved',28,1,0),('pitch',32,32768,0),('flags',28,0,1)]:
   b=bytearray(payload);struct.pack_into('<I',b,offset,value);p=root/(name+'.sa3dspak')
   p.write_bytes(sa3dspak.build([('METADATA',b'{}',0),('WHALE_TIP',bytes(b),flags)]));invalid.append(p)
  output=subprocess.check_output(['sh',str(ROOT/'tests/run_whale_tip.sh'),str(good),*map(str,invalid)],text=True)
  keys=[int(line.split()[2]) for line in output.splitlines() if line.startswith('KEY ')]
  expected=[struct.unpack('<i',r(0x10ad238+i*16+4,4))[0] for i in range(32)]
  assert keys==expected
 print('Whale tip owner oracle: PASS32 exact signed pitch words,4 source spans,CRC and8 malformed package rejections; native tip execution separate')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);a=ap.parse_args();audit(a.exe)
