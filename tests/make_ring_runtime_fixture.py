from pathlib import Path
import struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
header=struct.Struct('<8sHHIIIII');row=struct.Struct('<IIHhhhffffff');spec=struct.Struct('<ffffffII')
rows=[(0,0,0x8000,0,100,0,0.,0.,0.,-1.,0.,0.),
      (0,1,0x1000,0,0,0,100.,0.,0.,0.,0.,0.),
      (0,2,0x2000,0,0,0,200.,0.,0.,0.,0.,0.),
      (1,3,0,0,0,0,0.,0.,0.,0.,0.,0.)]
b=bytearray(header.pack(b'SA3DRING',1,0,2,len(rows),32,32+40*len(rows),64+40*len(rows)))
for r in rows:b.extend(row.pack(*r))
b.extend(spec.pack(0,0,0,5,360000,360100,364,1))
p=Path(sys.argv[1]);p.write_bytes(sa3dspak.build([('RINGS',b,0)]))
for name,offset,fmt,value in [('count',16,'I',2047),('act',32,'I',2),('index',36,'I',1023),('type',40,'H',1),('nan',48,'f',float('nan')),('radius',len(b)-20,'f',-1),('clip',len(b)-4,'I',3),('duplicate',32+40+4,'I',0)]:
 bad=bytearray(b);struct.pack_into('<'+fmt,bad,offset,value)
 Path(str(p)+'.'+name).write_bytes(sa3dspak.build([('RINGS',bad,0)]))

capacity=bytearray(header.pack(b'SA3DRING',1,0,2,130,32,32+40*130,64+40*130))
for i in range(130):capacity.extend(row.pack(0,i,0,0,0,0,0,0,0,0,0,0))
capacity.extend(spec.pack(0,0,0,5,360000,360100,364,1))
Path(str(p)+'.capacity').write_bytes(sa3dspak.build([('RINGS',capacity,0)]))
