#!/usr/bin/env python3
"""Owner-only Emerald Coast camera package: Sonic CAM rows per act and the
mode-table priority bytes (0x975410 +4) used by 437E90/437F20 region pushes."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader
from tools.importers.pc2004_cam import list_cam_sources
SPANS={ #decoded camera code (mode4, region drivers and pushes)
 (0x437e90,140):'b01af23f5baca27ae08ff77aed8f95930a2fafd558f36e232bdc6f0a23e6f2f6',
 (0x437f20,135):'cdd3e8123f3d7a101fd8c6d6f8bdaa38e5f78fc42bb94059eb77f6cf71b2b945',
 (0x4367b0,152):'d54a3af75f7f44746a021d20f6e0fedb17730022af71b34163a9d6122a822885',
 (0x436850,634):'d3851f6e3149fbe9900911a9b20c096a1a3ee66de5babec07f81b4bbf3336f5f',
 (0x436ad0,385):'8f0baa4604e1a713d588785ea005278133ba10652e07385dc095c5955864a473',
 (0x469590,1536):'7be96901c236fe46f5b9246c9cfcd4e97df868ca5d0f587164596df42e31f66c',
 #437FB0 selector, its 437750/4377A0/437900 cases, enter/leave/pop/set, adjust0 no-op, 436560
 (0x437fb0,204):'4a4eb83a604741d69d054a7208af9831f39db35d944a5515bcb2664fc05780ce',
 (0x437750,76):'9ad345ac91ab4c3751d5eb7b4cddc59b017bf85a02bfcfef21c3b9291cde9a5f',
 (0x4377a0,347):'253eee84ee71b8ea0d8a0555b1e0d1426b5abe2e1e1b09cb57598d88fdfb15a4',
 (0x437900,275):'3dc8fc12038ba7d0c81f6e9197ae42351a050c572f9ec4048fe27afde037e1ef',
 (0x435de0,368):'c4b75e70cda5ff0b9300f70e6b65b398642c3dca3cf1861f70cb64e447cffae8',
 (0x435f50,240):'0c61b1efe76585b6dafb76535bb33e9f2cf1ac765b1885856cd1f4f6f40e1942',
 (0x436140,160):'33f008b46d824c5f9cc3a3ec4f39ba034fbce5b4d5a6e584021a3db36c45a787',
 (0x436040,246):'83a70ac97cd60c7916e0a57947ce190531ea856168efefd30fe9559bfb1aa45c',
 (0x467d80,60):'fde050e1e4a93dccdf8aa943d7aefdee21856144cecbd877e7c0eab20326230d',
 (0x436560,39):'1c4f46fcae04f41304d120bc176b6eae3c5a46ea525a84e9335ac65964bcd3f5',
 #modes 39/31/20, 4629A0 standing orbit, 436C60 (dead blend), 436690 still flag
 (0x469e90,526):'02dfa96b953aa187dfae0d0e5ffa730d17bf890f9059eabd96aa860835bf69bf',
 (0x46a0a0,685):'339a9e2c217ef3bc89f0ddc460ce61d64a63befe04707adcc6ca6001eeb9cc72',
 (0x46a350,320):'6d6ea2d4b97b53bb8e8c5a1769561f8be68c1caca9a4660717cdb525b3d4f94c',
 (0x4629a0,407):'dc78759b2f8d34bdc04466413758e038e82c6d5c433cbc275d04d6afd3aac02d',
 (0x436c60,101):'ebf5cd605cc65e6eeaaaa0ccaf695d32ce0933e3835fe60a30c78c25d1decef6',
 (0x436690,96):'51477f3f553f94684ef684d8e3a1fe09319a8f872fa11db77623653a538d0f67',
}
def build(exe,system):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 assert struct.unpack('<I',r(0x9759c4,4))[0]==0x975410
 prio=bytearray(256);count=0
 for i in range(256):
  f,p=struct.unpack('<IB',r(0x975410+i*16,5))
  if not 0x401000<=f<0x7db000:break
  prio[i]=p;count+=1
 assert count==77 and [i for i in range(count) if prio[i]==4]==[3,7,11,15,19,61,64,72,76]
 apply=bytearray(256)
 for i in range(count):apply[i]=r(0x975410+i*16+5,1)[0]
 assert struct.unpack('<I',r(0x9759c8,4))[0]==0x9758e0
 adjf=bytearray(256);adjn=0
 for i in range(256):
  f,fn=struct.unpack('<II',r(0x9758e0+i*8,8))
  if not 0x401000<=fn<0x7db000:break
  adjf[i]=1 if f else 0;adjn+=1
 assert adjn==28 and [i for i in range(adjn) if adjf[i]]==[2,4] and struct.unpack('<I',r(0x9758e4,4))[0]==0x467d80
 assert r(0x915440,16)==bytes(16)
 assert struct.unpack('<5f',bytes(r(0x7df1fc,4))+r(0x7dffd8,4)+r(0x7dd568,4)+r(0x7dd58c,4)+r(0x7dfff0,4))==(7.,7.5,struct.unpack('<f',struct.pack('<f',.1))[0],struct.unpack('<f',struct.pack('<f',.2))[0],struct.unpack('<f',struct.pack('<f',.9))[0])
 assert r(0x909fb0,1)==b'\x01' #40EF60 manual camera default on #gate plane global is zero (BSS), see cam_trigger.h
 names={f'CAM010{a}S.BIN' for a in (0,1,2)}
 src={s.name:s for s in list_cam_sources(system,names)}
 chunks=[('CAMPRIO',bytes(prio),0),('CAMMODE',bytes(apply),0),('CAMADJF',bytes(adjf),0)];rows={}
 for a in (0,1,2):
  s=src[f'CAM010{a}S.BIN'];chunks.append((f'CAMACT{a}',b''.join(x.raw for x in s.records),0));rows[a]=len(s.records)
 meta={'source_exe_sha256':hashlib.sha256(Path(exe).read_bytes()).hexdigest(),'rows':rows,'modes':count,
  'scope':'Sonic CAM010xS rows (64-byte records) + mode priority bytes; camera code is native'}
 data=sa3dspak.build([('METADATA',json.dumps(meta).encode(),0)]+chunks);sa3dspak.parse(data);return data,rows
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 data,rows=build(a.exe,a.system);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'EC camera owner package: PASS 22 code spans,77 modes+apply,28 adjusts (flags 2/4, adjust0 no-op) (prio4 3/7/11/15/19/61/64/72/76),zero gate plane,rows {rows}; {len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
