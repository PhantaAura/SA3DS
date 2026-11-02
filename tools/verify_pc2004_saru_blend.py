#!/usr/bin/env python3
"""Raw-owner equation check for E SARU two-motion pose blending, not retail traces."""
from pathlib import Path
import math,struct,subprocess,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
from tools.audit_pass11b_source_pose import hierarchy,f32,BAMS_TO_RAD

def verify(exe,package,checker):
    read=verified_reader(exe)
    class Raw:read_va=staticmethod(read)
    pe=Raw();actions=[struct.unpack('<II',read(a,8)) for a in (0x96a89c,0x9669ec,0x967964,0x968adc,0x969924,0x96b444)]
    nodes=hierarchy(pe,actions[0][0]);order=sorted(n['va'] for n in nodes);idx={va:i for i,va in enumerate(order)}
    def weighted(a,b,t):return a*(1.-t)+b*t
    def sample(action,frame):
        root,motion=actions[action];data,frames,typ,interp=struct.unpack('<IIHH',read(motion,12));assert(typ,interp)==(3,2)
        frame=math.fmod(f32(frame),frames)
        out={idx[n['va']]:list(n['position'])+list(n['rotation'])+list(n['scale']) for n in nodes}
        for i,node in enumerate(nodes):
            assert not node['flags']&0x60
            pos,ang,npos,nang=struct.unpack('<4I',read(data+i*16,16))
            for pointer,count,offset,isangle in ((pos,npos,0,False),(ang,nang,3,True)):
                if not count:continue
                keys=list(struct.iter_unpack('<Iiii' if isangle else '<Ifff',read(pointer,count*16)))
                previous=0
                for j in range(1,count):
                    if keys[j][0]>math.trunc(frame):break
                    previous=j
                a,b=keys[previous],keys[(previous+1)%count];delta=b[0]-a[0]
                t=(frame-a[0])/(f32(delta) if delta>0 else 1.)
                for axis in range(3):
                    v=weighted(a[axis+1],b[axis+1],t)
                    out[idx[node['va']]][offset+axis]=math.trunc(v) if isangle else f32(v)
        return out
    cases=((0,3,1,0,.1),(0,3,2,0,.5),(1,14.75,0,3,.85),(2,15.5,5,7.25,.5),
           (4,13.5,3,32.25,.3),(5,7.25,0,16.5,1),(0,3,1,0,0))
    maximum=0.;count=0
    for a,af,b,bf,weight in cases:
        source=sample(a,af);dest=sample(b,bf);weight=f32(weight)
        rows=subprocess.check_output([str(checker),str(package),str(a),str(af),str(b),str(bf),str(weight)],text=True).splitlines()
        assert len(rows)==38
        for line in rows:
            values=line.split();node=int(values[0]);actual=list(map(float,values[1:]));assert len(actual)==9
            for component in range(9):
                expected=weighted(source[node][component],dest[node][component],weight)
                expected=f32(math.trunc(expected)*f32(BAMS_TO_RAD)) if 3<=component<6 else f32(expected)
                error=abs(expected-actual[component]);maximum=max(maximum,error);count+=1
                assert error<.00004,(a,af,b,bf,weight,node,component,expected,actual[component],error)
    print(f'Original E SARU two-motion blend: PASS {len(cases)} poses/{count} local components, max_error={maximum:.9g}; raw-key/source equations, not retail timing or native AI')
if __name__=='__main__':
    if len(sys.argv)!=4:raise SystemExit('usage: verify_pc2004_saru_blend.py OWNER_SONIC_EXE SARU_SA3C C_CHECKER')
    verify(*map(Path,sys.argv[1:]))
