#!/usr/bin/env python3
"""Owner-only raw geometry/instance scale audit; emits no game payloads.

Does not call the stage/character converters. Reuses the independent Pass11B
raw PE reader, and compares original float triples directly against packages.
This proves coordinate preservation, not retail rendering/contact parity.
"""
from pathlib import Path
import argparse, hashlib, math, struct, sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.audit_pass11b_source_pose import RawPE, source_actions, hierarchy, parse_sa3c
from tools.verify_pc2004_collision_work import parse_pe, va_to_off, PROFILE_SHA256
from tools import sa3dspak

def u32(b,o=0):return struct.unpack_from('<I',b,o)[0]
def vector(b,o):return struct.unpack_from('<fff',b,o)

def verify_vertices(read,models,package,model_offset,model_stride,vertex_offset,vertex_stride,first_field):
    checked=0
    for index,va in models.items():
        raw=read(va,44);points=u32(raw);count=u32(raw,8)
        stock=read(points,count*12)
        triples={stock[i*12:(i+1)*12] for i in range(count)}
        row=model_offset+index*model_stride
        first,n=struct.unpack_from('<II',package,row+first_field)
        for vi in range(first,first+n):
            p=package[vertex_offset+vi*vertex_stride:vertex_offset+vi*vertex_stride+12]
            if p not in triples:raise ValueError(f'rescaled/non-source vertex in model {index}')
            checked+=1
    return checked

def audit(exe,chrmodels,sonic,stage):
    image=exe.read_bytes();assert hashlib.sha256(image).hexdigest()==PROFILE_SHA256
    base,sections=parse_pe(image)
    def read(va,n):
        off,_=va_to_off(va,base,sections);b=image[off:off+n];assert len(b)==n;return b
    # Actual draw-list filter, not an inferred flag name: MOV flags; TEST; JNS
    # skips list append and reaches the next COL record's 0x24-byte stride.
    assert read(0x43aa5b,7)==bytes.fromhex('8b472085c0795a')
    skip=0x43aa5b+7+struct.unpack('b',read(0x43aa61,1))[0]
    assert read(skip,3)==bytes.fromhex('83c724')
    assert bytes.fromhex('893c8518d5b203') in read(0x43aa62,skip-0x43aa62)
    print('PC2004 land draw-list signed visibility filter: PASS (0x43AA5B -> loop stride)')
    pe=RawPE(chrmodels);actions=source_actions(pe);objects={}
    for action in actions:
        if action:
            for obj in hierarchy(pe,action[0]):objects[obj['va']]=obj
    data,ss=parse_sa3c(sonic);models={}
    assert ss[b'NODE'][0]==len(objects)
    for i,va in enumerate(sorted(objects)):
        row=ss[b'NODE'][1]+i*ss[b'NODE'][2];stock=pe.read_va(va,52)
        assert data[row+12:row+24]==stock[8:20]
        assert data[row+36:row+48]==stock[32:44]
        mid=struct.unpack_from('<H',data,row)[0]
        if mid!=65535:
            source=u32(stock,4);assert models.setdefault(mid,source)==source
    n=verify_vertices(pe.read_va,models,data,ss[b'MODL'][1],ss[b'MODL'][2],ss[b'VRTX'][1],ss[b'VRTX'][2],8)
    print(f'Sonic original NODE position/scale + {n} converted vertex positions: EXACT FLOAT-BYTE MATCH')
    pak=sa3dspak.parse(stage.read_bytes());g=next(c['data'] for c in pak['chunks'] if c['type']=='GEOMETRY')
    nm,_,ni,nv=struct.unpack_from('<IIII',g,12)
    mo,_,io,vo=struct.unpack_from('<IIII',g,32)
    land=u32(read(0x97da08+8*4,4));stock_count=struct.unpack('<h',read(land,2))[0]
    rows=u32(read(land+12,4));models={};visible=0;selected=0;identity=0
    for i in range(ni):
        row=io+i*84;mid,source_index,flags,land_flags=struct.unpack_from('<IIII',g,row)
        assert source_index<stock_count
        col=read(rows+source_index*36,36);obj=read(u32(col,24),52)
        assert flags==u32(obj) and land_flags==u32(col,32)
        assert g[row+32:row+44]==obj[8:20] and g[row+56:row+68]==obj[32:44]
        angles=struct.unpack_from('<iii',obj,20);rot=vector(g,row+44)
        assert all(abs(a*math.tau/65536-r)<2e-6 for a,r in zip(angles,rot))
        assert models.setdefault(mid,u32(obj,4))==u32(obj,4)
        assert g[row+68:row+84]==col[:16] # original center + radius
        if land_flags&0x80000000:
            visible+=1
            if math.hypot(*[vector(g,row+68)[a] for a in (0,2)])-struct.unpack_from('<f',g,row+80)[0]<=500:selected+=1
        if vector(obj,8)==(0,0,0) and angles==(0,0,0) and vector(obj,32)==(1,1,1):identity+=1
    assert len(models)==nm
    checked=verify_vertices(read,models,g,mo,48,vo,32,8)
    assert checked==nv
    print(f'Emerald Coast act 0: {ni} instance transforms/centers/radii EXACT; {checked}/{nv} vertex positions EXACT')
    print(f'Visible instances={visible}; visible opening residency={selected}; identity object transforms={identity}/{ni}')
    print('Map/character units preserved at 1:1; no claim of full-map residency, retail camera, or contact-body parity.')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('exe',type=Path);p.add_argument('chrmodels',type=Path);p.add_argument('sonic',type=Path);p.add_argument('stage',type=Path)
    a=p.parse_args();audit(a.exe,a.chrmodels,a.sonic,a.stage)
