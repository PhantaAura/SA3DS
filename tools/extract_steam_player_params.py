#!/usr/bin/env python3
import argparse, hashlib, json, struct, sys
from pathlib import Path

EXPECTED_SIZE = 90_922_000
EXPECTED_SHA256 = "e4330c00d7ee3910ecc1acab789ffd2a2d571c506984b99a8de4d3663e697fd6"
TABLE_VA = 0x025D9B90
RECORD_SIZE = 0x84
COUNT = 8
CHARACTERS = ["Sonic", "Eggman", "Tails", "Knuckles", "Tikal", "Amy", "Gamma", "Big"]

FIELDS = [
    "jump2_timer","pos_error","lim_h_spd","lim_v_spd","max_x_spd","max_psh_spd",
    "jmp_y_spd","nocon_speed","slide_speed","jog_speed","run_speed","rush_speed",
    "crash_speed","dash_speed","jmp_addit","run_accel","air_accel","slow_down",
    "run_break","air_break","air_resist_air","air_resist","air_resist_y",
    "air_resist_z","grd_frict","grd_frict_z","lim_frict","rat_bound","rad",
    "height","weight","eyes_height","center_height"
]

def sha256(path):
    h=hashlib.sha256()
    with path.open("rb") as f:
        for b in iter(lambda:f.read(1024*1024), b""):
            h.update(b)
    return h.hexdigest()

def parse_sections(data):
    if data[:2] != b"MZ": raise ValueError("not MZ")
    peoff=struct.unpack_from("<I", data, 0x3c)[0]
    if data[peoff:peoff+4] != b"PE\0\0": raise ValueError("not PE")
    coff=peoff+4
    _,nsec,_,_,_,opt_size,_=struct.unpack_from("<HHIIIHH",data,coff)
    opt=coff+20
    if struct.unpack_from("<H",data,opt)[0] != 0x10b: raise ValueError("not PE32")
    image_base=struct.unpack_from("<I",data,opt+28)[0]
    out=[]
    so=opt+opt_size
    for i in range(nsec):
        o=so+i*40
        name=data[o:o+8].split(b"\0",1)[0].decode("ascii","replace")
        vsize,rva,rsize,rptr=struct.unpack_from("<IIII",data,o+8)
        out.append((name,rva,vsize,rptr,rsize))
    return image_base,out

def va_to_off(va,image_base,sections):
    rva=va-image_base
    for name,srva,vsize,rptr,rsize in sections:
        if srva <= rva < srva + max(vsize,rsize):
            return rptr+(rva-srva),name
    raise ValueError(f"VA {va:#x} unmapped")

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("exe",type=Path)
    ap.add_argument("-o","--output",type=Path)
    args=ap.parse_args()
    size=args.exe.stat().st_size
    digest=sha256(args.exe)
    if size != EXPECTED_SIZE or digest != EXPECTED_SHA256:
        print(f"unsupported EXE: size={size} sha256={digest}",file=sys.stderr)
        return 2

    data=args.exe.read_bytes()
    image_base,sections=parse_sections(data)
    off,section=va_to_off(TABLE_VA,image_base,sections)

    # Strong structural validation from Steam initializer at 0x004DAA2F..0x004DAA4F:
    # CharID byte -> *0x84 -> +TABLE_VA -> rep movsd 0x21 dwords (=0x84 bytes).
    init_va=0x004DAA2F
    init_off,_=va_to_off(init_va,image_base,sections)
    expected=bytes.fromhex("0fb6710969f68400000081c6909b5d02")
    if data[init_off:init_off+len(expected)] != expected:
        raise ValueError("Steam player parameter initializer signature mismatch")
    copy_va=0x004DAA48
    copy_off,_=va_to_off(copy_va,image_base,sections)
    copy_sig=bytes.fromhex("b921000000f3a5")
    if data[copy_off:copy_off+len(copy_sig)] != copy_sig:
        raise ValueError("0x84-byte copy signature mismatch")

    records=[]
    for i,name in enumerate(CHARACTERS):
        roff=off+i*RECORD_SIZE
        values=[struct.unpack_from("<i",data,roff)[0]]
        values.extend(struct.unpack_from("<32f",data,roff+4))
        records.append({"index":i,"character":name,**dict(zip(FIELDS,values))})

    result={
        "schema":"sa3ds-steam-player-params-v1",
        "exe_size":size,
        "exe_sha256":digest,
        "table_va":TABLE_VA,
        "table_file_offset":off,
        "table_section":section,
        "record_size":RECORD_SIZE,
        "record_count":COUNT,
        "initializer_va":0x004DAA00,
        "records":records,
    }
    text=json.dumps(result,indent=2)
    if args.output: args.output.write_text(text+"\n")
    else: print(text)
    return 0

if __name__=="__main__":
    raise SystemExit(main())
