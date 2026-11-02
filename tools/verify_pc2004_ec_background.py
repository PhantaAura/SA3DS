#!/usr/bin/env python3
"""Exact-owner static witnesses for original EC sky/horizon draw/configuration."""
from pathlib import Path
import hashlib,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_stage import PC2004StageDecoder
RANGES={
 (0x4F6BB0,0xAC):'c45d80fd89ab38b2964c63e25043bd3c6b690267c1127b3fea2c5baf16ecbf8d',
 (0x4F6DB0,0x3BF):'557b1d9aa2754c1a95cadea7d9eb293dd8f6050d3695b346774c4049d5abf8c2',
 (0x4F7230,0x1A):'3651162227392b1104ec695adc505a7dbfcdb1e7c3a9eab8237b49ea128f5090',
}
SIGNATURES={
 'background display callback':(0x4F723B,'c74014b06d4f00'),
 'camera task X/Z and zero Y':(0x4F6E2C,'a1b0cbb2038b50288b4020526a005033c9'),
 'background far subtract':(0x4F6C16,'d8258cf27d00'),
 'source subtract constant1000':(0x7DF28C,'00007a44'),
 'BG texture list':(0x4F6E42,'68d4b30301'),
 'original background roots':(0x4F6E5B,'6870b20301e8cb16f1ff687cb30301e8c116f1ff'),
 'fog disable before background':(0x4F6E20,'e81badf1ff'),
 'fog restore after background':(0x4F6E92,'e859acf1ff'),
}
def verify(path):
 dec=PC2004StageDecoder(Path(path));read=dec.pe.read_bytes
 for (va,size),digest in RANGES.items():
  if hashlib.sha256(read(va,size)).hexdigest()!=digest:raise ValueError(f'background source range mismatch {va:#x}')
 for name,(va,hx) in SIGNATURES.items():
  expected=bytes.fromhex(hx)
  if read(va,len(expected))!=expected:raise ValueError('background source witness mismatch: '+name)
 if dec.resolve_texture_archive(0x103B3D4)!='BG_BEACH':raise ValueError('BG texture archive mismatch')
 return dec
if __name__=='__main__':
 if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_ec_background.py OWNER_SONIC_EXE')
 verify(sys.argv[1]);print('PC2004 EC background: PASS (exact source callback/config ranges, cameraXZ/zeroY, two roots, BG archive, clip-1000 and fog brackets; static only)')
