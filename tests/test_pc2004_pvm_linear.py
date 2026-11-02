#!/usr/bin/env python3
"""Source-neutral format9 spatial-layout tests; not merely decoder self-consistency."""
from pathlib import Path
import struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.importers.pc2004_pvm import SonicPVM

def archive(width,height,pixel_format,mips=False):
    rgba=bytearray();pixels=bytearray()
    for y in range(height):
        for x in range(width):
            if pixel_format==0:
                a=(x+y)&1;r=x%32;g=(x+y)%32;b=y%32
                value=(a<<15)|(r<<10)|(g<<5)|b;color=(r*255//31,g*255//31,b*255//31,a*255)
            elif pixel_format==1:
                r=x%32;g=(x+y)%64;b=y%32
                value=(r<<11)|(g<<5)|b;color=(r*255//31,g*255//63,b*255//31,255)
            else:
                a=(2*x+y)%16;r=x%16;g=(x+y)%16;b=y%16
                value=(a<<12)|(r<<8)|(g<<4)|b;color=(r*17,g*17,b*17,a*17)
            pixels.extend(struct.pack('<H',value));rgba.extend(color)
    if mips:
        assert width==height
        smaller=2*((width*height-1)//3)
        pixels+=bytes([0xee])*smaller+bytes(6)
    header=bytearray(64);struct.pack_into('<4sIHH',header,0,b'PVMH',56,0xf,1)
    dim=((height.bit_length()-3)<<4)|(width.bit_length()-3)
    struct.pack_into('<H28sBBHI',header,12,0,b'linear_fixture',pixel_format,9,dim,123)
    texture=struct.pack('<4sIBBHHH',b'PVRT',8+len(pixels),pixel_format,9,0,width,height)+pixels
    return bytes(header)+texture,bytes(rgba)

def main():
    cases=0
    for pf in range(3):
        for width,height,mips in [(8,8,False),(8,8,True),(8,16,False)]:
            blob,expected=archive(width,height,pf,mips)
            generic=PVMArchive(blob,'synthetic format9')
            actual=generic.decode_base_rgba(0)
            assert actual==expected,(pf,width,height,mips,'generic format9 must be row-major')
            # Exercise the shared-purpose Sonic decode method on neutral data;
            # production SonicPVM constructor remains exact-owner fingerprinted.
            sonic=SonicPVM.__new__(SonicPVM);sonic.data=blob;sonic.entries=generic.entries
            assert sonic.decode_base_rgba(0)==expected,(pf,width,height,mips,'Sonic format9 must be row-major')
            cases+=1
    print(f'PC2004 format9 linear: PASS ({cases} independent RGB/alpha grids,rectangular and base-first mip cases; generic+Sonic)')
if __name__=='__main__':main()
