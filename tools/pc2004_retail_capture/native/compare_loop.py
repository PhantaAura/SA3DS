#!/usr/bin/env python3
"""Feed recorded incoming first-loop state to the portable43C130 component."""
from pathlib import Path
import argparse,csv,subprocess

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('frames',type=Path);p.add_argument('owner_paths',type=Path);p.add_argument('checker',type=Path)
 a=p.parse_args();rows=list(csv.DictReader(a.frames.open()));lines=[]
 for r in rows:
  # The exact first EC automatic loop; other paths need their own admission.
  if int(r.get('loop_head_before','0'))!=0x102ff60 or not int(r['status'])&0x2000:continue
  keys=['frame','loop_distance_before','before_local_velocity_x']
  keys += [prefix+'_'+axis for prefix in ['before_position','before_world_velocity','before_collision_up','before_task_angle'] for axis in 'xyz']
  keys += ['motion_yaw_before','loop_distance_after','task_angle_y']
  try:lines.append(' '.join(r[k] for k in keys))
  except KeyError as e:p.error('capture needs the92-byte incoming-state sidecar: '+str(e))
 if not lines:p.error('no first-loop held-input state found')
 result=subprocess.run([str(a.checker.resolve()),str(a.owner_paths.resolve())],input='\n'.join(lines)+'\n',text=True)
 raise SystemExit(result.returncode)
if __name__=='__main__':main()
