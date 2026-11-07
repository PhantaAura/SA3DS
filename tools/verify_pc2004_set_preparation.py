#!/usr/bin/env python3
"""Exact-owner SET cold preparation, act clear/compaction and restart reset witnesses."""
from pathlib import Path
import argparse,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
def verify(exe):
 r=verified_reader(exe)
 #46C1D0 zeroes work entries, then both stage-entry branches append via46BD20.
 def branches(lo,hi,target): #call or tail jump rel32
  return [a for a in range(lo,hi) if r(a,1) in (b'\xe8',b'\xe9') and a+5+struct.unpack('<i',r(a+1,4))[0]==target]
 assert len(branches(0x46c1d0,0x46c2a0,0x46bd20))==2
 assert r(0x46bd90,6)==bytes.fromhex('66814e020080') #flags|=0x8000 per appended row
 assert r(0x46bd96,5)==bytes.fromhex('a100ba9100') #mission-object comparison global
 assert r(0x46be23,5)==bytes.fromhex('25e97f0000') #act clear mask7FE9
 assert r(0x46be28,2)==bytes.fromhex('a801') #old active bit selects deferred free
 assert r(0x46be32,7)==bytes.fromhex('c7401070b54000') #main=40B570, no inline free
 assert r(0x46bf51,3)==bytes.fromhex('f60010') #restart keeps0x10 rows
 assert r(0x46bf56,5)==bytes.fromhex('66c7000080') #others become flags0x8000
 #46C2A0 act-change machine: step0 clear, step2 compaction, step3 append.
 for target in (0x46be00,0x46be50,0x46bd20):
  assert branches(0x46c2a0,0x46c330,target),hex(target)
 assert branches(0x49dd70,0x49de20,0x46bf20) #49DD70 restart reset
 print('SET preparation owner witnesses: PASS exact SHA,46C1D0->46BD20 0x8000 append,mission global,46BE00 7FE9/40B570,46C2A0 clear/compact/append order,49DD70->46BF20 0x10-preserving reset; native fixed-row projection only')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);a=ap.parse_args();verify(a.exe)
