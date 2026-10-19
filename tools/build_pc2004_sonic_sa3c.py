#!/usr/bin/env python3
"""Build pointer-free SA3C v0 from exact owner-supplied PC2004 Sonic sources.

The generated .sa3c contains Sega-derived geometry/animation data.  Do not add
it to code-only SA3DS checkpoint archives.  Regenerate it locally from the
user's supported installation.
"""
from __future__ import annotations

from pathlib import Path
import argparse
import json
import math
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from tools.importers.pc2004_chrmodels import SonicDecoder, MESHSET_SIZE, MATERIAL_SIZE, KEY_SIZE
from tools.importers.pc2004_pvm import SonicPVM
from tools import sa3c

BAMS_TO_RAD=(2.0*math.pi)/65536.0


def parse_model_raw(dec:SonicDecoder,va:int):
    raw=dec.pe.read_bytes(va,44)
    return struct.unpack_from('<IIIIIHHffffi',raw,0)


def source_points(dec:SonicDecoder,va:int,count:int):
    raw=dec.pe.read_bytes(va,count*12)
    return [struct.unpack_from('<fff',raw,i*12) for i in range(count)]


def source_uvs(dec:SonicDecoder,va:int,count:int):
    raw=dec.pe.read_bytes(va,count*4)
    return [struct.unpack_from('<hh',raw,i*4) for i in range(count)]


def strip_stream(dec:SonicDecoder,va:int,count:int):
    off=dec.pe.va_to_offset(va); out=[]
    for _ in range(count):
        raw_count=struct.unpack_from('<H',dec.pe.data,off)[0]; off+=2
        n=raw_count&0x7fff
        inds=list(struct.unpack_from('<'+'H'*n,dec.pe.data,off)); off+=2*n
        out.append((bool(raw_count&0x8000),inds))
    return out


def quad_stream(dec:SonicDecoder,va:int,count:int):
    raw=dec.pe.read_bytes(va,count*4*2)
    inds=list(struct.unpack('<'+'H'*(count*4),raw))
    return [inds[i*4:(i+1)*4] for i in range(count)]


def triangulate_strip(corner_vertices, reverse: bool):
    """Convert a Ninja Basic strip to triangle-list order matching SA Tools."""
    out=[]
    for t in range(len(corner_vertices)-2):
        a,b,c=corner_vertices[t:t+3]
        flip=((t&1)!=0) ^ reverse
        out.extend((b,a,c) if flip else (a,b,c))
    return out


def triangulate_quad(q):
    """Convert one Ninja Basic quad to SA Tools MeshInfo.ToTriangles order."""
    if len(q)!=4:
        raise ValueError('quad must have exactly four corners')
    return [q[0],q[1],q[2],q[2],q[1],q[3]]


def read_vec_keys(dec:SonicDecoder,va:int,count:int):
    if not count:return []
    raw=dec.pe.read_bytes(va,count*16)
    return [struct.unpack_from('<Ifff',raw,i*16) for i in range(count)]


def read_ang_keys(dec:SonicDecoder,va:int,count:int):
    if not count:return []
    raw=dec.pe.read_bytes(va,count*16)
    out=[]
    for i in range(count):
        frame,x,y,z=struct.unpack_from('<Iiii',raw,i*16)
        out.append((frame,x*BAMS_TO_RAD,y*BAMS_TO_RAD,z*BAMS_TO_RAD))
    return out


