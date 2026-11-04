#!/usr/bin/env python3
"""Convert exact owner's whale morph banks to private, pointer-free SD data."""
from pathlib import Path
import argparse,hashlib,json,math,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader,PROFILE_SHA256
SPANS={
 (0x4054c0,136):'de81ccfcc04ff83b96d3b5505355c444ee87bb88c98c70d9a8777fc06d1b4f0b',
 (0x405550,50):'36a6b546b42ea9268f952b2649fcf71c1d595e2f7724fe0d7f5cc3f75b9377cb',
 (0x405590,160):'942b564aab2eab26f17bd8f7a9f7e536b972055562b99e4ce9b06db618b22c26',
 (0x405630,69):'8c0d8b1518d3f5db94a2dffbf25b0590dda2c912f6842cb8e16288b8b5117044',
 (0x405aa0,816):'4aacf88e732bb9ad233e585305ca2e95597c214a14c4ab75346c4111e6ff4f39',
 (0x405ef0,320):'c63258b907c5416594cf357fcd59bf252b28a40cb2880e0dc41d302a2e1ed1d5',
 (0x502210,144):'74b7ecfb2df1f7ba7084bee9a8a2023f8b523c42d9b145b263a0654947830345',
}
def build(exe):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 assert r(0x7dcb80,4)==struct.pack('<f',1e-5)
 assert struct.unpack('<4I',r(0x900884,16))[3]==0x405aa0
 assert struct.unpack('<I',r(0x106bb20+8,4))[0]==238
 banks=[]
 for header,md,duration,count in [(0x10b9dd0,0x10b9dc0,32,9),(0x10741f8,0x10741e8,30,7)]:
  assert struct.unpack('<IIHH',r(header,12))==(md,duration,0x30,2)
  pos,norm,np,nn=struct.unpack('<4I',r(md,16));assert np==nn==count
  channels=[]
  for pointer in (pos,norm):
   rows=list(struct.iter_unpack('<II',r(pointer,count*8)))
   assert rows[0][0]==0 and all(rows[i][0]<rows[i+1][0]<=duration for i in range(count-1))
   channels.append(rows)
  banks.append((duration,channels))
 key_start=80;key_size=sum(len(rows)*8 for _,channels in banks for rows in channels)
 data=bytearray(key_start+key_size);cursor=key_start;arrays={}
 for bank,(duration,channels) in enumerate(banks):
  offsets=[]
  for rows in channels:
   offsets.extend((len(rows),cursor))
   for frame,address in rows:
    if address not in arrays:
     raw=r(address,238*12);assert all(math.isfinite(v) for v in struct.unpack('<714f',raw))
     arrays[address]=len(data);data.extend(raw)
    struct.pack_into('<II',data,cursor,frame,arrays[address]);cursor+=8
  struct.pack_into('<6I',data,32+bank*24,duration,*offsets,0)
 assert cursor==key_start+key_size
 struct.pack_into('<8s6I',data,0,b'SA3WSHP\0',1,2,238,32,24,len(data))
 metadata=json.dumps({'source_exe_sha256':PROFILE_SHA256,'scope':'original whale cycle/exit position and normal morphs in source vertex order; geometry/textures/skeletal transforms separate'},sort_keys=True).encode()
 return sa3dspak.build([('METADATA',metadata,0),('WHALE_SHAPE',bytes(data),0)])
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 data=build(a.exe);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'Whale shape private package{len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
