#!/usr/bin/env python3
"""Exact-owner lighting selection/API-state/material conversion witnesses."""
from pathlib import Path
import hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
RANGES={
 (0x77ff90,48):'4173a1203a12cbea0b7b44ac8708b4478f39d837954e5f036aeab6cff8cec4b2',
 (0x402f40,208):'ef557ea95f23bf2f4fbca1703cadfbbbdf315952a6d419ab901fb5cf84024799',
 (0x4128a0,64):'35ff0e003569ab59837752ecdd4d4a61de30ad8fc122aa74acdcda8fd7a8da4b',
 (0x781df0,48):'b2d138b8890b8fb11cd5b64f2f7bf80e6b00e109131088f5965edb2de1b8e46f',
 (0x781cc0,304):'d594053afc90a286c1a7ece4de6f6c5631e19e60b630d01c4e8a3235445bcd09',
 (0x77ef70,32):'8f1b9fc1c924a1691c6200d9f9026ef592a77cca3f7fe13da83b7133253d3a06',
 (0x403470,64):'b03a513bc9181d683c5ebae5c994182cfdf442ef4fc89e09ec879b3733adcce9',
 (0x407020,32):'c39bfdf595fc9f68d31f7f840ec71ad27cef0d1c75faf22471a407b2de7b5411',
 (0x494400,368):'de5b551174e4e6580963db6b97f812b38d4334df1afe8f7b46c26c464d27256d',
 (0x4948c0,704):'8c11a9111fb1801f5caddae4d40702c7aa6787b65d2bb45bfcead962d4746428',
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
 print('Lighting owner witnesses: PASS exact SHA,17 spans,constant-white display/copy/resolve chain,EC ordinary profile and raw material API boundary; static state only')
if __name__=='__main__':verify(Path(sys.argv[1]))
