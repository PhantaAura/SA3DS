#!/usr/bin/env python3
"""Stage the owner's Emerald Coast sound-effect banks to an SD folder.

Checks the exact PC2004 exe: the 425xxx id->bank threshold table (0x910090,
{threshold, bank} pairs, descending, -1 terminated; ids above a threshold
map to that bank, index = id - threshold - 1; ids <= 39 are bank 0), the
B%02d_00_%02d.wav entry-name format (0x7DEBBC) and the bank sets used for
Emerald Coast (424C80 level1 -> set0) and Sonic (set1). Copies the six .dat
banks unchanged and writes se_map.sa3dspak (SE_THRESH int32 pairs, SE_SLOTS
slot->file names)."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader
def table(r):
 rows=[];i=0
 while True:
  t,b=struct.unpack('<ii',r(0x910090+i*8,8));rows.append((t,b))
  if t<0:break
  i+=1
 assert len(rows)==64 and rows[-1]==(-1,-1) and all(rows[k][0]>rows[k+1][0] for k in range(62)) and rows[-2]==(39,1)
 return rows
def bank_set(r,sid):
 cnt=r(0x90fcb8+sid*8,1)[0];p=struct.unpack('<I',r(0x90fcbc+sid*8,4))[0];out=[]
 for k in range(cnt):
  slot=r(p+k*8,1)[0];n=struct.unpack('<I',r(p+k*8+4,4))[0];out.append((slot,r(n,32).split(b'\0')[0].decode()))
 return out
def stage(exe,game,dest):
 r=verified_reader(exe)
 assert r(0x7debbc,18)==b'B%02d_00_%02d.wav\0'
 assert r(0x42515c,5)==bytes.fromhex('b890009100') #mov eax,0x910090
 rows=table(r)
 slots=bank_set(r,0)+bank_set(r,1)
 assert slots==[(0,'COMMON_BANK00'),(1,'EMERALD_COAST_BANK01'),(2,'CHECK_SHEET_BANK02'),(4,'ENEMY_SET_BANK04'),(5,'EMERALD_COAST_BANK05'),(3,'P_SONICTAILS_BANK03')]
 dest=Path(dest)
 if dest.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 dest.mkdir(parents=True,exist_ok=True)
 src=Path(game)/'SoundData'/'SE'
 for _,name in slots:
  b=(src/f'{name}.dat').read_bytes();assert b[:15]==b'archive  V2.DMZ';(dest/f'{name}.dat').write_bytes(b)
 thresh=b''.join(struct.pack('<ii',t,b) for t,b in rows)
 slot_blob=b''.join(struct.pack('<B31s',s,n.encode()) for s,n in slots)
 meta={'source_exe_sha256':hashlib.sha256(Path(exe).read_bytes()).hexdigest(),'slots':slots,'scope':'id->bank thresholds and EC/Sonic bank slots; audio is the unchanged .dat banks'}
 pak=sa3dspak.build([('METADATA',json.dumps(meta).encode(),0),('SE_THRESH',thresh,0),('SE_SLOTS',slot_blob,0)]);sa3dspak.parse(pak)
 (dest/'se_map.sa3dspak').write_bytes(pak)
 return rows,slots
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('game',type=Path);ap.add_argument('dest',type=Path);a=ap.parse_args()
 rows,slots=stage(a.exe,a.game,a.dest)
 print(f'PC2004 SE staging: PASS {len(rows)-1} thresholds, name format, EC set0+Sonic set1 slots {[s for s,_ in slots]}; banks copied unchanged')
