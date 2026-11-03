#!/usr/bin/env python3
"""Exact-owner SARU SET/CCL round-trip and source initialization equation check."""
from pathlib import Path
import struct,sys,tempfile,subprocess
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader
from tools.importers.pc2004_set import read_set_source
from tools.build_pc2004_emerald_coast_visual import read_source
from tools.build_pc2004_saru_definitions import build
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
def verify(exe,system,package):
 r=verified_reader(exe);original=Path(package).read_bytes();expected,_=build(exe,system);assert original==expected
 parsed=sa3dspak.parse(original);b=next(c['data'] for c in parsed['chunks'] if c['type']=='SARU')
 co=struct.unpack_from('<I',b,24)[0];assert b[co:co+48]==r(0x96c9e8,48)
 assert b[co+48:co+144]==r(0x91cbf0,96)
 for i,category in enumerate((0,3)):
  assert struct.unpack_from('<BBB x I',b,co+144+i*8)==(r(0x90e3bc+category,1)[0],r(0x90e3b0+category,1)[0],r(0x90e3c8+category,1)[0],struct.unpack('<I',r(0x90e3d4+category*4,4))[0])
 lut=r(0x88af78,4100);values=struct.unpack('<1025f',lut)
 def sample(i):
  i%=4096
  return values[i] if i<1024 else values[2048-i] if i<2048 else -values[i-2048] if i<3072 else -values[4096-i]
 def sine(a):
  a&=65535;i=a>>4;t=(a&15)/16.;return sample(i)+(sample(i+1)-sample(i))*t
 with tempfile.TemporaryDirectory(prefix='sa3ds-saru-owner-') as temp:
  l=Path(temp)/'owner-quarter.bin';l.write_bytes(lut)
  lines=subprocess.check_output(['bash',str(ROOT/'tests/run_saru.sh'),str(package),str(l)],text=True).splitlines()
 sources=[read_set_source(f'SET010{act}S.BIN',read_source(system,f'SET010{act}S.BIN')) for act in (0,1)]
 rows=[(act,i,p) for act,source in enumerate(sources) for i,p in enumerate(source.records) if p.type_word&4095==29]
 assert len(lines)==len(rows)==struct.unpack_from('<I',b,12)[0]
 maximum=0.
 for index,((act,row,p),line) in enumerate(zip(rows,lines)):
  v=line.split();assert list(map(int,v[:2]))==[act,row]
  assert struct.unpack_from('<II32s',b,64+index*40)==(act,row,p.raw)
  radius=max(1.,p.parameters[2]);a=p.angle_words[0]&65535
  dx=f32(f32(sine(a+16384))*radius);dz=f32(-f32(sine(a))*radius)
  pos=[f32(p.position[0]+dx),p.position[1],f32(p.position[2]+dz)]
  error=max(abs(f32(float(x))-y) for x,y in zip(v[2:5],pos));maximum=max(maximum,error)
  assert error<4e-5,(act,row,error)
  z=p.angle_words[2]&65535;mode=1 if p.parameters[1]<0 else 2 if p.parameters[1]>=1000 else 0
  assert list(map(int,v[5:12]))==[p.angle_words[0],p.angle_words[1],z>>8,z&255,mode,12345*20//32768,0x2100 if p.parameters[0]<0 else 0x100]
  assert f32(float(v[12]))==radius
 print(f'Original SARU definitions/init: PASS {len(rows)} exact SET/CCL rows with source quarter-LUT orbit; max_error={maximum}; equations only, no retail/AI/native actor proof')
if __name__=='__main__':
 if len(sys.argv)!=4:raise SystemExit('usage: verify_pc2004_saru_definitions.py OWNER_SONIC_EXE OWNER_SYSTEM SARU_PACKAGE')
 verify(*map(Path,sys.argv[1:]))
