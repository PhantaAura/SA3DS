#!/usr/bin/env python3
"""Check owner E SARU geometry and sampled C animated matrices against raw source."""
from pathlib import Path
import struct,subprocess,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
from tools.audit_pass11b_source_pose import hierarchy,expected_pose,f32,BAMS_TO_RAD
from tools import sa3c

def verify(exe,package,dumper):
    read=verified_reader(exe)
    class Raw:
        read_va=staticmethod(read)
    pe=Raw();addresses=(0x96a89c,0x9669ec,0x967964,0x968adc,0x969924,0x96b444)
    actions=[struct.unpack('<II',read(a,8)) for a in addresses]
    nodes=hierarchy(pe,actions[0][0]);objects={n['va']:n for n in nodes};indices={va:i for i,va in enumerate(sorted(objects))}
    blob=Path(package).read_bytes();parsed=sa3c.parse(blob);sections={tag.decode():vars(s) for tag,s in parsed['sections'].items()}
    def row(tag,index,fmt):
        s=sections[tag];return struct.unpack_from(fmt,blob,s['offset']+index*s['stride'])
    assert sections['NODE']['count']==38 and sections['ANIM']['count']==6
    # Independently decode source primitive streams and expand source corners;
    # compare actual positions/normals/raw UVs, not only writer counts/digests.
    models=sorted(set(struct.unpack('<I',read(va+4,4))[0] for va in objects)-{0})
    meshes=0;corners=0;types=set()
    for model_index,va in enumerate(models):
        points,normals,npoints,meshptr,matptr,nmesh,nmat,*_=struct.unpack('<IIIIIHHffffi',read(va,44))
        for mi in range(nmesh):
            typ,count,stream,attrs,normalptr,colorptr,uvptr,pad=struct.unpack('<HH6I',read(meshptr+mi*28,28))
            primitive=typ&0xc000;types.add(primitive);assert not colorptr
            strips=[];cursor=stream
            for _ in range(count):
                if primitive in (0,0x4000):n=3 if primitive==0 else 4;reverse=False
                else:
                    word=struct.unpack('<H',read(cursor,2))[0];cursor+=2;n=word&32767;reverse=bool(word&32768)
                v=struct.unpack('<'+'H'*n,read(cursor,n*2));cursor+=n*2;strips.append((v,reverse))
            total=sum(len(v) for v,_ in strips);uv=list(struct.iter_unpack('<hh',read(uvptr,total*4))) if uvptr else [(0,0)]*total
            expected=[];uvindex=0
            for v,reverse in strips:
                local=[]
                for vi in v:
                    assert vi<npoints
                    local.append((*struct.unpack('<3f',read(points+vi*12,12)),*struct.unpack('<3f',read(normals+vi*12,12)),*uv[uvindex]));uvindex+=1
                if primitive==0:order=(0,1,2)
                elif primitive==0x4000:order=(0,1,2,2,1,3)
                else:
                    order=[]
                    for t in range(len(local)-2):order.extend((t+1,t,t+2) if bool(t&1)^reverse else (t,t+1,t+2))
                expected.extend(local[i] for i in order)
            m=row('MESH',meshes,'<HHIIHHII');assert m[0]==model_index and m[3]==len(expected) and m[4]==primitive
            for i,source in enumerate(expected):
                vi=row('INDX',m[2]+i,'<H')[0];assert row('VRTX',vi,'<8f')==source
            # Compare all source material fields (including texture-enable flags).
            material=struct.unpack('<IIfII',read(matptr+(typ&0x3fff)*20,20))
            saved=row('MATL',m[1],'<IIfHHIIII')
            assert saved[:3]==material[:3] and saved[3]==(material[3]&0xffff) and saved[5]==material[4]
            meshes+=1;corners+=len(expected)
    assert meshes==sections['MESH']['count']==50 and corners==732 and types=={0,0x4000,0x8000}
    lut=struct.unpack('<1025f',read(0x88af78,4100))
    def sine(angle):
        def at(i):
            i%=4096
            return lut[i] if i<1024 else lut[2048-i] if i<2048 else -lut[i-2048] if i<3072 else -lut[4096-i]
        angle&=65535;i=angle>>4;t=(angle&15)/16
        return f32(at(i)+(at(i+1)-at(i))*t)
    def identity():return [[float(i==j) for j in range(4)] for i in range(4)]
    def mul(a,b):return [[f32(sum(a[i][k]*b[k][j] for k in range(4))) for j in range(4)] for i in range(4)]
    maximum=0.;checks=0;poses=0
    for slot in range(6):
        _,frames,_=expected_pose(pe,actions,objects,indices,0,slot)
        for frame in (0.,.5,3.,frames/2+.25,frames-1.):
            _,_,pose=expected_pose(pe,actions,objects,indices,frame,slot)
            rows=subprocess.check_output([str(dumper),str(package),str(slot),str(frame)],text=True).splitlines()
            actual={int(v[0]):list(map(float,v[1:])) for v in map(str.split,rows)}
            def walk(va,parent):
                nonlocal checks,maximum
                while va:
                    node=objects[va];p=pose[indices[va]];assert not node['flags']&32
                    if node['flags']&64:p={'position':node['position'],'rotation':[a*BAMS_TO_RAD for a in node['rotation']],'scale':node['scale']}
                    m=identity()
                    for i in range(3):m[i][3]=p['position'][i]
                    for axis in (2,1,0):
                        angle=round(p['rotation'][axis]/BAMS_TO_RAD);s=sine(angle);c=sine(angle+16384);r=identity()
                        if axis==0:r[1][1]=r[2][2]=c;r[1][2]=-s;r[2][1]=s
                        if axis==1:r[0][0]=r[2][2]=c;r[0][2]=s;r[2][0]=-s
                        if axis==2:r[0][0]=r[1][1]=c;r[0][1]=-s;r[1][0]=s
                        m=mul(m,r)
                    scale=identity()
                    for i in range(3):scale[i][i]=p['scale'][i]
                    world=mul(parent,mul(m,scale));got=actual[indices[va]]
                    for i in range(4):
                        for j in range(4):
                            error=abs(world[i][j]-got[j*4+i]);maximum=max(maximum,error);checks+=1
                            assert error<.00025,(slot,frame,indices[va],i,j,error)
                    if node['child']:walk(node['child'],world)
                    va=node['sibling']
            walk(actions[slot][0],identity());poses+=1
    print(f'Original E SARU assets: PASS {meshes} meshes/{corners} exact source corners/materials, {poses} poses/{checks} matrix components, max_error={maximum:.9g}; N-poly follows SA Tools strips; no retail/raster/AI proof')
if __name__=='__main__':
    if len(sys.argv)!=4:raise SystemExit('usage: verify_pc2004_saru_assets.py OWNER_SONIC_EXE SARU_SA3C C_WORLD_DUMPER')
    verify(*map(Path,sys.argv[1:]))
