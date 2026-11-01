#!/usr/bin/env python3
"""Independent original-node motion world-matrix check (not retail raster parity).

Uses raw owner hierarchy, raw-key local oracle, and owner Ninja lookup samples.
No owner geometry, lookup table or source bytes are embedded in this program.
"""
from pathlib import Path
import argparse,json,math,struct,subprocess,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.audit_pass11b_source_pose import RawPE,source_actions,hierarchy,expected_pose,f32,BAMS_TO_RAD
from tools.verify_pc2004_ec_route import verified_reader
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('chrmodels',type=Path);ap.add_argument('sonic_exe',type=Path)
ap.add_argument('sa3c',type=Path);ap.add_argument('dumper',type=Path)
args=ap.parse_args()
pe=RawPE(args.chrmodels);actions=source_actions(pe)
objects={node['va']:node for action in actions if action for node in hierarchy(pe,action[0])}
indices={va:i for i,va in enumerate(sorted(objects))}
read=verified_reader(args.sonic_exe);lut=struct.unpack('<1025f',read(0x88AF78,4100))
def sample(index):
    index &=4095
    if index<1024:return lut[index]
    if index<2048:return lut[2048-index]
    if index<3072:return -lut[index-2048]
    return -lut[4096-index]
def sine(bams):
    bams &=65535;i=bams>>4;t=(bams&15)/16
    return f32(sample(i)+(sample(i+1)-sample(i))*t)
def identity():return [[float(i==j) for j in range(4)] for i in range(4)]
def product(a,b):return [[f32(sum(a[i][k]*b[k][j] for k in range(4))) for j in range(4)] for i in range(4)]
def rotation(axis,radians):
    angle=round(radians/BAMS_TO_RAD);s=sine(angle);c=sine(angle+16384)
    m=identity()
    if axis==0:m[1][1]=c;m[1][2]=-s;m[2][1]=s;m[2][2]=c
    if axis==1:m[0][0]=c;m[0][2]=s;m[2][0]=-s;m[2][2]=c
    if axis==2:m[0][0]=c;m[0][1]=-s;m[1][0]=s;m[1][1]=c
    return m
checks=0;max_error=0;cases=0
mapping=json.loads((Path(__file__).resolve().parents[1]/'docs/PC2004_SONIC_ANIMDATA_REMAP.json').read_text())
for slot in sorted(set(mapping['dense_to_source_action_slot'][:30])):
    _,n,_=expected_pose(pe,actions,objects,indices,0,slot)
    for frame in (0.,.5,n/2+.25,n-1.):
        _,_,pose=expected_pose(pe,actions,objects,indices,frame,slot)
        text=subprocess.check_output([str(args.dumper),str(args.sa3c),str(slot),str(frame)],text=True)
        actual={int(row.split()[0]):list(map(float,row.split()[1:])) for row in text.splitlines()}
        def walk(va,parent):
            global checks,max_error
            while va:
                node=objects[va];item=pose[indices[va]]
                # Source SKIP branch applies the bind TRS, not motion channels.
                if node['flags']&64:
                    item={'position':node['position'],'rotation':[v*BAMS_TO_RAD for v in node['rotation']], 'scale':node['scale']}
                if node['flags']&32:raise AssertionError('mixed rotation-order profile needs separate source proof')
                m=identity()
                for i in range(3):m[i][3]=item['position'][i]
                for axis in (2,1,0):m=product(m,rotation(axis,item['rotation'][axis]))
                scale=identity()
                for i in range(3):scale[i][i]=item['scale'][i]
                world=product(parent,product(m,scale))
                got=actual[indices[va]]
                for i in range(4):
                    for j in range(4):
                        error=abs(world[i][j]-got[j*4+i]);max_error=max(max_error,error);checks+=1
                        if error>0.00025:raise AssertionError((slot,frame,indices[va],i,j,world[i][j],got[j*4+i],error))
                if node['child']:walk(node['child'],world)
                va=node['sibling']
        walk(actions[slot][0],identity());cases+=1
print(f'Original animated world oracle: PASS {cases} poses, {checks} matrix components, max_error={max_error:.9g}; source LUT versus analytic port trig, not retail/raster parity')
