#!/usr/bin/env python3
from pathlib import Path
import math,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
visual=Path(sys.argv[1]).read_bytes();out=Path(sys.argv[2])
co=104;po=co+48;ro=po+96;no=ro+16;land=no+36;lo=land+24;total=lo+4100
b=bytearray(total)
struct.pack_into('<8s9I2fI',b,0,b'SA3CHKP\0',1,1,64,co,po,ro,no,lo,total,360000.,360100.,land)
struct.pack_into('<IIHhhh6f',b,64,0,90,19,0,0,0,0,0,0,0,0,0)
for off,form,category in [(co,0,3),(po,0,7),(po+48,2,1)]:
 b[off]=form;b[off+1]=category;struct.pack_into('<f',b,off+20,4)
for i in range(2):struct.pack_into('<BBBxI',b,ro+i*8,255,255,255,0)
for i in (1,2):struct.pack_into('<3f',b,no+i*12,15 if i==1 else -15,9.5,0)
for i in range(1025):struct.pack_into('<f',b,lo+i*4,math.sin(i*math.pi/2048))
def packet(config):return sa3dspak.build([('METADATA',b'{}',0),('CHECKPOINT',config,0),*[(f'VISUAL{i}',visual,0) for i in range(5)]])
out.write_bytes(packet(b))
bad=bytearray(b);bad[56]=1;out.with_suffix('.reserved.sa3dspak').write_bytes(packet(bad))
bad=bytearray(b);struct.pack_into('<f',bad,co+20,float('nan'));out.with_suffix('.cci.sa3dspak').write_bytes(packet(bad))
