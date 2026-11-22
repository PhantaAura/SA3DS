#!/usr/bin/env python3
"""Decode an external original-PC2004, post-Sonic-main capture."""
import argparse,csv,json,math,struct,collections,hashlib
from pathlib import Path
HEADER=struct.Struct('<8s10I2Q64s')
fields=[]
def add(name,kind):fields.append((name,kind))
for name in ['frame','input_sequence','act1_ran']:add(name,'I')
for name in ['raw_x','raw_y']:add(name,'i')
add('filtered_magnitude','f')
for name in ['filtered_angle','camera_yaw','held','pressed','released']:add(name,'I')
for prefix in ['position','world_velocity','local_velocity']:
 for axis in ['x','y','z']:add(prefix+'_'+axis,'f')
for axis in ['x','y','z']:add('task_angle_'+axis,'I')
for name in ['status','action','player_bits','player_low_status','terrain_flags','support_model']:add(name,'I')
for name in ['accel_forward','accel_normal','accel_side','spindash_charge']:add(name,'f')
add('spindash_ticks','I')
for name in ['animation_requested','animation_source','animation_transition']:add(name,'i')
add('animation_frame','f')
for prefix in ['collision_up','collision_normal','collision_normal2']:
 for axis in ['x','y','z']:add(prefix+'_'+axis,'f')
for name in ['collision_count','collision_selected','collision_valid','logical_held','logical_pressed','logical_released','camera_valid']:add(name,'I')
add('qpc','Q')
RECORD=struct.Struct('<'+''.join(kind for _,kind in fields))
assert HEADER.size==128 and RECORD.size==224
PROFILE='6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('raw',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 repo=Path(__file__).resolve().parents[3]
 output=a.output.resolve()
 if output==repo or repo in output.parents:raise SystemExit('Keep original frame data outside the source checkout')
 if a.output.exists() or a.output.with_suffix('.summary.json').exists():raise SystemExit('Output already exists')
 data=a.raw.read_bytes()
 if len(data)<HEADER.size:raise SystemExit('Truncated capture header')
 h=HEADER.unpack_from(data)
 magic,version,header_bytes,record_bytes,count,act1,errors,warmup,base,image_size,mask,start,frequency,sha=h
 if version not in (1,2) or (magic,header_bytes,record_bytes,base,mask,sha.decode())!=('SA3DPCTR'.encode(),128,224,0x400000,7,PROFILE):raise SystemExit('Unsupported capture header')
 if len(data)!=128+count*224 or count>60000 or errors or not frequency:raise SystemExit('Incomplete or failed capture')
 if (a.raw.parent/'capture.error.txt').exists():raise SystemExit('Capture producer reported an error')
 rows=[dict(zip([name for name,_ in fields],values)) for values in RECORD.iter_unpack(data[128:])]
 loop_path=a.raw.parent/'loop-state.raw'
 loop_data=loop_path.read_bytes() if loop_path.exists() else None
 loop_kinds='IffIIiII'
 loop_names=['loop_frame','loop_distance_before','loop_distance_after','loop_head_before',
             'loop_head_after','control_timer_before','task_yaw_before','motion_yaw_before']
 if loop_data is not None and len(loop_data)==count*92:
  for prefix,kind in [('before_position','f'),('before_local_velocity','f'),
                      ('before_world_velocity','f'),('before_task_angle','I'),('before_collision_up','f')]:
   for axis in ('x','y','z'):
    loop_names.append(prefix+'_'+axis);loop_kinds+=kind
 loop_record=struct.Struct('<'+loop_kinds)
 if loop_data is not None and len(loop_data)!=count*loop_record.size:
  raise SystemExit('Incomplete loop sidecar')
 if loop_data is not None:
  for index,values in enumerate(loop_record.iter_unpack(loop_data)):
   if values[0]!=index or any(not math.isfinite(v) for v,k in zip(values,loop_kinds) if k=='f'):
    raise SystemExit('Invalid loop sidecar state')
   rows[index].update(zip(loop_names,values))
 for index,row in enumerate(rows):
  if row['frame']!=index or not row['input_sequence'] or row['act1_ran'] not in (0,1):raise SystemExit('Invalid frame chronology')
  if row['qpc']<start or (index and row['qpc']<rows[index-1]['qpc']):raise SystemExit('Invalid observation time')
  if index and row['input_sequence']<rows[index-1]['input_sequence']:raise SystemExit('Input sequence moved backwards')
  if any(not math.isfinite(row[name]) for name,kind in fields if kind=='f'):raise SystemExit('Non-finite original state')
  row['runtime_animation_id']=row['animation_source'] if row['animation_transition']==2 else row['animation_requested']
  row['elapsed_ms']=(row['qpc']-start)*1000/frequency
 if sum(row['act1_ran'] for row in rows)!=act1:raise SystemExit('Physics-update count mismatch')
 if not count or not act1:raise SystemExit('No original Sonic gameplay updates observed')
 a.output.parent.mkdir(parents=True,exist_ok=True)
 with a.output.open('x',newline='') as out:
  w=csv.DictWriter(out,fieldnames=[name for name,_ in fields]+['runtime_animation_id','elapsed_ms']+(loop_names if loop_data is not None else []));w.writeheader();w.writerows(rows)
 summary={'schema':'sa3ds-original-frame-summary-v2','capture_version':version,
  'loop_sidecar_observed':loop_data is not None,'loop_record_bytes':loop_record.size if loop_data is not None else None,
  'normalized_input_source':'active_at_sonic_main_entry' if version==2 else 'legacy_unused_buffer_not_Sonic_control_input',
  'rows':count,'physics_updates':act1,'errors':errors,'warmup_skipped':warmup,'phase':'post_sonic_main','frame_number':'capture-local Sonic update index','executable_sha256':PROFILE,
  'seconds_between_first_and_last_sample':(rows[-1]['qpc']-rows[0]['qpc'])/frequency,
  'actions':dict(collections.Counter(row['action'] for row in rows)),
  'runtime_animations':dict(collections.Counter(row['runtime_animation_id'] for row in rows)),
  'physical_button_values':dict(collections.Counter(hex(row['held']) for row in rows)),
  'logical_button_values':dict(collections.Counter(hex(row['logical_held']) for row in rows)),
  'raw_axes':dict(collections.Counter(str((row['raw_x'],row['raw_y'])) for row in rows)),
  'maximum_forward_speed':max(abs(row['local_velocity_x']) for row in rows),
  'first_position':[rows[0]['position_'+axis] for axis in ['x','y','z']],
  'last_position':[rows[-1]['position_'+axis] for axis in ['x','y','z']],
  'raw_sha256':hashlib.sha256(data).hexdigest(),'csv_sha256':hashlib.sha256(a.output.read_bytes()).hexdigest(),'port_parity_verified':False,'limitations':['Captured mid-session; no matched SA3DS run yet.','Video frames and Sonic updates are different clocks.']}
 a.output.with_suffix('.summary.json').write_text(json.dumps(summary,indent=2)+'\n')
 print(json.dumps(summary,indent=2))
if __name__=='__main__':main()
