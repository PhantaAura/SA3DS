#!/usr/bin/env python3
"""Owner-only original whale collision-tip pitch data, never embedded in Git."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.verify_pc2004_ec_route import verified_reader,PROFILE_SHA256
SPANS={(0x5022c0,1840):'5f1f0c8dc4185cb9a49208854f1030216d53d5e0e0a1004cc90e37a84ac41cd3',
       (0x784e20,191):'f101608bb9ce1ab9c1e245d7c59304a4becf13d4e774e2974aff6944298e3757',
       (0x784ee0,193):'a3814f3dad96a45e0739766739929ea8c460b92056baa814ff402d444bbcb427',
       (0x10ad238,512):'ecc608f9cacc42cefb3ad7ae2e1e47f9e9df4096341c8b70d5191a68e1efbae2'}
def build(exe):
 r=verified_reader(exe)
 for (a,n),sha in SPANS.items():assert hashlib.sha256(r(a,n)).hexdigest()==sha,hex(a)
 rows=list(struct.iter_unpack('<4i',r(0x10ad238,512)))
 assert all(row[0]==i and row[2:]==(0,0) and -32768<=row[1]<=32767 for i,row in enumerate(rows))
 payload=struct.pack('<8s6I32i',b'SA3WTIP\0',1,32,32,4,160,0,*(row[1] for row in rows))
 metadata=json.dumps({'source_exe_sha256':PROFILE_SHA256,'purpose':'original whale collision-tip pitch; not visible body shape animation'},sort_keys=True).encode()
 return sa3dspak.build([('METADATA',metadata,0),('WHALE_TIP',payload,0)])
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('output',type=Path);a=ap.parse_args()
 if a.output.resolve().is_relative_to(ROOT):raise ValueError('owner packages must stay outside source tree')
 data=build(a.exe);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(data)
 print(f'Whale tip owner4 spans/32 indexed pitch keys PASS; private package{len(data)}B SHA256 {hashlib.sha256(data).hexdigest()}')
