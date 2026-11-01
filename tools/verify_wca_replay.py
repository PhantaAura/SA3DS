#!/usr/bin/env python3
"""Check complete WCA input/transition replay and compare bounded host/native rows.

This verifies port consistency, never retail authority or full-stage completion.
Execution/build identity still requires the observed native test and artifact hash.
"""
from __future__ import annotations
import argparse
import csv
import json
import math
from pathlib import Path

DIRECTION_CASES=144
DIRECTION_TICKS=48
TRANSITION_NAMES=['STAND_TURN','RUN_TURN','RUN_REVERSE','CAMERA_ROTATE',
                  'AIR_STEER','CHARGE_RELEASE','ROLL_UNCURL','JUMP_LAND']
INTEGER_FIELDS={'trace_version','test_id','frame','filtered_input_angle_bams',
    'jump_pressed','jump_held','jump_released','action_pressed','action_held','action_released',
    'grounded','action_state','surface_id','surface_flags','angle_delta_bams','turn_step_bams',
    'turn_kind','used_brake_branch','crossed_zero','side_stopped_at_jog_threshold',
    'runtime_animation_id','action_slot','selector_provisional','spindash_ticks','ball_form',
    'jump_ticks','animation_requested_id','landing_classification_valid','landing_classified_request',
    'landing_impact_valid','landing_air_action','processed_analog_active','processed_analog_valid',
    'idle_counter','circle_sample_valid','circle_raw_x','circle_raw_y',
    'circle_normalized_angle_bams','pc2004_action_state','pc2004_action_state_valid'}

def read_rows(path):
    with Path(path).open(newline='') as stream:
        reader=csv.DictReader(stream)
        if not reader.fieldnames or len(set(reader.fieldnames))!=len(reader.fieldnames):
            raise ValueError('missing/duplicate CSV header')
        required=INTEGER_FIELDS|{'test_name','heading','move_x','move_z','camera_yaw',
            'animation_frame','filtered_input_magnitude'}
        if not required.issubset(reader.fieldnames):raise ValueError('incomplete trace schema')
        rows=list(reader)
    for index,row in enumerate(rows):
        if None in row or any(value is None or value=='' for value in row.values()):
            raise ValueError(f'incomplete row {index}')
        for key,value in row.items():
            if key=='test_name':continue
            if not math.isfinite(float(value)):raise ValueError(f'nonfinite {key} at row{index}')
            if key in INTEGER_FIELDS and float(value)!=int(float(value)):
                raise ValueError(f'noninteger {key} at row{index}')
    return rows

def verify_cases(rows):
    expected_count=DIRECTION_CASES*DIRECTION_TICKS+len(TRANSITION_NAMES)*360
    if len(rows)!=expected_count:raise ValueError(f'expected {expected_count} ticks, got {len(rows)}')
    offset=0;max_heading_error=0;cases=[]
    for case_id in range(DIRECTION_CASES+len(TRANSITION_NAMES)):
        direction=case_id<DIRECTION_CASES
        count=DIRECTION_TICKS if direction else 360
        name=f'WCA_{case_id:03d}_'+('DIRECTION' if direction else TRANSITION_NAMES[case_id-DIRECTION_CASES])
        case=rows[offset:offset+count];offset+=count
        for tick,row in enumerate(case):
            if row['test_name']!=name or int(row['frame'])!=tick+1:
                raise ValueError(f'scenario/tick order mismatch in {name} at {tick}')
            if int(row['circle_sample_valid']):raise ValueError('scripted input falsely marked as hardware')
        if direction:
            cam=case_id//18*math.pi/4
            dirs=[(0,0),(0,1),(1,1),(1,0),(1,-1),(0,-1),(-1,-1),(-1,0),(-1,1)]
            dx,dz=dirs[(case_id//2)%9];scale=1 if case_id%2 else .5
            if dx and dz:scale/=math.sqrt(2)
            for row in case:
                if abs(float(row['move_x'])+dx*scale)>1e-6 or abs(float(row['move_z'])-dz*scale)>1e-6:
                    raise ValueError(f'wrong screen/core input conversion in {name}')
                if abs(float(row['camera_yaw'])-cam)>1e-6:raise ValueError(f'camera drift in {name}')
            if dx or dz:
                row=case[-1]
                target=cam+int(row['filtered_input_angle_bams'])*2*math.pi/65536
                delta=float(row['heading'])-target
                error=abs(math.atan2(math.sin(delta),math.cos(delta)))
                max_heading_error=max(max_heading_error,error)
                if error>.2:raise ValueError(f'heading did not converge in {name}: {error}')
        cases.append(case)
    transitions=cases[DIRECTION_CASES:]
    for index in (4,7):
        case=transitions[index]
        if not any(int(r['grounded'])==0 for r in case) or int(case[-1]['grounded'])!=1:
            raise ValueError('jump/landing transition not exercised')
    for index in (5,6):
        case=transitions[index];states={int(r['action_state']) for r in case}
        if not {0,2,3}.issubset(states):raise ValueError('charge/Roll/ground transition not exercised')
        if 29 not in {int(r['animation_requested_id']) for r in case}:
            raise ValueError('Uncurl request not exercised')
    return {'ticks':expected_count,'cases':len(cases),'max_final_direction_error_radians':max_heading_error,
            'animation_ids':sorted({int(r['runtime_animation_id']) for r in rows}),
            'authority':'PORT-INTERNAL REPLAY; NOT RETAIL VERIFIED'}

def compare_rows(actual,expected,absolute=.0002,relative=1e-6):
    if len(actual)!=len(expected):raise ValueError('trace lengths differ')
    max_errors={}
    for index,(a,b) in enumerate(zip(actual,expected)):
        if set(a)!=set(b):raise ValueError('trace schemas differ')
        for key,av in a.items():
            bv=b[key]
            if key=='test_name':
                if av!=bv:raise ValueError(f'scenario mismatch at row{index}')
            elif key in INTEGER_FIELDS:
                if int(av)!=int(bv):raise ValueError(f'{key} mismatch at row{index}: {av} vs {bv}')
            else:
                av,bv=float(av),float(bv)
                angular=key in {'heading','camera_yaw'}
                error=abs(math.atan2(math.sin(av-bv),math.cos(av-bv))) if angular else abs(av-bv)
                max_errors[key]=max(max_errors.get(key,0),error)
                matches=error<=absolute if angular else math.isclose(av,bv,abs_tol=absolute,rel_tol=relative)
                if not matches:
                    raise ValueError(f'{key} mismatch at row{index}: {av} vs {bv}')
    return max_errors

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('trace',type=Path)
    parser.add_argument('--host',type=Path)
    args=parser.parse_args()
    actual=read_rows(args.trace);summary=verify_cases(actual)
    if args.host:
        expected=read_rows(args.host);verify_cases(expected)
        summary['host_comparison_max_errors']=compare_rows(actual,expected)
    print(json.dumps(summary,indent=2))

if __name__=='__main__':main()
