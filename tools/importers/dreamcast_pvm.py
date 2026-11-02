"""Bounded Dreamcast PVM/PRS decoding for owner-side EC texture overrides.

The installed Dreamcast Conversion archive uses PVM flags0x0109 (index/name/GBIX
rows) and original Dreamcast smallest-first mip chains. This deliberately does
not reuse the PC2004 format9 base-first decoder. Only uncompressed square
PVRT formats1/2 and 16-bit RGB565/ARGB1555/ARGB4444 are decoded. VQ/indexed data
fail explicitly. References: PuyoTools PVR-Texture/PVM-Archive wiki and actual
owner archive metadata; no game pixels or compressed data are shipped here.
"""
from dataclasses import dataclass
import hashlib
import struct
from .pc2004_pvm_archive import _pixel_rgba, _morton2

MAX_DECOMPRESSED=8*1024*1024

def decompress_prs(source:bytes,limit:int=MAX_DECOMPRESSED)->bytes:
    pos=bits=left=0
    out=bytearray()
    def byte():
        nonlocal pos
        if pos>=len(source):raise ValueError('truncated PRS')
        value=source[pos];pos+=1;return value
    def bit():
        nonlocal bits,left
        if not left:bits=byte();left=8
        value=bits&1;bits>>=1;left-=1;return value
    while True:
        if bit():
            out.append(byte())
        else:
            if bit():
                value=byte();value|=byte()<<8
                offset=value>>3
                if not offset:
                    if pos!=len(source):raise ValueError('PRS trailing bytes')
                    return bytes(out)
                offset-=8192
                length=value&7
                length=byte()+1 if not length else length+2
            else:
                length=bit()<<1;length=(length|bit())+2;offset=byte()-256
            if -offset>len(out) or offset>=0:raise ValueError('PRS back-reference before output')
            if len(out)+length>limit:raise ValueError('PRS output limit')
            for _ in range(length):out.append(out[offset])
        if len(out)>limit:raise ValueError('PRS output limit')

@dataclass(frozen=True)
class DCTexture:
    index:int;name:str;global_index:int
    pixel_format:int;data_format:int;width:int;height:int
    payload_offset:int;payload_size:int

class DreamcastPVM:
    def __init__(self,data:bytes):
        self.data=bytes(data)
        if len(data)<12 or data[:4]!=b'PVMH':raise ValueError('missing DC PVMH')
        payload,flags,count=struct.unpack_from('<IHH',data,4)
        if flags!=0x0109 or not 0<count<=4096:raise ValueError('unsupported DC PVM directory flags/count')
        end=payload+8;directory_end=12+count*34
        if directory_end>end or end>len(data) or end%16 or any(data[directory_end:end]):
            raise ValueError('invalid DC PVM directory span/padding')
        entries=[];offset=end
        for i in range(count):
            row=12+i*34;index=struct.unpack_from('<H',data,row)[0]
            if index!=i:raise ValueError('DC PVM index discontinuity')
            name=data[row+2:row+30].split(b'\0',1)[0].decode('ascii','strict')
            global_index=struct.unpack_from('<I',data,row+30)[0]
            if offset+16>len(data) or data[offset:offset+4]!=b'PVRT':raise ValueError('missing DC PVRT')
            size=struct.unpack_from('<I',data,offset+4)[0]
            if size<8 or offset+8+size>len(data):raise ValueError('DC PVRT bounds')
            pf,df,reserved,w,h=struct.unpack_from('<BBHHH',data,offset+8)
            if reserved or w<1 or h<1 or w>1024 or h>1024:raise ValueError('DC PVRT header')
            entries.append(DCTexture(i,name,global_index,pf,df,w,h,offset+16,size-8))
            offset+=8+size
        if offset!=len(data):raise ValueError('DC PVM trailing bytes')
        self.entries=tuple(entries)
    def sha256(self):return hashlib.sha256(self.data).hexdigest()
    def named(self,name):
        matches=[e for e in self.entries if e.name.casefold()==name.casefold()]
        if len(matches)!=1:raise ValueError('DC texture name missing/ambiguous')
        return matches[0]
    def decode_base_rgba(self,index):
        e=self.entries[index]
        if e.data_format not in (1,2) or e.pixel_format not in (0,1,2):
            raise ValueError('unsupported DC texture encoding (only uncompressed twiddled1/2)')
        if e.width!=e.height or e.width&(e.width-1):raise ValueError('DC twiddled texture must be square power-of-two')
        # Format2 begins with2 padding bytes, followed by1x1..base mip levels.
        prefix=2+2*((e.width*e.height-1)//3) if e.data_format==2 else 0
        needed=prefix+e.width*e.height*2
        padding=e.payload_size-needed
        if padding<0 or padding>=32 or any(self.data[e.payload_offset+needed:e.payload_offset+e.payload_size]):
            raise ValueError('DC mip payload size/padding')
        if e.data_format==2 and any(self.data[e.payload_offset:e.payload_offset+2]):
            raise ValueError('DC mip prefix')
        base=e.payload_offset+prefix;out=bytearray(e.width*e.height*4)
        for y in range(e.height):
            for x in range(e.width):
                value=struct.unpack_from('<H',self.data,base+2*_morton2(y,x))[0]
                o=(y*e.width+x)*4;out[o:o+4]=bytes(_pixel_rgba(value,e.pixel_format))
        return bytes(out)
