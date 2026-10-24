#!/usr/bin/env python3
"""Verify the exact PC2004 directional-input filter without extracting data."""
from __future__ import annotations
import argparse, hashlib, json, struct
from pathlib import Path

SHA="6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74"
SIZE=55_549_952
SIGS={
 "filter_entry":(0x0040F170,"83ec085355565733ed33ffb3018d4900"),
 "raw_axes_signed16_shift8":(0x0040F18A,"b0030fbf481c0fbf501ec1e108894c2414c1e208db44241489542414"),
 "component_deadzone":(0x0040F203,"d9442410d81df4cc7d00d9442410dfe0f6c4417508"),
 "cubic_and_clamp":(0x0040F2B0,"8bf0d9c0d8c9d9c2d8cbdec1d95c24148b442414ddd850ddd8e8227f3700"),
 "filtered_outputs":(0x0040F2ED,"a0b09f9000d914fd6ce3b00381e6ffff00003ac38934fd68e3b003"),
}

def digest(p):
 h=hashlib.sha256()
 with p.open("rb") as f:
  for b in iter(lambda:f.read(1024*1024),b""): h.update(b)
 return h.hexdigest()
def pe(data):
 peoff=struct.unpack_from("<I",data,0x3c)[0]; assert data[peoff:peoff+4]==b"PE\0\0"
 coff=peoff+4; _,n,_,_,_,opt_size,_=struct.unpack_from("<HHIIIHH",data,coff)
 opt=coff+20; assert struct.unpack_from("<H",data,opt)[0]==0x10b
 base=struct.unpack_from("<I",data,opt+28)[0]; sec=[]; so=opt+opt_size
 for i in range(n):
  o=so+i*40; vs,rva,rs,rp=struct.unpack_from("<IIII",data,o+8); sec.append((rva,vs,rp,rs))
 return base,sec
def off(va,base,secs):
 rva=va-base
 for srva,vs,rp,rs in secs:
  if srva<=rva<srva+rs:return rp+rva-srva
 raise ValueError(f"unbacked VA {va:#x}")
def main():
 ap=argparse.ArgumentParser();ap.add_argument("exe",type=Path);ap.add_argument("--json",action="store_true");a=ap.parse_args()
 if a.exe.stat().st_size!=SIZE or digest(a.exe)!=SHA: raise SystemExit("unsupported PC2004 executable identity")
 data=a.exe.read_bytes();base,secs=pe(data)
 for name,(va,hx) in SIGS.items():
  want=bytes.fromhex(hx); got=data[off(va,base,secs):off(va,base,secs)+len(want)]
  if got!=want: raise SystemExit(f"{name} mismatch at {va:#x}")
 dead=struct.unpack_from("<f",data,off(0x007DCCF4,base,secs))[0]
 cubic=struct.unpack_from("<f",data,off(0x007DCCE0,base,secs))[0]
 expected=1.0/(29441.0**3)
 if dead!=3072.0 or abs(cubic-expected)>1e-20: raise SystemExit("input constants mismatch")
 out={"sha256":SHA,"input_filter_va":"0x0040f170","component_deadzone":dead,
      "cubic_scale":cubic,"logical_positive_axis":127,"logical_negative_axis":128,
      "old_0x004c3fc0_lead":"rejected for this executable; address is inside FUN_004c3f90"}
 print(json.dumps(out,indent=2,sort_keys=True) if a.json else
       "PC2004 input filter: PASS (0x0040F170, deadzone 3072, cubic radial response)")
 return 0
if __name__=="__main__":raise SystemExit(main())
