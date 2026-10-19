#!/usr/bin/env python3
"""Generate the 0.0.9.0E RomFS packages from the owner's exact PC2004 files.

This script intentionally writes Sega-derived data only into runtime_0_0_9_0E/
romfs for a local build. Those generated files are excluded from code checkpoint
archives.
"""
from __future__ import annotations
import argparse, hashlib, json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
import sys
sys.path.insert(0,str(ROOT))
from tools.build_pc2004_sonic_sa3c import build_package as build_sa3c
from tools.build_pc2004_sonic_sa3t import build_package as build_sa3t

CHRMODELS_SHA="76bbaa1b6aa23872648e6f1f39642e4262403e372b4c4b32f20f0ec89780c2a5"
SONIC_PVM_SHA="f8e30dfbebeb901a0ac210a61929c45fbcefcc1a2758cba400f959989b93ecbe"

def sha256(path:Path)->str:
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
    return h.hexdigest()

def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument('chrmodels',type=Path,help='exact original PC2004 system/CHRMODELS.DLL')
    ap.add_argument('sonic_pvm',type=Path,help='exact PC2004 system/SONIC.PVM')
    ap.add_argument('--runtime',type=Path,default=ROOT/'runtime_0_0_9_0E')
    a=ap.parse_args()
    got_c=sha256(a.chrmodels); got_p=sha256(a.sonic_pvm)
    if got_c!=CHRMODELS_SHA: raise SystemExit(f'CHRMODELS.DLL hash mismatch\n expected {CHRMODELS_SHA}\n got      {got_c}')
    if got_p!=SONIC_PVM_SHA: raise SystemExit(f'SONIC.PVM hash mismatch\n expected {SONIC_PVM_SHA}\n got      {got_p}')
    outdir=a.runtime/'romfs'/'sa3ds';outdir.mkdir(parents=True,exist_ok=True)
    c_pkg,c_rep=build_sa3c(a.chrmodels,a.sonic_pvm)
    t_pkg,t_rep=build_sa3t(a.sonic_pvm)
    (outdir/'sonic.sa3c').write_bytes(c_pkg)
    (outdir/'sonic.sa3t').write_bytes(t_pkg)
    report={'schema':'sa3ds-0.0.9.0E-owner-romfs-v1','chrmodels_sha256':got_c,'sonic_pvm_sha256':got_p,'sa3c':c_rep,'sa3t':t_rep}
    (outdir/'owner_asset_build_report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'output':str(outdir),'sa3c_bytes':len(c_pkg),'sa3t_bytes':len(t_pkg),'sa3c_sha256':hashlib.sha256(c_pkg).hexdigest(),'sa3t_sha256':hashlib.sha256(t_pkg).hexdigest()},indent=2))
    return 0
if __name__=='__main__': raise SystemExit(main())
