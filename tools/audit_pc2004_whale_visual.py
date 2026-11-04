#!/usr/bin/env python3
"""Check runtime UV-seam mapping against raw owner Basic polygon streams."""
from pathlib import Path
import argparse,struct,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
from tools import sa3dspak
from tools.build_pc2004_whale_visual import build
from tools.verify_pc2004_ec_route import verified_reader
from tools.importers.pc2004_stage import PC2004StageDecoder
from tools.importers.pc2004_pvm_archive import PVMArchive
from tools.build_pc2004_spring_visual import Adapter
from tools.build_pc2004_emerald_coast_visual import build_visual_chunk,read_source
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def audit(exe,system):
 data=build(exe,system);r=verified_reader(exe);chunks=sa3dspak.parse(data)['chunks'];mapping=next(c['data'] for c in chunks if c['type']=='WHALE_MAP')
 with tempfile.TemporaryDirectory(prefix='sa3ds-owner-whale-visual-') as directory:
  root=Path(directory);good=root/'visual.sa3dspak';good.write_bytes(data);out=root/'view.bin';bad=[]
  for name,offset,value in [('version',8,2),('sourcecount',12,237),('vertexcount',16,0),('offset',20,28),('stride',24,4),('size',28,len(mapping)-2),('sourceindex',32,238)]:
   m=bytearray(mapping)
   if offset==32:struct.pack_into('<H',m,offset,value)
   else:struct.pack_into('<I',m,offset,value)
   packet=sa3dspak.build([(c['type'],bytes(m) if c['type']=='WHALE_MAP' else c['data'],c['flags']) for c in chunks]);p=root/(name+'.sa3dspak');p.write_bytes(packet);bad.append(p)
  output=subprocess.check_output(['sh',str(ROOT/'tests/run_whale_visual.sh'),str(good),str(out),*map(str,bad)],text=True);assert 'seam-preserving' in output
  view=out.read_bytes();nv,ni,nm,nmat=struct.unpack_from('<4I',view);cursor=16;vertices=[]
  points,normals,count,meshes,materials=struct.unpack('<5I',r(0x106bb20,20));assert count==238
  identities={}
  for i in range(nv):
   index=struct.unpack_from('<H',view,cursor)[0];raw=view[cursor+2:cursor+34];cursor+=34
   assert raw[:12]==r(points+index*12,12) and raw[12:24]==r(normals+index*12,12)
   uv=struct.unpack_from('<2f',raw,24);vertices.append((index,*uv));identities.setdefault(index,set()).add(uv)
  indices=struct.unpack_from('<'+'H'*ni,view,cursor);cursor+=ni*2
  meshrows=[struct.unpack_from('<3I',view,cursor+i*12) for i in range(nm)];cursor+=nm*12
  assert nm==nmat==5
  original_materials=[struct.unpack('<IIfII',r(materials+i*20,20)) for i in range(5)];textures=sorted({m[3] for m in original_materials});assert textures==[27,28,29,30]
  for i,mat in enumerate(original_materials):
   actual=struct.unpack_from('<IIfII',view,cursor+i*20);assert actual==(*mat[:3],textures.index(mat[3]),mat[4])
  cursor+=nmat*20;assert cursor==len(view)
  corners=0
  for mesh,(material,first,nindex) in enumerate(meshrows):
   typ,npoly,poly,_,_,_,uvptr,_=struct.unpack('<HH6I',r(meshes+mesh*28,28));assert material==typ&0x3fff
   streams=[];p=poly
   for _ in range(npoly):
    if typ&0xc000==0xc000:
     word=struct.unpack('<H',r(p,2))[0];p+=2;n=word&0x7fff;rev=bool(word&0x8000)
    else:assert typ&0xc000==0x4000;n=4;rev=False
    stream=struct.unpack('<'+'H'*n,r(p,n*2));p+=n*2;streams.append((rev,stream))
   expected=[];u=0
   for rev,stream in streams:
    source=[]
    for index in stream:
     rawu,rawv=struct.unpack('<2h',r(uvptr+u*4,4));u+=1;source.append((index,f32(rawu/255),f32(rawv/255)))
    if typ&0xc000==0xc000:
     for triangle in range(len(source)-2):
      a,b,c=source[triangle:triangle+3];expected.extend((b,a,c) if bool(triangle&1)^rev else (a,b,c))
    else:expected.extend(source[j] for j in (0,1,2,2,1,3))
   actual=[vertices[i] for i in indices[first:first+nindex]];assert actual==expected;corners+=len(expected)
  #Optional identity collection cannot change existing callers' geometry bytes.
  d=PC2004StageDecoder(exe);pvm=PVMArchive(read_source(system,'OBJ_BEACH.PVM'),'OBJ_BEACH.PVM')
  without=build_visual_chunk(Adapter(d,0x106bb20),0,pvm.texture_count);collected=[]
  with_map=build_visual_chunk(Adapter(d,0x106bb20),0,pvm.texture_count,collected);assert without==with_map
  seams=sum(len(uvs)>1 for uvs in identities.values());assert seams>0 and len(collected)==nv
 print(f'Whale visual owner oracle: PASS{nv} mapped vertices/{corners} ordered triangle corners/{seams} source vertices with UV seams,5 exact materials/4 source textures,both-bank runtime scatter,7 malformed maps; default geometry output unchanged')
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('exe',type=Path);ap.add_argument('system',type=Path);a=ap.parse_args();audit(a.exe,a.system)
