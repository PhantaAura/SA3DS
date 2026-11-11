"""Source-neutral packing/bounds fixtures; never a retail execution oracle."""
from pathlib import Path
import dataclasses
import hashlib
import struct
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3dspak
from tools import build_pc2004_ec_movement_objects as mod
from tools.importers.pc2004_set import read_set_source,SOURCE_ROW

def bank(act,variant='S'):
    rows=[SOURCE_ROW.pack(k,-16384,32767,-3,100.,200.,300.,93.15,2.25,13.) for k in (0,0x8002,3,1,40,18,26)]
    return read_set_source(f'SET010{act}{variant}.BIN',struct.pack('<I',len(rows))+bytes(28)+b''.join(rows))
sources=[bank(0),bank(1)]
b,counts=mod.build_chunk(sources,legacy=True)
extended, extended_counts=mod.build_chunk(sources)
assert extended_counts==[{1:1,2:1,3:1,18:1,26:1,40:1}]*2
assert counts==[{1:1,2:1,3:1}]*2
assert mod.HEADER.unpack_from(b)==(b'SA3DMOVE',1,0,6,32,272,3,320)
for i in range(6):
    act,index,raw=mod.ROW.unpack_from(b,32+i*40)
    assert (act,index)==(i//3,i%3+1) and raw==sources[act].records[index].raw
assert b==mod.build_chunk(list(reversed(sources)),legacy=True)[0]
for invalid in ([sources[0],sources[0]],[bank(0,'H'),sources[1]],
                [dataclasses.replace(sources[0],sha256='0'*64),sources[1]],
                [dataclasses.replace(sources[0],records=sources[0].records*300),sources[1]]):
    try:mod.build_chunk(invalid)
    except ValueError:pass
    else:raise AssertionError('invalid source accepted')
if len(sys.argv)>1:
    path=Path(sys.argv[1])
    def save(suffix,data):path.with_name(path.name+suffix).write_bytes(sa3dspak.build([('MOVE_OBJECTS',bytes(data),0)]))
    save('',b)
    save('.extended',extended)
    for name,offset,fmt,value in [('count',12,'<I',2047),('act',32,'<I',2),
        ('slot',36,'<I',1023),('type',40,'<H',4),('nan',48,'<f',float('nan')),
        ('param',60,'<f',float('inf')),('spec',276,'<f',1.),('duplicate',76,'<I',1)]:
        copy=bytearray(b);struct.pack_into(fmt,copy,offset,value);save('.'+name,copy)
    save('.truncated',b[:-1])
    path.with_name(path.name+'.twice').write_bytes(sa3dspak.build([('MOVE_OBJECTS',b,0),('MOVE_OBJECTS',b,0)]))
print('Movement object importer: PASS (source-neutral row identity, class/high bits, act/slot/order, integrity, bounded schema)')
