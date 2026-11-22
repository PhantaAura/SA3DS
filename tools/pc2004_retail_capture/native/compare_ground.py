#!/usr/bin/env python3
"""Compare the first-loop static contact component with original terrain state."""
from pathlib import Path
import argparse,csv,math,subprocess,sys
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_stage import PC2004StageDecoder

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('frames',type=Path);p.add_argument('owner_exe',type=Path)
 p.add_argument('owner_collision',type=Path);p.add_argument('checker',type=Path)
 a=p.parse_args();rows=list(csv.DictReader(a.frames.open()));d=PC2004StageDecoder(a.owner_exe)
 identities={}
 for i,r in enumerate(d.collisions(d.landtable(1,0))):identities.setdefault(r.object_va,[]).append(i)
 lines=[]
 for index,r in enumerate(rows):
  if int(r.get('loop_head_before','0'))!=0x102ff60 or not int(r['status'])&0x2000:continue
  if not index or int(rows[index-1]['frame'])+1!=int(r['frame']):p.error('consecutive incoming support state required')
  if int(r.get('ground_calls','0'))!=1:p.error('capture needs one449500 entry/exit per selected update')
  if int(r['ground_terrain_flags'])&0x24008:p.error('exceptional terrain is outside this static-loop comparator')
  if float(r['ground_height'])!=10 or float(r['ground_pos_error'])!=2:p.error('unexpected original Sonic body parameters')
  matches=identities.get(int(rows[index-1]['support_model']),[])
  if len(matches)!=1:p.error('previous support identity is absent or ambiguous')
  for axis in 'xyz':
   if int(r['ground_angle_'+axis])!=int(r['before_task_angle_'+axis]) and axis!='y':p.error('incoming task tilt changed before solver entry')
  keys=['frame']+[prefix+'_'+axis for prefix in ['ground_position','before_collision_up','ground_motion','position','ground_normal'] for axis in 'xyz']
  values=[r[k] for k in keys]+[str(matches[0]),r['ground_local_x']]
  if not all(math.isfinite(float(v)) for v in values):p.error('nonfinite original state')
  lines.append(' '.join(values))
 if not lines:p.error('no first-loop static contact updates found')
 result=subprocess.run([str(a.checker.resolve()),str(a.owner_collision.resolve())],input='\n'.join(lines)+'\n',text=True)
 raise SystemExit(result.returncode)
if __name__=='__main__':main()
