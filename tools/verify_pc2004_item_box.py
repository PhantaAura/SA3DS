#!/usr/bin/env python3
"""Exact-owner witnesses for the O ITEMBOX rules ported in src/game/item_box.c."""
from pathlib import Path
import argparse,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
def verify(exe):
 r=verified_reader(exe)
 _,_,_,_,_,fn,name=struct.unpack('<BBHfIII',r(0x1036420+13*20,20));assert fn==0x4d6f10 and r(name,9)==b'O ITEMBOX'
 handlers=[struct.unpack('<I',r(0x9bf194+k*8,4))[0] for k in range(9)]
 assert handlers==[0x4d6bf0,0x4d6d80,0x4d6c50,0x4d6c90,0x4d6cd0,0x4d6dc0,0x4d6d40,0x4d6e00,0x4d6e40]
 assert r(0x4d6c50,2)==bytes.fromhex('6a05') and struct.unpack('<i',r(0x4d6c53,4))[0]+0x4d6c57==0x425be0 #push5; call AddRings
 assert r(0x4d6c90,2)==bytes.fromhex('6a0a')           #push10
 assert r(0x4d6d40,2)==bytes.fromhex('6a01') and struct.unpack('<i',r(0x4d6d43,4))[0]+0x4d6d47==0x425b60 #AddLives(1)
 assert r(0x4d6c7d,2)==bytes.fromhex('6a0b') and r(0x4d6c6a,5)==bytes.fromhex('681a050000') #sound 0xB, 0x51A
 assert struct.unpack('<6h',r(0x7e6fe4,12))==(1,5,10,15,20,40)
 assert struct.unpack('<ff',r(0x7dd53c,4)+r(0x7e0b38,4))==(1/32768,-6.0)
 assert r(0x6443c4,6)==bytes.fromhex('69c0fd430300') #MSVC rand LCG 0x343FD
 print('O ITEMBOX owner witnesses: PASS list entry13, handler table, 5/10 rings, AddLives(1), sounds 0xB/0x51A, random table 1/5/10/15/20/40, 1/32768*-6 index, MSVC rand')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);verify(ap.parse_args().exe)
