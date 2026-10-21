#!/usr/bin/env python3
"""Create a tiny source-neutral package for the native reader regression."""
from pathlib import Path
import struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak,sa3t

def a16(n): return (n+15)&~15
vh=struct.Struct('<8sHHIIIIIIIIII'); model=struct.Struct('<IIIIIIffffiI')
mesh=struct.Struct('<HHIIHHII'); inst=struct.Struct('<IIIIIIIIfffffffffffff')
vertex=struct.Struct('<ffffffff')
mo=a16(vh.size); meo=a16(mo+model.size); io=a16(meo+mesh.size)
vo=a16(io+inst.size); xo=a16(vo+3*vertex.size); size=xo+6
g=bytearray(size)
vh.pack_into(g,0,b'SA3DVIS\0',1,0,1,1,1,3,3,mo,meo,io,vo,xo)
model.pack_into(g,mo,0,1,0,3,0,3,0,0,0,2,-1,0)
mesh.pack_into(g,meo,0,0,0,3,0xC000,0,1,1)
inst.pack_into(g,io,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,2)
vertex.pack_into(g,vo,0,0,0,0,1,0,0,0)
vertex.pack_into(g,vo+vertex.size,1,0,0,0,1,0,1,0)
vertex.pack_into(g,vo+2*vertex.size,0,1,0,0,1,0,0,1)
struct.pack_into('<HHH',g,xo,0,1,2)
mh=struct.Struct('<8sHHIIII'); mr=struct.Struct('<IIfII'); ro=a16(mh.size)
m=bytearray(ro+mr.size);mh.pack_into(m,0,b'SA3DMATL',1,0,1,mr.size,ro,len(m));mr.pack_into(m,ro,0xffffffff,0,1.0,0,0)
t=sa3t.build([{'width':8,'height':8,'rgba':bytes([255,255,255,255])*64,'source_pixel_format':0,'source_data_format':9,'global_index':0}],'00'*32)
p=sa3dspak.build([('METADATA',b'{"act":0}\n',0),('GEOMETRY',bytes(g),0),('MATERIALS',bytes(m),0),('TEXTURES',t,0)])
Path(sys.argv[1]).write_bytes(p)
