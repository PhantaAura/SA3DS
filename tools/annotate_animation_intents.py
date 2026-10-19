#!/usr/bin/env python3
from __future__ import annotations
import argparse, csv, json
from collections import Counter
from pathlib import Path

# Exact Sonic player parameter evidence already locked by the frozen baseline.
JOG_SPEED = 0.46000000834465027
RUN_SPEED = 1.3899999856948853
EPS = 0.00001

NAMES = {
  0:'grounded_idle',
  1:'grounded_below_jog',
  2:'grounded_jog_to_run',
  3:'grounded_run_plus',
  4:'airborne_upright',
  5:'airborne_ball',
  6:'spindash_charge',
  7:'roll',
}

def classify(row):
    action=int(row['action_state']); grounded=row['grounded']=='1'; ball=row['ball_form']=='1'
    if action==2: intent=6
    elif action==3: intent=7
    elif (not grounded) or action==1: intent=5 if ball else 4
    else:
        speed=float(row['surface_tangent_speed'])
        if speed <= EPS: intent=0
        elif speed < JOG_SPEED: intent=1
        elif speed < RUN_SPEED: intent=2
        else: intent=3
    if grounded: air='none'
    else:
        vy=float(row['vel_y'])
        air='rising' if vy>EPS else ('falling' if vy<-EPS else 'apex')
    return NAMES[intent],air

def summarize(path: Path):
    counts=Counter(); air=Counter(); transitions=[]; prev=None
    with path.open(newline='') as f:
        for row in csv.DictReader(f):
            intent,phase=classify(row); counts[intent]+=1; air[phase]+=1
            if intent!=prev:
                transitions.append({'frame':int(row['frame']),'intent':intent,'action_state':int(row['action_state']),
                                    'ball_form':int(row['ball_form']),'surface_tangent_speed':float(row['surface_tangent_speed'])})
                prev=intent
    return {'frames':sum(counts.values()),'intent_counts':dict(counts),'air_phase_counts':dict(air),'transitions':transitions}

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('trace_dir',type=Path)
    ap.add_argument('--json',type=Path)
    a=ap.parse_args()
    result={}
    for p in sorted(a.trace_dir.glob('*.csv')):
        if p.name.startswith(('determinism_','divergent')): continue
        result[p.stem]=summarize(p)
    text=json.dumps(result,indent=2)
    if a.json: a.json.write_text(text+'\n')
    print(text)
if __name__=='__main__': main()
