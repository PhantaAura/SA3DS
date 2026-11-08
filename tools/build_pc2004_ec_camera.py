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
 assert r(0x915440,16)==bytes(16) #gate plane global is zero (BSS), see cam_trigger.h
 names={f'CAM010{a}S.BIN' for a in (0,1,2)}
 src={s.name:s for s in list_cam_sources(system,names)}
 chunks=[('CAMPRIO',bytes(prio),0)];rows={}
 for a in (0,1,2):
  s=src[f'CAM010{a}S.BIN'];chunks.append((f'CAMACT{a}',b''.join(x.raw for x in s.records),0));rows[a]=len(s.records)
 meta={'source_exe_sha256':hashlib.sha256(Path(exe).read_bytes()).hexdigest(),'rows':rows,'modes':count,
  'scope':'Sonic CAM010xS rows (64-byte records) + mode priority bytes; camera code is native'}
 data=sa3dspak.build([('METADATA',json.dumps(meta).encode(),0)]+chunks);sa3dspak.parse(data);return data,rows
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner output must remain outside source tree')
 data,rows=build(a.exe,a.system);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'EC camera owner package: PASS 6 code spans,77 modes (prio4 3/7/11/15/19/61/64/72/76),zero gate plane,rows {rows}; {len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
