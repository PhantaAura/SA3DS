#!/usr/bin/env python3
"""Copy the owner's voice clips (SoundData/VOICE_US or VOICE_JP /WMA/NNNN.adx,
the Steam install's ADX form of the 2004 %04d.wma voices) to an SD folder.

Checks the exact exe: 425500/425575 format "%sSoundData\\VOICE_JP|US\\WMA\\%04d.wma"
selected by 03B0EF38, and 425135 routing PlaySound ids >0x568 to voice
id+0x159. Files are copied unchanged."""
from pathlib import Path
import argparse,shutil,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
def stage(exe,game,dest,language='US'):
 r=verified_reader(exe)
 assert r(0x7df14c,33)==b'%sSoundData\\VOICE_JP\\WMA\\%04d.wma'
 assert r(0x7df128,33)==b'%sSoundData\\VOICE_US\\WMA\\%04d.wma'
 assert r(0x42512d,6)==bytes.fromhex('81fd68050000') and r(0x425135,6)==bytes.fromhex('81c559010000') #cmp ebp,0x568 / add ebp,0x159
 src=Path(game)/'SoundData'/f'VOICE_{language}'/'WMA';dest=Path(dest)
 if dest.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 dest.mkdir(parents=True,exist_ok=True);n=0
 for p in sorted(src.glob('*.adx')):
  if not p.stem.isdigit():continue
  b=p.read_bytes();assert b[:2]==b'\x80\x00',p.name
  out=dest/f'{int(p.stem):04d}.adx'
  if not out.exists() or out.stat().st_size!=len(b):out.write_bytes(b)
  n+=1
 return n
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('game',type=Path);ap.add_argument('dest',type=Path)
 ap.add_argument('--language',choices=('US','JP'),default='US');a=ap.parse_args()
 print(f'PC2004 voice staging: PASS path formats + 0x568/+0x159 routing; {stage(a.exe,a.game,a.dest,a.language)} {a.language} voice clips copied unchanged')
