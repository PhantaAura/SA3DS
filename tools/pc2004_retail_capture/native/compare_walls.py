#!/usr/bin/env python3
"""Compare first-loop category8 WALL updates with an external incoming-state trace."""
from pathlib import Path
import argparse,csv,math,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('frames',type=Path);p.add_argument('owner_exe',type=Path)
 p.add_argument('owner_walls',type=Path);p.add_argument('checker',type=Path)
 a=p.parse_args();rows=list(csv.DictReader(a.frames.open()));lines=[];entered=False
 for current,following in zip(rows,rows[1:]):
  if int(following['frame'])!=int(current['frame'])+1:p.error('trace must contain consecutive updates')
  active=int(current.get('loop_head_before','0'))==0x102ff60 and int(current['status'])&0x2000
  entered=entered or bool(active)
  if entered and not active:break
  try:
   speed=math.sqrt(sum(float(current['local_velocity_'+axis])**2 for axis in 'xyz'))
   values=[current['frame']]+[current['position_'+axis] for axis in 'xyz']
   values += [following['before_position_'+axis] for axis in 'xyz']+[str(speed)]
  except KeyError as e:p.error('capture needs incoming-state sidecar: '+str(e))
  if not all(math.isfinite(float(v)) for v in values):p.error('nonfinite trace value')
  lines.append(' '.join(values))
 if not entered:p.error('first EC automatic loop was not recorded')
 read=verified_reader(a.owner_exe)
 with tempfile.TemporaryDirectory(prefix='sa3ds-wall-reference-') as temporary:
  lut=Path(temporary)/'quarter-lut.bin';lut.write_bytes(read(0x88af78,4100))
  result=subprocess.run([str(a.checker.resolve()),str(a.owner_walls.resolve()),str(lut)],input='\n'.join(lines)+'\n',text=True)
 raise SystemExit(result.returncode)
if __name__=='__main__':main()
