#!/usr/bin/env python3
"""Independent raw-owner morph-byte and sampled-vector oracle."""
from pathlib import Path
import argparse,math,struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.build_pc2004_whale_shape import build
from tools.verify_pc2004_ec_route import verified_reader
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def audit(exe):
 r=verified_reader(exe);data=build(exe);raw_banks=[]
 #Direct source addresses, independent of the converted package descriptors.
 for header in (0x10b9dd0,0x10741f8):
  md,duration,kind,interp=struct.unpack('<IIHH',r(header,12));assert (kind,interp)==(0x30,2)
  p,n,np,nn=struct.unpack('<4I',r(md,16));channels=[]
  for address,count in ((p,np),(n,nn)):
   rows=[]
   for frame,pointer in struct.iter_unpack('<II',r(address,count*8)):
    raw=r(pointer,238*12);rows.append((frame,raw,list(struct.iter_unpack('<3f',raw))))
   channels.append(rows)
  raw_banks.append((duration,channels))
 with tempfile.TemporaryDirectory(prefix='sa3ds-owner-whale-shape-') as directory:
  root=Path(directory);good=root/'shape.sa3dspak';good.write_bytes(data);out=root/'sample.bin';binary=root/'dump'
  subprocess.run(['cc','-std=c11','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror','-I'+str(ROOT/'src/game'),'-I'+str(ROOT/'src/sa3dspak'),str(ROOT/'tests/whale_shape_dump.c'),str(ROOT/'src/game/whale_shape.c'),str(ROOT/'src/sa3dspak/sa3dspak.c'),'-lm','-o',str(binary)],check=True)
  subprocess.run([str(binary),str(good),str(out)],check=True)
  result=out.read_bytes();cursor=0
  def take(n):
   nonlocal cursor
   b=result[cursor:cursor+n];assert len(b)==n;cursor+=n;return b
  def integer():return struct.unpack('<I',take(4))[0]
  assert integer()==238
  max_error=0;values=0
  for duration,channels in raw_banks:
   assert integer()==duration
   for rows in channels:
    assert integer()==len(rows)
    for frame,raw,_ in rows:assert integer()==frame and take(238*12)==raw
   for tick in range(duration*8+1):
    frame=(tick*.125)%duration
    for normal,rows in enumerate(channels):
     key=max(i for i,row in enumerate(rows) if row[0]<=int(frame));next_key=(key+1)%len(rows)
     first,second=rows[key],rows[next_key];gap=(second[0]-first[0])&0xffffffff
     divisor=f32(gap if gap else duration-second[0]);weight=f32((frame-first[0])/divisor)
     actual=list(struct.iter_unpack('<3f',take(238*12)))
     for vertex,(a,b) in enumerate(zip(first[2],second[2])):
      mixed=[math.fsum(((1-weight)*x,weight*y)) for x,y in zip(a,b)]
      stored=[f32(x) for x in mixed]
      if normal:
       squared=math.fsum((stored[0]**2,stored[1]**2,mixed[2]**2))
       expected=[f32(x/math.sqrt(squared)) for x in stored] if squared>f32(1e-5) else [0,0,1]
      else:expected=stored
      for got,want in zip(actual[vertex],expected):
       error=abs(got-want);max_error=max(max_error,error);values+=1
       assert math.isfinite(got) and error<=max(2e-6,abs(want)*2e-7),(tick,normal,vertex,got,want)
  assert cursor==len(result)
  payload=next(c['data'] for c in sa3dspak.parse(data)['chunks'] if c['type']=='WHALE_SHAPE')
  point=struct.unpack_from('<I',payload,struct.unpack_from('<I',payload,40)[0]+4)[0]
  for name,offset,value in [('version',8,2),('vertices',16,0),('bankoffset',20,28),('stride',24,20),('size',28,len(payload)-4),('duration',32,0),('keycount',36,1),('keyoffset',40,0xfffffffc),('reserved',52,1),('firstframe',80,1),('vectoroffset',84,0xfffffffc),('nan',point,0x7fc00000)]:
   b=bytearray(payload);struct.pack_into('<I',b,offset,value);bad=root/(name+'.sa3dspak')
   bad.write_bytes(sa3dspak.build([('METADATA',b'{}',0),('WHALE_SHAPE',bytes(b),0)]));subprocess.run([str(binary),str(bad)],check=True)
  bad=root/'crc.sa3dspak';b=bytearray(data);b[-1]^=1;bad.write_bytes(b);subprocess.run([str(binary),str(bad)],check=True)
 print(f'Whale shape owner oracle: PASS all32 key-array byte records/{values} sampled components across both banks; max_error={max_error:.9g};12 malformed+CRC rejection; native display separate')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);a=ap.parse_args();audit(a.exe)
