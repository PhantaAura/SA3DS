"""Source-neutral route metadata and CRC-valid malformed variants."""
from pathlib import Path
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
from tools.build_pc2004_emerald_coast_route import HEADER,START,EDGE
starts=[(0,0,10,20,30,0x6000),(1,0,-10,100,-20,0x4000)]
edges=[(0,1,500,30,-60,22)]
so=32;eo=so+len(starts)*24
body=bytearray(eo+len(edges)*24)
HEADER.pack_into(body,0,b'SA3DECRO',1,0,1,2,1,so,eo)
for i,row in enumerate(starts):START.pack_into(body,so+i*24,*row)
for i,row in enumerate(edges):EDGE.pack_into(body,eo+i*24,*row)
path=Path(sys.argv[1])
def write(suffix,data):path.with_name(path.name+suffix).write_bytes(sa3dspak.build([('ROUTE',bytes(data),0)]))
write('',body)
for name,off,fmt,value in [('version',8,'H',2),('flags',10,'H',1),('level',12,'I',3),
 ('count',16,'I',99),('overlap',28,'I',32),('duplicate',so+24,'I',0),
 ('character',so+4,'I',7),('nan',so+8,'f',float('nan')),('yaw',so+20,'I',65536),
 ('from',eo,'I',7),('to',eo+4,'I',7),('self',eo+4,'I',0),
 ('radius',eo+20,'f',-1),('infinite',eo+20,'f',float('inf'))]:
 bad=bytearray(body);struct.pack_into('<'+fmt,bad,off,value);write('.'+name,bad)
