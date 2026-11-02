#!/usr/bin/env python3
"""Verify original PC2004 PVR9 linear-copy dispatch, not a rendered-game capture."""
from pathlib import Path
import hashlib,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_stage import PC2004StageDecoder
WITNESSES={
 'format900 has no mip converter':(0x389D4B8,'0009000000000000'),
 'format900 at copy-dispatch index1':(0x389D57C,'00090000'),
 'format900 points to contiguous copy':(0x38A63B0,'70c57800'),
 'dispatch format-code search':(0x78C92B,'b978d589033929740b4083c10483f80975f333c0'),
 'normal pixel format callback table':(0x78C949,'8b1c85ac638a03'),
 'width times height times pixel bytes':(0x78C570,'8b4c240c0faf4c24100faf4c2414'),
 'copy full dwords then remaining bytes':(0x78C58A,'c1e902f3a58bc883e103f3a4'),
}
def verify(path):
 d=PC2004StageDecoder(Path(path));read=d.pe.read_bytes
 for label,(va,hx) in WITNESSES.items():
  b=bytes.fromhex(hx)
  if read(va,len(b))!=b:raise ValueError('format9 source mismatch: '+label)
 if hashlib.sha256(read(0x78C570,0x29)).hexdigest()!='1285fb44ae9da658903ca35293ada9bba213af59cccc66c21c1158df64839fbd':
  raise ValueError('format9 contiguous-copy function mismatch')
 return d
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_pvm_layout.py OWNER_SONIC_EXE')
 verify(sys.argv[1]);print('PC2004 PVM9: PASS (exact original0x900 dispatch to contiguous width*height*pixel-size copy; no Morton path)')
