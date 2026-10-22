from pathlib import Path
import struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
from tools.build_pc2004_emerald_coast_collision import HEADER,ACT,MODEL,INSTANCE,align16
# Two floor triangles, two slope triangles and two wall triangles; source-neutral.
vertices=[(-32,0,0),(-32,0,64),(32,0,0),(32,0,64),
          (-32,0,64),(-32,6.4,128),(32,0,64),(32,6.4,128),
          (32,-4,0),(32,20,0),(32,-4,64),(32,20,64),
          (-32,-4,64),(-32,20,64)]
indices=[0,1,2,2,1,3,4,5,6,6,5,7,8,9,10,10,9,11,
         10,11,12,12,11,13]
ao=64;mo=align16(ao+3*ACT.size);io=align16(mo+MODEL.size)
vo=align16(io+INSTANCE.size);xo=align16(vo+len(vertices)*12);b=bytearray(xo+len(indices)*2)
HEADER.pack_into(b,0,b'SA3DECOL',1,0,3,1,1,len(vertices),len(indices),ao,mo,io,vo,xo)
ACT.pack_into(b,ao,0,0,1,0,500,0,0,0)
ACT.pack_into(b,ao+32,1,1,0,0,500,0,0,0)
ACT.pack_into(b,ao+64,2,1,0,0,500,0,0,0)
MODEL.pack_into(b,mo,0,len(vertices),0,len(indices),0,0,64,100)
INSTANCE.pack_into(b,io,0,1,0,0,0,0,64,100,0,0,0,0,0,0,1,1,1,0,0,0)
for i,v in enumerate(vertices):struct.pack_into('<fff',b,vo+i*12,*v)
for i,x in enumerate(indices):struct.pack_into('<H',b,xo+i*2,x)
Path(sys.argv[1]).write_bytes(sa3dspak.build([('METADATA',b'{}',0),('COLLISION',bytes(b),0)]))
for name,offset,fmt,value in [
    ('version',8,'H',2),('model',io,'I',9),('index',xo,'H',65535),
    ('nan',vo,'f',float('nan')),('overlap',44,'I',64),
]:
    invalid=bytearray(b);struct.pack_into('<'+fmt,invalid,offset,value)
    # Rebuild valid package CRCs: rejection must be structural, not just CRC.
    Path(sys.argv[1]+'.'+name).write_bytes(sa3dspak.build([('COLLISION',bytes(invalid),0)]))
