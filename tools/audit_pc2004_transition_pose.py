#!/usr/bin/env python3
"""Independent raw-owner transition-angle oracle, not a retail runtime trace.

Reads original int32 keys without using the importer or C pose implementation.
Position/scale blending, x87 extended precision and raster output are excluded.
"""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.audit_pass11b_source_pose import RawPE,source_actions,hierarchy,f32,BAMS_TO_RAD
from pathlib import Path
import struct,math,subprocess
if len(sys.argv)!=4:raise SystemExit('usage: audit_pc2004_transition_pose.py OWNER_CHRMODELS SONIC_SA3C TRANSITION_DUMPER')
p=RawPE(Path(sys.argv[1]))
actions=source_actions(p);allnodes={x['va']:x for a in actions if a for x in hierarchy(p,a[0])};indices={va:i for i,va in enumerate(sorted(allnodes))}
s16=lambda x:(int(x)+32768)%65536-32768
rad=f32(BAMS_TO_RAD)
def motion(slot):
 root,m=actions[slot];data,n,word=struct.unpack('<III',p.read_va(m,12));stride={3:16,7:24}[word&65535]
 out=[]
 for i,node in enumerate(hierarchy(p,root)):
  row=struct.unpack('<'+'I'*(stride//4),p.read_va(data+i*stride,stride));ptr,cnt=row[1],row[3 if stride==16 else 4]
  keys=list(struct.iter_unpack('<Iiii',p.read_va(ptr,cnt*16))) if cnt else []
  out.append((node,keys))
 return out,n
def capture(node,keys,frame,n):
 if not keys:return [s16(v) for v in node['rotation']]
 prev=nxt=0
 if keys[0][0]<frame:
  nxt=1
  while nxt<len(keys):
   prev=nxt-1
   if keys[nxt][0]>=frame:break
   nxt+=1
  if nxt==len(keys):prev=len(keys)-1;nxt=0
 a,b=keys[prev],keys[nxt];t=0
 if prev!=nxt:t=f32((frame-a[0])/(b[0]-a[0]+(n if nxt<prev else 0)))
 return [s16(a[j]+math.trunc(s16(b[j]-a[j])*t)) for j in range(1,4)]
checks=0;maxerr=0
for source,destination in [(3,1),(1,3),(18,6),(14,14)]:
 sr,n=motion(source);dr,_=motion(destination);assert actions[source][0]==actions[destination][0]
 for frame in [0,.5,7.5]:
  frame=f32(frame%n)
  for weight in [0,.0625,.25,.5,.75,1]:
   rows=subprocess.check_output([sys.argv[3],sys.argv[2],str(source),str(frame),str(destination),str(weight)],text=True)
   actual={int(row.split()[0]):list(map(float,row.split()[1:])) for row in rows.splitlines()}
   for (node,sk),(dn,dk) in zip(sr,dr):
    assert node['va']==dn['va']
    ca=capture(node,sk,frame,n);cb=[s16(v) for v in (dk[0][1:] if dk else dn['rotation'])]
    for axis in range(3):
     a=ca[axis];b=a+s16(cb[axis]-a);expected=math.trunc(a*(1-weight)+b*weight)
     expectedrad=f32(f32(expected)*rad);value=actual[indices[node['va']]][axis];err=abs(value-expectedrad);maxerr=max(err,maxerr);checks+=1
     if err>2e-6:raise AssertionError((source,destination,frame,weight,indices[node['va']],axis,expectedrad,value))
print('Independent raw-owner transition angle oracle: PASS',checks,'components; maxraderror',maxerr,'; no position/scale/retail trace check')
