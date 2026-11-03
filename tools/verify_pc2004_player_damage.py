#!/usr/bin/env python3
"""Static owner witnesses for player damage; no executable bytes are emitted."""
from pathlib import Path
import hashlib, struct, sys
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools.verify_pc2004_ec_route import verified_reader
# Focused code spans, not a claim that each span is one complete function.
RANGES={
 (0x492d70,512):'b6572900b18bb3bc1a6718134efbb9345d895adb0683c6bf0e53540b12663afa',
 (0x496f84,147):'82e37f67e922dfc8fe1452b38170b078aa96a8ec492930201b4f22110d0f86b6',
 (0x4506f0,277):'bab07c4b3124c5d71dc3ebb4ddbf1786fe3c991b3aa5da8daeb4e38d28d2302e',
 (0x440cd0,128):'f194c1dd8beb6ef7d465d678a6c5ebaea6724783bedd5b4f66680257767bc77a',
 (0x4949c7,10):'c923b3362d5c89d2735f8ebc68e854e6b5e1e232db08428dd2778b179ca8fbb0',
 (0x49b072,96):'f1c3282e0e8edaf33ae352fe0d4c864d58af640d11f6b210444e247f64ac41b3',
 (0x497e3f,304):'c336973904e4f73ad58398a31393d3d87c9796a61fa106b41860381e40ab69a5',
}
def call(read,address,target):
    assert read(address,1)==b'\xe8',hex(address)
    assert address+5+struct.unpack('<i',read(address+1,4))[0]==target,hex(address)
def verify(exe):
    r=verified_reader(exe)
    for (a,n),digest in RANGES.items():assert hashlib.sha256(r(a,n)).hexdigest()==digest,hex(a)
    assert struct.unpack('<I',r(0x49a850+15*4,4))[0]==0x497e3f
    for a,t in [(0x496fae,0x492d70),(0x496ffe,0x4506f0),(0x492da6,0x4bcc10),
                (0x492f24,0x440740),(0x4506f2,0x6443bf),(0x45074a,0x40b860)]:call(r,a,t)
    assert r(0x492da0,6)==bytes.fromhex('66c746067800') #timer120
    assert r(0x492f29,6)==bytes.fromhex('d987c4000000') #forward=-run_speed
    assert r(0x492f2f,6)==bytes.fromhex('8b97b4000000') #normal=jmp_y_spd
    assert r(0x4949c7,4)==bytes.fromhex('f6470602') #hurt timer bit1
    assert r(0x4507c0,2)==bytes.fromhex('f7ff') #integer per-ring quotient
    for a,f,v in [(0x7dd53c,'f',1/32768),(0x7e00d8,'f',360),
                  (0x7dccd0,'d',65536),(0x7df1a0,'d',1/360)]:
        assert struct.unpack('<'+f,r(a,struct.calcsize(f)))[0]==v,hex(a)
    print('Player damage owner witnesses: PASS exact SHA,7 code spans,6 calls,timer/recoil/blink/integer burst and float constants; static evidence only, not retail execution')
if __name__=='__main__':
    if len(sys.argv)!=2:raise SystemExit('usage: verify_pc2004_player_damage.py OWNER_SONIC_EXE')
    verify(Path(sys.argv[1]))
