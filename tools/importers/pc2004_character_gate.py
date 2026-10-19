#!/usr/bin/env python3
from __future__ import annotations
from dataclasses import dataclass
import struct

OBJECT_MODEL=0x04
OBJECT_CHILD=0x2C
OBJECT_SIBLING=0x30
OBJECT_MIN_SIZE=0x34
ACTION_OBJECT=0x00
ACTION_MOTION=0x04
ACTION_MIN_SIZE=0x08
SONIC_MODELS_COUNT=11
SONIC_OBJECTS_COUNT=79
SONIC_ACTIONS_COUNT=149

class GateError(Exception): pass

@dataclass
class PtrMemory:
    base_va:int
    data:bytes
    def u32(self,va):
        off=va-self.base_va
        if off<0 or off+4>len(self.data): raise GateError(f'pointer read OOB {va:#x}')
        return struct.unpack_from('<I',self.data,off)[0]

def walk_object_graph(mem:PtrMemory, root_va:int, max_nodes=4096):
    """Traverse only fields proven by real code: model +0x04, child +0x2C, sibling +0x30."""
    order=[]; seen=set(); active=set()
    def rec(obj):
        if obj==0: return
        if obj in active: raise GateError(f'cycle at {obj:#x}')
        if obj in seen: return
        if len(seen)>=max_nodes: raise GateError('node cap')
        active.add(obj); seen.add(obj)
        model=mem.u32(obj+OBJECT_MODEL)
        child=mem.u32(obj+OBJECT_CHILD)
        sibling=mem.u32(obj+OBJECT_SIBLING)
        order.append({'object_va':obj,'model_va':model,'child_va':child,'sibling_va':sibling})
        rec(child)
        active.remove(obj)
        rec(sibling)
    rec(root_va); return order

def read_action(mem:PtrMemory, action_va:int):
    return {'object_va':mem.u32(action_va+ACTION_OBJECT),'motion_va':mem.u32(action_va+ACTION_MOTION)}
