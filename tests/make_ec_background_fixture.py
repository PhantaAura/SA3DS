#!/usr/bin/env python3
"""Source-neutral original-background wrapper fixtures."""
from pathlib import Path
import struct,subprocess,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.build_pc2004_ec_background import make_package,config_blob
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
subprocess.run([sys.executable,str(ROOT/'tests/make_emerald_coast_visual_fixture.py'),str(out/'visual.sa3dspak')],check=True)
visual=(out/'visual.sa3dspak').read_bytes()
rows=[]
for act in range(2):
 for clip in range(3):rows.append((act,clip,1.+act,2.+clip,3.+act+clip,1.,2500.+clip,1.,2000.,1.,3000.,0xff123456,1,0x123456,3,0))
(out/'valid.sa3dspak').write_bytes(make_package(visual,rows,{}))
def save(name,changed):
 (out/(name+'.sa3dspak')).write_bytes(make_package(visual,changed,{}))
for name,index,value in [('bad_act',0,2),('bad_clip',1,3),('nan_scale',2,float('nan')),('zero_scale',3,0.),('clip_order',6,.5),('fog_order',10,.5),('fog_flag',12,2),('flags',14,1),('reserved',15,1)]:
 changed=list(rows);row=list(changed[0]);row[index]=value;changed[0]=tuple(row);save(name,changed)
bad=bytearray(visual);bad[-1]^=1
(out/'bad_nested.sa3dspak').write_bytes(make_package(bytes(bad),rows,{}))
config=bytearray(config_blob(rows));struct.pack_into('<I',config,12,7)
(out/'bad_count.sa3dspak').write_bytes(sa3dspak.build([('METADATA',b'{}',0),('BACKGROUND',bytes(config),0),('VISUAL',visual,0)]))
(out/'missing.sa3dspak').write_bytes(sa3dspak.build([('METADATA',b'{}',0),('BACKGROUND',config_blob(rows),0)]))
valid=(out/'valid.sa3dspak').read_bytes();(out/'truncated.sa3dspak').write_bytes(valid[:-1])
bad=bytearray(valid);bad[-1]^=1;(out/'bad_crc.sa3dspak').write_bytes(bad)
