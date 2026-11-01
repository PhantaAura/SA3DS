"""Source-neutral multi-act translated geometry for bounded-residency tests."""
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
from tools.build_pc2004_emerald_coast_collision import HEADER,ACT,MODEL,INSTANCE,align16

vertices=[(-32,0,0),(-32,0,64),(32,0,0),(32,0,64),
          (-32,0,64),(-32,6.4,128),(32,0,64),(32,6.4,128),
          (32,-4,0),(32,20,0),(32,-4,64),(32,20,64),
          (-32,-4,64),(-32,20,64)]
indices=[0,1,2,2,1,3,4,5,6,6,5,7,8,9,10,10,9,11,
         10,11,12,12,11,13]
# Act1 deliberately contains both a distant copy and an origin copy at y32.
# Correct region selection must distinguish act identity from grid location.
instances=[(0,0,0),(8192,0,-16384),(0,32,0),(-32768,-10,24576)]
ao=64;mo=align16(ao+3*ACT.size);io=align16(mo+MODEL.size)
vo=align16(io+len(instances)*INSTANCE.size);xo=align16(vo+len(vertices)*12)
b=bytearray(xo+len(indices)*2)
HEADER.pack_into(b,0,b'SA3DECOL',1,0,3,1,len(instances),len(vertices),len(indices),ao,mo,io,vo,xo)
for i,(first,count) in enumerate([(0,1),(1,2),(3,1)]):
    ACT.pack_into(b,ao+i*ACT.size,i,first,count,0,500,0,0,0)
MODEL.pack_into(b,mo,0,len(vertices),0,len(indices),0,0,64,100)
for i,(x,y,z) in enumerate(instances):
    INSTANCE.pack_into(b,io+i*INSTANCE.size,0,1,0,0,x,y,z+64,100,x,y,z,0,0,0,1,1,1,0,0,0)
for i,v in enumerate(vertices):struct.pack_into('<fff',b,vo+i*12,*v)
for i,index in enumerate(indices):struct.pack_into('<H',b,xo+i*2,index)
Path(sys.argv[1]).write_bytes(sa3dspak.build([('COLLISION',bytes(b),0)]))
