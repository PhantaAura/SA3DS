#!/usr/bin/env python3
"""Exact-owner lighting selection/API-state/material conversion witnesses."""
from pathlib import Path
import hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
 (0x40a630,624):'2f04caa41b98fd49ce8560c792ea20fc1e6c0638e9adc6d33f85542c35a41527',
 (0x40a8a0,80):'f10094fdc104d244b4840723301eca6d53f719ef1a2374f59c4a35260baad6f7',
 (0x40a950,224):'ddd7d009f97bc65d3ea1a24d4c39a4cf6a78de2d053c445c800e42cbb6011f2c',
 (0x40aa30,128):'a6103f38c61571c595c0f98c7b9230562c5e649de7b96d4b9ff4898c58307700',
 (0x40aab0,176):'e1ccd4f6593f0e447ff7eb1f5fabd98a77ebfa47c6e6199836939e42ea3cc491',
 (0x412420,800):'a5be42dcb1c089d375958261b6481648fdd7f6508b491ab46beaaf79697727ce',
 (0x784850,656):'af1903eb31dc4894f5c26e94b1ad80ff355b1c78a6c564319c1063d45aab9726',
}
def verify(exe):
 r=verified_reader(exe)
 for (a,n),digest in RANGES.items():assert hashlib.sha256(r(a,n)).hexdigest()==digest,hex(a)
 assert r(0x903e88+3*96,4)==bytes((1,0,0,3))
 #Raw components reach FILD/FSTP, then Device8 SetMaterial(slot42).
 assert r(0x784969,6)==bytes.fromhex('ff91a8000000')
 print('Lighting owner witnesses: PASS exact SHA,7 spans,EC ordinary profile and raw material API boundary; static state only')
if __name__=='__main__':verify(Path(sys.argv[1]))
