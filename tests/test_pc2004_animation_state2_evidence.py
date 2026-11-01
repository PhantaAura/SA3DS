#!/usr/bin/env python3
"""Owner-only exact-EXE witnesses for PlayAnimation state2 and loop phases."""
import hashlib
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
from verify_pc2004_collision_work import PROFILE_SHA256, parse_pe, va_to_off

SIGNATURES={
    # Row+0 -> ACTION; ACTION+4 -> motion; motion+4 -> full frame_count.
    # The loop bound has no frames-1 subtraction (unlike one-shot states).
    "fixed loop full motion frame_count":(0x44A962,"8b51048b4204db4204"),
    "signed-speed loop full motion frame_count":(0x44AA0F,"8b41048b5004db4004"),
    "scalar loop full motion frame_count":(0x44AABE,"8b41048b5004db4004"),
    "fixed loop bound compare":(0x44A975,"d8542418dfe0f6c441"),
    "fixed loop stored frame subtraction":(0x44A980,"d94610d8e1d95e10"),
    "fixed loop repeat compare":(0x44A99D,"d85610dfe0f6c4417bd9"),
    "signed-speed loop phase subtraction":(0x44AA30,"d94610d8e1d95e10"),
    "signed-speed loop negative phase addition":(0x44AA83,"d84610d95e10"),
    "scalar loop phase subtraction":(0x44AAE0,"d94610d8e1d95e10"),
    "scalar loop negative phase addition":(0x44AB33,"d84610d95e10"),
    "retained index into CX":(0x44A808,"668b4e06"),
    "current request into BX":(0x44A80C,"668b5e04"),
    "state2 bypasses mismatch reset":(0x44A828,"663d02007414"),
    "state2 current request row":(0x44A916,"8b4e200fb7c3c1e00403c8"),
    "state2 current row speed accumulation":(0x44A921,"d94108d84610d95610"),
    "state2 current row playback property":(0x44A93B,"660fbe5105668916"),
    "display selects request unless state2":(0x4949AE,"6683be2001000002550fb7ae2401000075070fb7ae26010000"),
    "forward start frame restore":(0x43F261,"8b4114894110c7411400000000"),
    "reverse start frame retained index":(0x43F270,"0fb751068b4120c1e2048b1402"),
}

def main(path):
    image=Path(path).read_bytes()
    assert hashlib.sha256(image).hexdigest()==PROFILE_SHA256,"unsupported owner executable"
    base,sections=parse_pe(image)
    for label,(va,hx) in SIGNATURES.items():
        off,_=va_to_off(va,base,sections)
        expected=bytes.fromhex(hx)
        assert image[off:off+len(expected)]==expected,label
    print("PC2004 state2 request/index + loop-phase instruction witnesses: PASS (exact owner EXE; no live retail trace)")

if __name__=="__main__":
    if len(sys.argv)!=2:raise SystemExit("usage: test_pc2004_animation_state2_evidence.py OWNER_SONIC_EXE")
    main(sys.argv[1])
