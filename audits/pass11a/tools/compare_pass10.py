#!/usr/bin/env python3
"""Compare owner-side sampled Sonic bounds with same-mode/yaw Pose10 hardware CSV.

Compares only frame/mode/preset matches, never claims retail-PC visual parity.
"""
import argparse,csv,math,sys
from pathlib import Path
KEYS=('min_x','min_y','min_z','max_x','max_y','max_z')

def compare(host,device,atol=0.00005):
    with open(host,newline='') as f: h=list(csv.DictReader(f))
    with open(device,newline='') as f: d=list(csv.DictReader(f))
    if not h or not d: raise ValueError('empty CSV')
    results=[]
    for row in h:
        action=int(row['action']); frame=float(row['frame']); bind=int(row['bind'])
        matches=[v for v in d if int(v['action'])==action and abs(float(v['frame'])-frame)<0.0005
                 and int(v['bind'])==bind and abs(float(v['yaw_radians']))<0.0005]
        if not matches:raise ValueError(f'No Pose10 yaw-zero sample for action {action}, frame {frame}, bind={bind}')
        maxerr=0
        for v in matches:
            for k in KEYS:
                av,bv=float(row[k]),float(v[k])
                if not math.isfinite(av) or not math.isfinite(bv):raise ValueError('non-finite bound')
                maxerr=max(maxerr,abs(av-bv))
            for k in ('root','vertices','triangles'):
                if int(row[k])!=int(v[k]):raise ValueError(f'{k} mismatch action={action} bind={bind}: {row[k]} != {v[k]}')
        if maxerr>atol:raise ValueError(f'bounds mismatch action={action} bind={bind} maxerr={maxerr:.9f}')
        results.append((action,frame,bind,maxerr,len(matches)))
    return results

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('host_csv');p.add_argument('pass10_pose_metrics');p.add_argument('--tolerance',type=float,default=0.00005)
    a=p.parse_args()
    try:
        results=compare(a.host_csv,a.pass10_pose_metrics,a.tolerance)
        for action,frame,bind,err,n in results:
            print(f'PASS action={action} frame={frame:g} bind={bind} matched_Pose10_rows={n} max_abs_bound_error={err:.6g}')
        print('Pass11A host versus Pose10 geometry parity PASS (not retail rendering parity)')
    except (ValueError,KeyError,FileNotFoundError) as e:
        print(f'FAIL: {e}',file=sys.stderr);sys.exit(1)
