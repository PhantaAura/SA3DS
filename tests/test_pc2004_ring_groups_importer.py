"""Source-neutral group packing, not a source execution or retail oracle."""
from pathlib import Path
import hashlib,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import build_pc2004_ring_groups as mod
from tools.importers.pc2004_set import read_set_source,SOURCE_ROW

def bank(act,params,kind=21):
 row=SOURCE_ROW.pack(kind,-16384,32767,5,100.,200.,300.,*params)
 return read_set_source(f'SET010{act}S.BIN',struct.pack('<I',1)+bytes(28)+row)
lut=b''.join(struct.pack('<f',i/1024) for i in range(1025))
# The packer only admits the exact-owner LUT in production. Patch its digest
# within this test to serialize a deliberately source-neutral fixture table.
mod.LUT_SHA=hashlib.sha256(lut).hexdigest()
sources=[bank(0,(3.,10.,0.),0x8015),bank(1,(4.01,20.,1.))]
b,counts,groups=mod.build_group_chunk(sources,360000.,lut)
assert counts==[4,4] and groups==2
magic,ver,flags,n,off,spec,trig,total=mod.HEADER.unpack_from(b)
assert magic==b'SA3DRGRP' and (ver,flags,n,off,spec,trig,total)==(1,0,2,32,112,144,4244)
for i,source in enumerate(sources):
 act,index,raw=mod.ROW.unpack_from(b,off+i*40)
 assert act==i and index==0 and raw==source.records[0].raw
assert b[trig:]==lut and mod.SPEC.unpack_from(b,spec)==(360000,360100,168100,7,1025,0,0,0)
for invalid in ([sources[0],sources[0]],[bank(0,(8.,10.,0.)),sources[1]],
                [bank(0,(3.,10.,2.)),sources[1]],
                [bank(0,(-1.,10.,0.)),sources[1]]):
 try:mod.build_group_chunk(invalid,360000,lut)
 except ValueError:pass
 else:raise AssertionError('unsupported source configuration accepted')
try:mod.build_group_chunk(sources,360000,lut[:-1])
except ValueError:pass
else:raise AssertionError('bad lookup table accepted')
print('Ring-group importer: PASS (raw source rows, selector bits, fractional counts, schema/LUT, limits; source-neutral)')
