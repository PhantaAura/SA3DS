#!/usr/bin/env python3
"""Exact-owner player position history and Sonic-tail scheduling witnesses."""
from pathlib import Path
import argparse,hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
SPANS={
 (0x43b920,64):'78cbd5ed1eefa7d6b30e0fc1c492da38375da28338786568f934725397e2bad5',
 (0x43b960,93):'e68e6f52e647517a5858e9bd611515adcf6bc3f98a83110df0d2fc7ee9af24cc',
 (0x4419c0,127):'92a02e007a6c84677170f53b29f706538a2c3730ef5cce9f25a461448e57dfd6',
 (0x441a40,127):'c98f1963a150fd2e8ae6f17d84b7edf35a544d6a31bd590e038f32e8caf8dff3',
 (0x446f10,96):'6fb8717cb3270c5683b7449e17b8c906e80a794e18d805c14b6f37f1dc8e2da1',
 (0x49befa,194):'8400bb0e4bb68a26aa7a26ef0f5dd2a813b33cb389942dd90ba2c551fb4d78fa',
}
def verify(exe):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 for site,target in ((0x49bf04,0x4948c0),(0x49bf7b,0x446f10),(0x49bfad,0x446f10),(0x446f61,0x43b960)):
  assert r(site,1)==b'\xe8' and site+5+struct.unpack('<i',r(site+1,4))[0]==target,hex(site)
 assert r(0x43b970,2)==bytes.fromhex('fec2') #increment byte BEFORE storing position
 assert r(0x4419cf,2)==bytes.fromhex('2ad1') #byte cursor-offset
 print('Player history owner witnesses: PASS exact SHA,6 spans,Sonic display/tail/history call order,byte increment/subtraction and preserved history across player-table clear; position-only port,not retail runtime')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);a=ap.parse_args();verify(a.exe)
