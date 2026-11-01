from pathlib import Path
import math,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
path=Path(sys.argv[1]);original=sa3dspak.parse(path.read_bytes())
rows=[(0,10,21,0,0,0,300.,0.,0.,4.,10.,0.),
      (1,11,21,16384,16384,16384,1000.,100.,2000.,4.01,20.,1.)]
row=struct.Struct('<IIHhhhffffff');header=struct.Struct('<8sHHIIIII');spec=struct.Struct('<fffIIIII')
so=32+len(rows)*40;to=so+32
b=bytearray(header.pack(b'SA3DRGRP',1,0,len(rows),32,so,to,to+4100))
for value in rows:b.extend(row.pack(*value))
b.extend(spec.pack(360000,360100,168100,7,1025,0,0,0))
# Deliberately linear fixture LUT makes interpolation/quadrant checks exact.
# This is source-neutral and is NOT the owner's Ninja lookup table.
for i in range(1025):b.extend(struct.pack('<f',i/1024))
chunks=[(c['type'],c['data'],c['flags']) for c in original['chunks']]
def write(name,data):Path(str(path)+'.groups'+name).write_bytes(sa3dspak.build(chunks+[('RING_GROUPS',data,0)]))
write('',b)
for name,offset,fmt,value in [('count',12,'I',2047),('class',32+8,'H',0),('mode',32+36,'f',2),('children',32+28,'f',8),('collision',32+4,'I',0),('range',so+8,'f',float('nan')),('nan',to+4,'f',float('nan')),('endpoint',to+4096,'f',.99)]:
 bad=bytearray(b);struct.pack_into('<'+fmt,bad,offset,value);write('.'+name,bad)