def build_package(chrmodels:Path,pvm_path:Path):
    dec=SonicDecoder(chrmodels)
    pvm=SonicPVM(pvm_path)
    actions=dec.actions()

    # Global node/model sets are deterministic by source VA.  Source pointers
    # disappear from the package; they are used only while converting.
    root_vas=sorted({a.root_object_va for a in actions if a})
    node_by_va={}
    for root in root_vas:
        for obj in dec.hierarchy(root):
            node_by_va[obj.va]=obj
    node_vas=sorted(node_by_va)
    node_index={va:i for i,va in enumerate(node_vas)}
    if len(node_vas)>=sa3c.INVALID_U16:
        raise ValueError('SA3C v0 node count exceeds u16 index range')

    model_vas=sorted({o.model_va for o in node_by_va.values() if o.model_va})
    model_index={va:i for i,va in enumerate(model_vas)}
    if len(model_vas)>=sa3c.INVALID_U16:
        raise ValueError('SA3C v0 model count exceeds u16 index range')

    # Materials are deduplicated by exact source fields.  Texture IDs are local
    # SONIC.PVM indices for this exact source profile and are range-checked.
    material_keys=[]; material_index={}
    model_material_map={}
    for mv in model_vas:
        _p,_n,_np,_ms,matva,_nms,nmat,*_rest=parse_model_raw(dec,mv)
        row=[]
        for i in range(nmat):
            m=dec.material(matva+i*MATERIAL_SIZE)
            if not 0<=m.texture_id<pvm.texture_count:
                raise ValueError('material texture id outside SONIC.PVM')
            key=(m.diffuse,m.specular,m.exponent,m.texture_id,m.flags)
            if key not in material_index:
                material_index[key]=len(material_keys); material_keys.append(key)
            row.append(material_index[key])
        model_material_map[mv]=row

    vertices=[]; indices=[]; meshes=[]; model_rows=[]
    for mv in model_vas:
        points_va,normals_va,npts,meshsets_va,_materials_va,nmesh,_nmat,cx,cy,cz,radius,org=parse_model_raw(dec,mv)
        points=source_points(dec,points_va,npts)
        normals=source_points(dec,normals_va,npts)
        model_first_mesh=len(meshes)
        model_first_vertex=len(vertices)
        model_first_index=len(indices)
        # Deduplicate only within a source model by source point + raw UV.
        vmap={}
        for mi in range(nmesh):
            raw=dec.pe.read_bytes(meshsets_va+mi*MESHSET_SIZE,MESHSET_SIZE)
            type_material,poly_count=struct.unpack_from('<HH',raw,0)
            ptrs=struct.unpack_from('<IIIIII',raw,4)
            primitive=type_material&0xc000
            local_mat=type_material&0x3fff
            global_mat=model_material_map[mv][local_mat]
            mesh_first_index=len(indices)
            if primitive==0xc000:
                strips=strip_stream(dec,ptrs[0],poly_count)
                corner_count=sum(len(s[1]) for s in strips)
                uvs=source_uvs(dec,ptrs[4],corner_count)
                uv_cursor=0
                for reverse,strip in strips:
                    corner_vertices=[]
                    for src_idx in strip:
                        uv=uvs[uv_cursor]; uv_cursor+=1
                        key=(src_idx,uv[0],uv[1])
                        if key not in vmap:
                            # Preserve raw signed-16 UV coordinate numerically in
                            # float32.  Runtime UV scale is intentionally a later
                            # texture-semantics gate, so no lossy normalization.
                            pos=points[src_idx]; nrm=normals[src_idx]
                            vmap[key]=len(vertices)
                            vertices.append((*pos,*nrm,float(uv[0]),float(uv[1])))
                        corner_vertices.append(vmap[key])
                    indices.extend(triangulate_strip(corner_vertices,reverse))
            elif primitive==0x4000:
                quads=quad_stream(dec,ptrs[0],poly_count)
                uvs=source_uvs(dec,ptrs[4],poly_count*4)
                uv_cursor=0
                for quad in quads:
                    q=[]
                    for src_idx in quad:
                        uv=uvs[uv_cursor]; uv_cursor+=1
                        key=(src_idx,uv[0],uv[1])
                        if key not in vmap:
                            pos=points[src_idx]; nrm=normals[src_idx]
                            vmap[key]=len(vertices)
                            vertices.append((*pos,*nrm,float(uv[0]),float(uv[1])))
                        q.append(vmap[key])
                    indices.extend(triangulate_quad(q))
            else:
                raise ValueError(f'unsupported primitive {primitive:#x}')
            meshes.append((model_index[mv],global_mat,mesh_first_index,len(indices)-mesh_first_index,primitive,type_material,poly_count))
        model_rows.append((model_first_mesh,len(meshes)-model_first_mesh,model_first_vertex,len(vertices)-model_first_vertex,model_first_index,len(indices)-model_first_index,radius,org))

    if len(vertices)>=65536:
        raise ValueError(f'SA3C v0 global u16 index range exceeded: {len(vertices)} vertices')

    # Nodes.
    node_blob=bytearray()
    for va in node_vas:
        o=node_by_va[va]
        midx=model_index[o.model_va] if o.model_va else sa3c.INVALID_U16
        child=node_index[o.child_va] if o.child_va else sa3c.INVALID_U16
        sibling=node_index[o.sibling_va] if o.sibling_va else sa3c.INVALID_U16
        rot=tuple(v*BAMS_TO_RAD for v in o.rotation_bams)
        node_blob += struct.pack('<HHHHIfffffffff',midx,child,sibling,0,o.flags,*o.position,*rot,*o.scale)

    # Model records.
    model_blob=bytearray()
    for first_mesh,mesh_count,first_vertex,vertex_count,first_index,index_count,radius,org in model_rows:
        model_blob += struct.pack('<IIIIII fi',first_mesh,mesh_count,first_vertex,vertex_count,first_index,index_count,radius,org)

    mesh_blob=bytearray()
    for model_i,mat_i,first_index,index_count,primitive,type_material,poly_count in meshes:
        mesh_blob += struct.pack('<HHIIHHII',model_i,mat_i,first_index,index_count,primitive,type_material,poly_count,0)

    vertex_blob=bytearray().join(struct.pack('<ffffffff',*v) for v in vertices)
    index_blob=bytearray().join(struct.pack('<H',i) for i in indices)

    material_blob=bytearray()
    for diffuse,specular,exponent,texid,flags in material_keys:
        material_blob += struct.pack('<IIfHHIIII',diffuse,specular,exponent,texid,texid,flags,0,0,0)

    tex_blob=bytearray()
    for e in pvm.entries:
        tex_blob += struct.pack('<IIHHHHII',e.index,e.global_index,e.pixel_format,e.data_format,e.width,e.height,0,0)

    keys=[]; channels=[]; anim_rows=[]
    for action_index,action in enumerate(actions):
        if action is None:
            anim_rows.append((action_index,sa3c.INVALID_U16,0,0,len(channels),0,0,0))
            continue
        hierarchy=dec.hierarchy(action.root_object_va)
        motion=action.motion
        record_size=dec.motion_record_size(motion.type_bits)
        first_channel=len(channels)
        for local_i,obj in enumerate(hierarchy):
            raw=dec.pe.read_bytes(motion.data_va+local_i*record_size,record_size)
            if motion.type_bits==3:
                posva,angva,npos,nang=struct.unpack('<IIII',raw); sclva=nscl=0
            else:
                posva,angva,sclva,npos,nang,nscl=struct.unpack('<IIIIII',raw)
            if not (npos or nang or nscl):
                continue
            pos_first=len(keys); poskeys=read_vec_keys(dec,posva,npos); keys.extend(poskeys)
            rot_first=len(keys); rotkeys=read_ang_keys(dec,angva,nang); keys.extend(rotkeys)
            scl_first=len(keys); sclkeys=read_vec_keys(dec,sclva,nscl); keys.extend(sclkeys)
            flags=(1 if npos else 0)|(2 if nang else 0)|(4 if nscl else 0)
            channels.append((node_index[obj.va],flags,pos_first,npos,nang,rot_first,scl_first,nscl,0))
        # motion.interpolation is the legacy field name from 0.0.9.0C.  The
        # value is actually Ninja's high-16 motion pair/function word.  For
        # this Sonic set its low nibble is 2 or 3 and bits 0x00C0 are zero,
        # proving linear interpolation.
        anim_rows.append((action_index,node_index[action.root_object_va],motion.frame_count,motion.interpolation,first_channel,len(channels)-first_channel,motion.type_bits,0))

    anim_blob=bytearray()
    for source_action,root_idx,frames,interp,first_channel,channel_count,type_bits,reserved in anim_rows:
        anim_blob += struct.pack('<HHHHIIII',source_action,root_idx,frames,interp,first_channel,channel_count,type_bits,reserved)

    channel_blob=bytearray()
    for node_i,flags,pos_first,npos,nang,rot_first,scl_first,nscl,reserved in channels:
        channel_blob += struct.pack('<HHIHHIIHH',node_i,flags,pos_first,npos,nang,rot_first,scl_first,nscl,reserved)

    key_blob=bytearray().join(struct.pack('<Ifff',*k) for k in keys)

    sections={
        b'NODE':bytes(node_blob),b'MODL':bytes(model_blob),b'MESH':bytes(mesh_blob),
        b'VRTX':bytes(vertex_blob),b'INDX':bytes(index_blob),b'MATL':bytes(material_blob),
        b'TEXS':bytes(tex_blob),b'ANIM':bytes(anim_blob),b'CHAN':bytes(channel_blob),b'KEYS':bytes(key_blob),
    }
    counts={
        b'NODE':len(node_vas),b'MODL':len(model_rows),b'MESH':len(meshes),
        b'VRTX':len(vertices),b'INDX':len(indices),b'MATL':len(material_keys),
        b'TEXS':len(pvm.entries),b'ANIM':len(anim_rows),b'CHAN':len(channels),b'KEYS':len(keys),
    }
    package=sa3c.build(sections,counts,dec.pe.sha256(),flags=1) # bit0: UV values are raw S16 units stored as float
    report={
        'schema':'sa3ds-pc2004-sonic-sa3c-build-v1',
        'source_chrmodels_sha256':dec.pe.sha256(),
        'source_sonic_pvm_sha256':pvm.sha256(),
        'counts':{k.decode():v for k,v in counts.items()},
        'package_bytes':len(package),
        'package_sha256':__import__('hashlib').sha256(package).hexdigest(),
        'flags':{'uv_raw_s16_as_float':True},
        'source_semantics':{
            'basic_uv_divisor':255.0,
            'triangle_strip_winding':'SA Tools MeshInfo.ToTriangles parity',
            'quad_triangulation':'0,1,2 + 2,1,3 (SA Tools parity)',
            'motion_header_high16':'pair-count/function word; low nibble 2 or 3, interpolation bits 0x00c0 clear => linear',
        },
        'not_yet_certified':[
            'semantic action names/physics intent mapping',
            'final hardware front-face/culling state on Citro3D',
        ],
    }
    return package,report


def main()->int:
    ap=argparse.ArgumentParser()
    ap.add_argument('chrmodels',type=Path)
    ap.add_argument('sonic_pvm',type=Path)
    ap.add_argument('output',type=Path)
    ap.add_argument('--report',type=Path)
    args=ap.parse_args()
    package,report=build_package(args.chrmodels,args.sonic_pvm)
    args.output.write_bytes(package)
    if args.report:
        args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))
    return 0

if __name__=='__main__':
    raise SystemExit(main())
