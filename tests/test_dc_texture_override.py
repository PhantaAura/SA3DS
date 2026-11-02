#!/usr/bin/env python3
"""Owner-free PRS, original Dreamcast mip/axis and override-format tests."""
from pathlib import Path
import hashlib,struct,sys,tempfile,zlib
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools import sa3t
from tools.importers.dreamcast_pvm import DreamcastPVM,decompress_prs
from tools.build_dc_ec_texture_override import build_override

def expect_error(fn):
    try:fn()
    except ValueError:return
    raise AssertionError('malformed input accepted')

def fixture():
    size=8;prefix=2+2*((size*size-1)//3)
    payload=bytearray(prefix+size*size*2);spread=[0,1,4,5,16,17,20,21]
    for y in range(size):
        for x in range(size):struct.pack_into('<H',payload,prefix+2*((spread[x]<<1)|spread[y]),(x<<11)|y)
    payload+=bytes((-16-len(payload))%32)
    header=bytearray(48);struct.pack_into('<4sIHH',header,0,b'PVMH',40,0x109,1)
    struct.pack_into('<H28sI',header,12,0,b'synthetic',123)
    return bytes(header)+struct.pack('<4sIBBHHH',b'PVRT',8+len(payload),1,2,0,size,size)+payload

def tests():
    assert decompress_prs(bytes([0x17])+b'ABC'+bytes(2))==b'ABC'
    assert decompress_prs(bytes([0x83])+b'AB'+bytes([0xfe,0,0]))==b'ABAB'
    assert decompress_prs(bytes([0x59])+b'A'+bytes([0xff,0,0]))==b'AAAAAA'
    expect_error(lambda:decompress_prs(b''))
    expect_error(lambda:decompress_prs(bytes([0x83])+b'AB'+bytes([0xf0,0,0])))
    expect_error(lambda:decompress_prs(bytes([0x17])+b'ABC'+bytes(2),2))
    expect_error(lambda:decompress_prs(bytes([0x17])+b'ABC'+bytes(3)))
    image=DreamcastPVM(fixture());entry=image.named('SYNTHETIC');assert entry.index==0
    rgba=image.decode_base_rgba(0)
    for y in range(8):
        for x in range(8):assert rgba[(y*8+x)*4:(y*8+x)*4+4]==bytes((x*255//31,0,y*255//31,255))
    damaged=bytearray(fixture());damaged[-1]=1
    expect_error(lambda:DreamcastPVM(damaged).decode_base_rgba(0))
    damaged=bytearray(fixture());damaged[57]=4
    expect_error(lambda:DreamcastPVM(damaged).decode_base_rgba(0))
    print('Dreamcast texture importer: PASS (PRS literal/overlap/bounds, smallest-first mip base, original X-high Morton axes, padding/encoding rejection)')

def fixtures(out):
    out.mkdir(parents=True,exist_ok=True)
    sha=hashlib.sha256(b'original owner-free archive').hexdigest()
    def tex(n,color):return sa3t.build([dict(width=n,height=n,rgba=bytes(color)*(n*n),source_pixel_format=1,source_data_format=2,global_index=123)],hashlib.sha256(b'replacement').hexdigest())
    original=sa3t.build([dict(width=8,height=8,rgba=bytes((1,2,3,255))*64,source_pixel_format=1,source_data_format=9,global_index=1)],sha)
    (out/'baseline.sa3t').write_bytes(original)
    replacement=tex(8,(4,5,6,255))
    valid=build_override(replacement,sha,0,0)
    (out/'valid.sa3mod').write_bytes(valid)
    (out/'wrong_act.sa3mod').write_bytes(build_override(replacement,sha,1,0))
    (out/'wrong_hash.sa3mod').write_bytes(build_override(replacement,'ab'*32,0,0))
    (out/'wrong_target.sa3mod').write_bytes(build_override(replacement,sha,0,1))
    (out/'wrong_dimensions.sa3mod').write_bytes(build_override(tex(16,(4,5,6,255)),sha,0,0))
    damaged=bytearray(valid);damaged[-1]^=1;(out/'corrupt.sa3mod').write_bytes(damaged)
    (out/'truncated.sa3mod').write_bytes(valid[:-1])
    damaged=bytearray(valid);struct.pack_into('<I',damaged,12,63);struct.pack_into('<I',damaged,28,0);struct.pack_into('<I',damaged,28,zlib.crc32(damaged));(out/'bad_header.sa3mod').write_bytes(damaged)
    (out/'too_large.sa3mod').write_bytes(bytes(262305))
if __name__=='__main__':
    tests()
    if len(sys.argv)==2:fixtures(Path(sys.argv[1]))
