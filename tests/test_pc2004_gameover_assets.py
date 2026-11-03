#!/usr/bin/env python3
"""Owner-only pixel authority regression; no pixels are saved in source tree."""
from pathlib import Path
import struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_pvm_archive import PVMArchive,PVMArchiveError
from tools.build_pc2004_gameover_assets import build
from tools import sa3dspak,sa3t
exe,pvm=map(Path,sys.argv[1:])
try:PVMArchive(pvm)
except PVMArchiveError as e:assert 'directory/PVRT mismatch' in str(e)
else:raise AssertionError('English directory mismatch was not rejected by default')
a=PVMArchive(pvm,pixel_format_from_pvrt=True)
assert a.texture_count==7 and [x.pixel_format for x in a.entries]==[0,0,2,2,2,2,2]
for e in a.entries:
 actual=a.decode_base_rgba(e.index)
 #Independent direct bitfield oracle from PVRT bytes, including all alpha4 levels.
 raw=a.data[e.pvrt_offset+16:e.pvrt_offset+16+e.width*e.height*2]
 expected=bytearray()
 for (v,) in struct.iter_unpack('<H',raw):
  if e.index<2:expected.extend((((v>>10)&31)*255//31,((v>>5)&31)*255//31,(v&31)*255//31,255 if v&32768 else 0))
  else:expected.extend((((v>>8)&15)*17,((v>>4)&15)*17,(v&15)*17,((v>>12)&15)*17))
 assert actual==expected
 assert len(actual)==e.width*e.height*4
#Explicit pixel authority still rejects bad sizes and unsupported encoding.
for offset,value in [(a.entries[2].pvrt_offset+12,130),(a.entries[2].pvrt_offset+8,3)]:
 b=bytearray(a.data)
 if value==130:struct.pack_into('<H',b,offset,value)
 else:b[offset]=value
 try:PVMArchive(b,pixel_format_from_pvrt=True)
 except PVMArchiveError:pass
 else:raise AssertionError('malformed payload accepted')
chunks=sa3dspak.parse(build(exe,pvm));assert len(chunks['chunks'])==2
print('Game-over owner assets: PASS explicit PVRT pixel authority,strict default,all seven raw-pixel oracles and malformed size/encoding rejection; native display separate')
