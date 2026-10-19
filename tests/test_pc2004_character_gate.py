import struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from importers.pc2004_character_gate import *
base=0x10000000
b=bytearray(0x400)
def w32(va,v): struct.pack_into('<I',b,va-base,v)
root=base+0x40; child=base+0x100; sib=base+0x180; act=base+0x240
w32(root+OBJECT_MODEL,base+0x300); w32(root+OBJECT_CHILD,child); w32(root+OBJECT_SIBLING,sib)
w32(child+OBJECT_MODEL,base+0x304); w32(child+OBJECT_CHILD,0); w32(child+OBJECT_SIBLING,0)
w32(sib+OBJECT_MODEL,0); w32(sib+OBJECT_CHILD,0); w32(sib+OBJECT_SIBLING,0)
w32(act+ACTION_OBJECT,root); w32(act+ACTION_MOTION,base+0x350)
mem=PtrMemory(base,bytes(b))
rows=walk_object_graph(mem,root)
assert [x['object_va'] for x in rows]==[root,child,sib]
a=read_action(mem,act); assert a['object_va']==root and a['motion_va']==base+0x350
# cycle rejection
b2=bytearray(b); struct.pack_into('<I',b2,child+OBJECT_CHILD-base,root)
try: walk_object_graph(PtrMemory(base,bytes(b2)),root); raise AssertionError('cycle accepted')
except GateError: pass
print('PC2004 proven-offset character gate tests: PASS')
