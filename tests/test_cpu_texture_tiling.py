#!/usr/bin/env python3
"""CPU-only synthetic 8x8 3DS tile preflight; no game assets / devkitARM.

Verify the C Morton bit mixer itself against a known reference tile order,
and guard that the live loader selects the software tile path.
"""
from pathlib import Path
import re, tempfile, subprocess, ctypes
src = Path(__file__).resolve().parents[1] / 'runtime_0_0_9_0G/source/sonic_gpu.c'
t = src.read_text()
m = re.search(r'static uint32_t morton8\(uint32_t x, uint32_t y\)\s*\{.*?\n\}', t, re.S)
assert m, 'morton8() missing'
assert 'upload_rgba8_cpu(&r->gpu_textures[i]' in t, 'runtime does not use CPU uploader'
assert 'C3D_TexFlush(tex);' in t and 'CPU swizzle DONE' in t
assert 'src_y=(uint32_t)height-1u-y' in t
assert 'dst[dest+0u]=rgba[source+3u]' in t and 'dst[dest+3u]=rgba[source+0u]' in t
assert 'linearAlloc(bytes)' in t, 'legacy path should remain available for diagnosis'
with tempfile.TemporaryDirectory() as d:
    p=Path(d)
    test_c=p/'test.c'
    test_c.write_text('#include <stdint.h>\n'+m.group(0)+'\nuint32_t test_morton(uint32_t x, uint32_t y){return morton8(x,y);}\n')
    so=p/'test.so'
    subprocess.run(['cc','-shared','-fPIC','-std=c99','-Wall','-Wextra','-Werror',str(test_c),'-o',str(so)],check=True)
    f=ctypes.CDLL(str(so)).test_morton
    f.argtypes=(ctypes.c_uint32,ctypes.c_uint32)
    f.restype=ctypes.c_uint32
    tile=[None]*64
    for y in range(8):
        for x in range(8):
            ofs=f(x,y)
            assert 0 <= ofs < 64 and tile[ofs] is None
            tile[ofs]=y*8+x
    assert tile[:16]==[0,1,8,9,2,3,10,11,16,17,24,25,18,19,26,27],tile[:16]
    assert sorted(tile)==list(range(64))
print('CPUUpload2 Morton tile + active uploader path: PASS')
