"""Owner-free negative gates for replay verification; synthetic is not gameplay."""
import csv
import math
import tempfile
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.verify_wca_replay import INTEGER_FIELDS,verify_cases,read_rows,compare_rows

def rejected(call):
    try:call()
    except ValueError:return
    raise AssertionError('invalid trace accepted')

fields=INTEGER_FIELDS|{'test_name','heading','move_x','move_z','camera_yaw','animation_frame','filtered_input_magnitude'}
rows=[]
names=['STAND_TURN','RUN_TURN','RUN_REVERSE','CAMERA_ROTATE','AIR_STEER','CHARGE_RELEASE','ROLL_UNCURL','JUMP_LAND']
dirs=[(0,0),(0,1),(1,1),(1,0),(1,-1),(0,-1),(-1,-1),(-1,0),(-1,1)]
for case_id in range(152):
    direction=case_id<144
    for t in range(48 if direction else 360):
        row={key:'0' for key in fields}
        row.update(test_name=f'WCA_{case_id:03d}_'+('DIRECTION' if direction else names[case_id-144]),frame=str(t+1),grounded='1')
        if direction:
            dx,dz=dirs[(case_id//2)%9];scale=1 if case_id%2 else .5
            if dx and dz:scale/=math.sqrt(2)
            camera=case_id//18*math.pi/4;angle=math.atan2(-dx,dz)
            bams=round(angle*65536/(2*math.pi))%65536
            row.update(move_x=str(-dx*scale),move_z=str(dz*scale),camera_yaw=str(camera),heading=str(camera+angle),filtered_input_angle_bams=str(bams))
        elif case_id in (148,151) and t==40:row['grounded']='0'
        elif case_id in (149,150):
            if t==40:row['action_state']='2'
            if t==80:row['action_state']='3'
            if t==120:row['animation_requested_id']='29'
        rows.append(row)
assert verify_cases(rows)['cases']==152
assert not any(compare_rows(rows,rows).values())
rejected(lambda:verify_cases(rows[:-1]))
for index,key,value in [(1,'frame','1'),(50,'move_x','1'),(100,'circle_sample_valid','1'),(200,'camera_yaw','3'),(143,'heading','2')]:
    saved=rows[index][key];rows[index][key]=value
    rejected(lambda:verify_cases(rows));rows[index][key]=saved
other=[dict(row) for row in rows]
other[100]['grounded']='0';rejected(lambda:compare_rows(rows,other))
other[100]['grounded']='1';other[100]['heading']=str(float(rows[100]['heading'])+2*math.pi);assert compare_rows(rows,other)['heading']<1e-12
other[100]['heading']='200';rejected(lambda:compare_rows(rows,other))
with tempfile.TemporaryDirectory() as tmp:
    path=Path(tmp)/'synthetic.csv'
    for value in ('nan','inf',''):
        data=dict(rows[0]);data['heading']=value
        with path.open('w',newline='') as file:
            writer=csv.DictWriter(file,fieldnames=sorted(fields));writer.writeheader();writer.writerow(data)
        rejected(lambda:read_rows(path))
print('WCA replay verifier: PASS (synthetic shape, truncation, duplicated tick, input sign, camera drift, false hardware provenance, heading and numeric rejection; NOT gameplay evidence)')
