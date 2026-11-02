#!/usr/bin/env python3
"""Compare external panel poses/C world matrices with independently read raw ACTION."""
from pathlib import Path
import math,struct,subprocess,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_pc2004_ec_route import verified_reader
from tools import sa3dspak

def verify(exe,package,checker):
    r=verified_reader(exe);root,motion=struct.unpack('<2I',r(0x8b8bc4,8))
    data,frames,typ,interp=struct.unpack('<IIHH',r(motion,12));assert (frames,typ,interp)==(8,3,2)
    nodes=[]
    def walk(va,parent):
        while va:
            raw=struct.unpack('<II3f3i3fII',r(va,52));index=len(nodes);nodes.append((raw,parent))
            if raw[-2]:walk(raw[-2],index)
            va=raw[-1]
    walk(root,-1);assert len(nodes)==3
    p=sa3dspak.parse(Path(package).read_bytes());config=next(x['data'] for x in p['chunks'] if x['type']=='PANEL')
    rows=subprocess.check_output([str(checker),str(package)],text=True).splitlines();assert len(rows)==24
    actual={(int(x[0]),int(x[1])):list(map(float,x[2:])) for x in map(str.split,rows)}
    def mul(a,b):return [[sum(a[i][k]*b[k][j] for k in range(4)) for j in range(4)] for i in range(4)]
    def ident():return [[float(i==j) for j in range(4)] for i in range(4)]
    error=0.;checks=0
    for frame in range(frames):
        worlds=[]
        for i,(node,parent) in enumerate(nodes):
            pos=list(node[2:5]);angles=list(node[5:8]);scale=list(node[8:11]);flags=node[0]
            posptr,angptr,npos,nang=struct.unpack('<4I',r(data+i*16,16));assert not npos
            if nang:
                keys=list(struct.iter_unpack('<Iiii',r(angptr,nang*16)));assert [x[0] for x in keys]==[0,7]
                # Integer display frames: preserve raw int32 angles, no s16 shortest arc.
                angles=[math.trunc(keys[0][a]+(keys[1][a]-keys[0][a])*frame/7.) for a in (1,2,3)]
            saved=struct.unpack_from('<3f3i3f',config,80+(frame*3+i)*36)
            assert list(saved[:3])==pos and list(saved[3:6])==angles and list(saved[6:])==scale
            assert struct.unpack_from('<HHI',config,32+i*16)==(65535 if parent<0 else parent,i,flags)
            m=ident()
            for a in range(3):m[a][3]=pos[a]
            for axis in (2,1,0):
                angle=angles[axis]*math.tau/65536.;c=math.cos(angle);s=math.sin(angle);rot=ident()
                if axis==0:rot[1][1]=rot[2][2]=c;rot[1][2]=-s;rot[2][1]=s
                if axis==1:rot[0][0]=rot[2][2]=c;rot[0][2]=s;rot[2][0]=-s
                if axis==2:rot[0][0]=rot[1][1]=c;rot[0][1]=-s;rot[1][0]=s
                m=mul(m,rot)
            for row in range(4):
                for col in range(3):m[row][col]*=scale[col]
            if parent>=0:m=mul(worlds[parent],m)
            worlds.append(m);expected=[m[row][col] for col in range(4) for row in range(4)]
            for a,b in zip(actual[frame,i],expected):error=max(error,abs(a-b));checks+=1
    assert error<4e-5,error
    print(f'Original panel ACTION oracle: PASS 24 poses/{checks} matrix components, max_error={error:.9g}; no retail/raster parity')
if __name__=='__main__':
    if len(sys.argv)!=4:raise SystemExit('usage: verify_pc2004_panel_visual.py OWNER_SONIC_EXE PANEL_PACKAGE C_CHECKER')
    verify(Path(sys.argv[1]),Path(sys.argv[2]),Path(sys.argv[3]))
