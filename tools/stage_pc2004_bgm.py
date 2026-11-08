#!/usr/bin/env python3
"""Copy the owner's Emerald Coast BGM (ecoast1-3.adx) to an SD asset folder.

Checks the exact PC2004 exe's music table (0x910298, 8-byte {name, loop}
entries; ids 28-30 = ecoast1-3, looping) and each ADX header (encoding 3,
loop byte offset consistent). Files are copied unchanged; nothing is written
inside the source tree."""
from pathlib import Path
import argparse,hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
def check_table(exe):
 r=verified_reader(exe)
 for i,name in ((28,b'ecoast1'),(29,b'ecoast2'),(30,b'ecoast3')):
  p,loop=struct.unpack('<II',r(0x910298+i*8,8));assert loop==1 and r(p,8).split(b'\0')[0]==name,(i,name)
 assert r(0x425421,7)==bytes.fromhex('8b0cc598029100') #425421: mov ecx,[0x910298+eax*8]
def check_adx(blob,name):
 assert blob[:2]==b'\x80\x00',name
 off=struct.unpack('>H',blob[2:4])[0]+4;assert blob[off-6:off]==b'(c)CRI'
 assert blob[4:8]==bytes([3,18,4,2]),name
 ls,lsb,le=struct.unpack('>III',blob[0x1c:0x28]);assert struct.unpack('>I',blob[0x18:0x1c])[0]==1 and lsb==off+(ls//32)*36,name
 return struct.unpack('>I',blob[12:16])[0],ls,le
def stage(exe,game,dest):
 check_table(exe)
 src=Path(game)/'SoundData'/'bgm'/'wma'
 if not src.is_dir():src=Path(game)/'system'/'sounddata'/'bgm'/'wma'
 dest=Path(dest)
 if dest.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 dest.mkdir(parents=True,exist_ok=True);rows=[]
 for k in (1,2,3):
  b=(src/f'ecoast{k}.adx').read_bytes();total,ls,le=check_adx(b,k)
  (dest/f'ecoast{k}.adx').write_bytes(b);rows.append((k,total,ls,le,hashlib.sha256(b).hexdigest()[:12]))
 return rows
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('game',type=Path);ap.add_argument('dest',type=Path);a=ap.parse_args()
 rows=stage(a.exe,a.game,a.dest)
 print('PC2004 BGM staging: PASS music table ids 28-30 ecoast1-3 looping; '+'; '.join(f'ecoast{k} {t} samples loop {s}..{e} sha {h}' for k,t,s,e,h in rows))
