#include "emerald_coast_collision.h"
#include "sa3dspak.h"
#include "sa3c_pose.h"
#include "../sa1/physics/surface_frame.h"
#include "../sa1/physics/steam_player_parameters.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define GRID_MIN (-512.0f)
#define GRID_MAX 512.0f
#define CELL_SIZE 64.0f
#define MAX_TRIANGLES 16384u
#define MAX_REFERENCES 262144u
/* Source land attribute bit 0 selects collision meshes. Raw flags remain in
 * each triangle; no water, hurt, friction or other flag behavior is inferred. */
#define LAND_SOLID 1u

static uint16_t r16(const uint8_t*p){return (uint16_t)p[0]|((uint16_t)p[1]<<8);}
static uint32_t r32(const uint8_t*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static int span(uint32_t o,uint32_t n,uint32_t s,size_t size){return o<=size&&n<=(size-o)/s;}
static float clamp(float x,float a,float b){return fmaxf(a,fminf(b,x));}
static int cell(float x){return (int)clamp(floorf((x-GRID_MIN)/CELL_SIZE),0,15);}
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static SA1SurfaceFrame actor_frame(const SA1PlayerState*p){
    SA1Vec3 up=p->ground_normal;
    if(!finite3(up)||sa1_vec3_length(up)<0.5f)up=(SA1Vec3){0,1,0};
    return sa1_surface_frame_make(up,(SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)});
}
SA1Vec3 sa3ds_ec_task_origin(const SA1PlayerState*p){
    SA1SurfaceFrame f=actor_frame(p);
    return sa1_vec3_sub(p->pos,sa1_vec3_scale(f.up,SA3DS_EC_CONTROLLER_SUPPORT));
}
void sa3ds_ec_model_transform(const SA1PlayerState*p,SA3CMatrix*out){
    SA1SurfaceFrame f=actor_frame(p);
    /* 0x4915F0 transforms (0,center_height,0), then adds task position;
     * 0x4948C0 uses that center before actor rotations and ACTION drawing.
     * Our contact frame supplies the orientation; retail angle smoothing and
     * airborne orientation remain partial. Model +X is forward, +Y is up. */
    SA1Vec3 o=sa1_vec3_add(sa3ds_ec_task_origin(p),
        sa1_vec3_scale(f.up,SA1_STEAM_SONIC_PARAMETER_EVIDENCE.center_height));
    memset(out,0,sizeof(*out));
    out->m[0]=f.forward.x;out->m[1]=f.forward.y;out->m[2]=f.forward.z;
    out->m[4]=f.up.x;out->m[5]=f.up.y;out->m[6]=f.up.z;
    out->m[8]=-f.right.x;out->m[9]=-f.right.y;out->m[10]=-f.right.z;
    out->m[12]=o.x;out->m[13]=o.y;out->m[14]=o.z;out->m[15]=1;
}
static SA1Vec3 point(const uint8_t*p){return (SA1Vec3){rf(p),rf(p+4),rf(p+8)};}
static SA1Vec3 xform(const SA3CMatrix*m,SA1Vec3 p){return (SA1Vec3){
    p.x*m->m[0]+p.y*m->m[4]+p.z*m->m[8]+m->m[12],
    p.x*m->m[1]+p.y*m->m[5]+p.z*m->m[9]+m->m[13],
    p.x*m->m[2]+p.y*m->m[6]+p.z*m->m[10]+m->m[14]};}
static int inside(const SA3DSECTriangle*t,SA1Vec3 p){
    SA1Vec3 u=sa1_vec3_sub(t->b,t->a),v=sa1_vec3_sub(t->c,t->a),w=sa1_vec3_sub(p,t->a);
    float uu=sa1_vec3_dot(u,u),uv=sa1_vec3_dot(u,v),vv=sa1_vec3_dot(v,v);
    float wu=sa1_vec3_dot(w,u),wv=sa1_vec3_dot(w,v),den=uu*vv-uv*uv;
    if(fabsf(den)<1e-8f)return 0;
    float s=(wu*vv-wv*uv)/den,q=(wv*uu-wu*uv)/den;
    return s>=-0.0001f&&q>=-0.0001f&&s+q<=1.0001f;
}
void sa3ds_ec_collision_destroy(SA3DSECCollision*s){
    if(!s)return;
    free(s->triangles);free(s->references);free(s->visited);memset(s,0,sizeof(*s));
}
int sa3ds_ec_collision_open(SA3DSECCollision*s,const void*data,size_t size){
    if(!s||!data)return 0;
    memset(s,0,sizeof(*s));SA3DSPak pak;SA3DSPakChunk chunk;
    if(sa3dspak_open(&pak,data,size)!=SA3DSPAK_OK||
       sa3dspak_find(&pak,"COLLISION",&chunk)!=SA3DSPAK_OK||chunk.size<64)return 0;
    const uint8_t*b=chunk.data;
    if(memcmp(b,"SA3DECOL",8)||r16(b+8)!=1||r16(b+10)!=0)return 0;
    uint32_t na=r32(b+12),nm=r32(b+16),ni=r32(b+20),nv=r32(b+24),nx=r32(b+28);
    uint32_t ao=r32(b+32),mo=r32(b+36),io=r32(b+40),vo=r32(b+44),xo=r32(b+48);
    if(!na||!nm||!ni||!nv||nv>=65536||nx%3||
       !span(ao,na,32,chunk.size)||!span(mo,nm,32,chunk.size)||
       !span(io,ni,80,chunk.size)||!span(vo,nv,12,chunk.size)||!span(xo,nx,2,chunk.size)||
       ao<64||ao+(uint64_t)na*32>mo||mo+(uint64_t)nm*32>io||
       io+(uint64_t)ni*80>vo||vo+(uint64_t)nv*12>xo||xo+(uint64_t)nx*2!=chunk.size)return 0;
    uint32_t first=0,count=0;
    for(uint32_t i=0;i<na;i++){
        const uint8_t*a=b+ao+(size_t)i*32;
        if(r32(a+4)>ni||r32(a+8)>ni-r32(a+4))return 0;
        if(r32(a)==0){if(count)return 0;first=r32(a+4);count=r32(a+8);}
    }
    if(!count)return 0;
    for(uint32_t i=0;i<nv;i++)if(!finite3(point(b+vo+(size_t)i*12)))return 0;
    for(uint32_t i=0;i<nx;i++)if(r16(b+xo+(size_t)i*2)>=nv)return 0;
    for(uint32_t i=0;i<nm;i++){
        const uint8_t*m=b+mo+(size_t)i*32;
        if(r32(m)>nv||r32(m+4)>nv-r32(m)||r32(m+8)>nx||r32(m+12)>nx-r32(m+8)||r32(m+12)%3)return 0;
    }
    for(uint32_t i=0;i<ni;i++){
        const uint8_t*r=b+io+(size_t)i*80;
        if(r32(r)>=nm||!finite3(point(r+16))||!isfinite(rf(r+28))||rf(r+28)<0||
           !finite3(point(r+32))||!finite3(point(r+44))||!finite3(point(r+56)))return 0;
    }
    s->triangles=calloc(MAX_TRIANGLES,sizeof(*s->triangles));
    if(!s->triangles)goto fail;
    for(uint32_t i=first;i<first+count;i++){
        const uint8_t*r=b+io+(size_t)i*80;
        if(!(r32(r+4)&LAND_SOLID))continue;
        if(hypotf(rf(r+16),rf(r+24))-rf(r+28)>500.0f)continue;
        const uint8_t*m=b+mo+(size_t)r32(r)*32;SA3CMatrix matrix;
        SA1Vec3 p=point(r+32),a=point(r+44),sc=point(r+56);
        sa3c_matrix_build_object(&matrix,(SA3CVec3){p.x,p.y,p.z},
            (SA3CVec3){a.x,a.y,a.z},(SA3CVec3){sc.x,sc.y,sc.z},r32(r+72));
        s->instance_count++;
        for(uint32_t j=0;j<r32(m+12);j+=3){
            SA3DSECTriangle t={0};SA1Vec3 *points[]={&t.a,&t.b,&t.c};
            for(uint32_t k=0;k<3;k++){
                uint16_t index=r16(b+xo+(size_t)(r32(m+8)+j+k)*2);
                if(index<r32(m)||index-r32(m)>=r32(m+4))goto fail;
                *points[k]=xform(&matrix,point(b+vo+(size_t)index*12));
                if(!finite3(*points[k]))goto fail;
            }
            SA1Vec3 cross=sa1_vec3_cross(sa1_vec3_sub(t.b,t.a),sa1_vec3_sub(t.c,t.a));
            if(sa1_vec3_length(cross)<0.00001f)continue;
            t.normal=sa1_vec3_normalize(cross);
            t.min_x=fminf(t.a.x,fminf(t.b.x,t.c.x));t.max_x=fmaxf(t.a.x,fmaxf(t.b.x,t.c.x));
            t.min_z=fminf(t.a.z,fminf(t.b.z,t.c.z));t.max_z=fmaxf(t.a.z,fmaxf(t.b.z,t.c.z));
            if(t.max_x<GRID_MIN||t.min_x>GRID_MAX||t.max_z<GRID_MIN||t.min_z>GRID_MAX)continue;
            t.source_instance=i;t.source_triangle=j/3;t.flags=r32(r+4);
            if(s->triangle_count==MAX_TRIANGLES)goto fail;
            s->triangles[s->triangle_count++]=t;
        }
    }
    if(!s->triangle_count)goto fail;
    SA3DSECTriangle *compact=realloc(s->triangles,(size_t)s->triangle_count*sizeof(*s->triangles));
    if(compact)s->triangles=compact;
    for(uint32_t i=0;i<s->triangle_count;i++){
        SA3DSECTriangle*t=&s->triangles[i];
        for(int z=cell(t->min_z);z<=cell(t->max_z);z++)for(int x=cell(t->min_x);x<=cell(t->max_x);x++)s->cells[z*16+x+1]++;
    }
    for(unsigned i=1;i<=256;i++)s->cells[i]+=s->cells[i-1];
    s->reference_count=s->cells[256];if(s->reference_count>MAX_REFERENCES)goto fail;
    s->references=malloc((size_t)s->reference_count*sizeof(*s->references));
    s->visited=calloc(s->triangle_count,sizeof(*s->visited));
    if(!s->references||!s->visited)goto fail;
    uint32_t cursor[256];memcpy(cursor,s->cells,sizeof(cursor));
    for(uint32_t i=0;i<s->triangle_count;i++){
        SA3DSECTriangle*t=&s->triangles[i];
        for(int z=cell(t->min_z);z<=cell(t->max_z);z++)for(int x=cell(t->min_x);x<=cell(t->max_x);x++)s->references[cursor[z*16+x]++]=i;
    }
    s->world=(SA1PhysicsWorld){.query_surface=sa3ds_ec_collision_surface,
        .query_contact=sa3ds_ec_collision_contact,.context=s,
        .reset_min_x=GRID_MIN,.reset_max_x=GRID_MAX,.reset_min_y=-80.0f,
        .reset_min_z=GRID_MIN,.reset_max_z=GRID_MAX};
    return 1;
fail:sa3ds_ec_collision_destroy(s);return 0;
}

SA1Surface sa3ds_ec_collision_surface(void*ctx,float x,float z){
    SA3DSECCollision*s=ctx;SA1Surface result={0};float best=1e30f;
    if(!s||!isfinite(x)||!isfinite(z)||x<GRID_MIN||x>GRID_MAX||z<GRID_MIN||z>GRID_MAX)return result;
    s->queries++;unsigned c=cell(z)*16+cell(x);
    for(uint32_t j=s->cells[c];j<s->cells[c+1];j++){
        SA3DSECTriangle*t=&s->triangles[s->references[j]];s->triangle_tests++;
        if(t->normal.y<=0.45f||x<t->min_x||x>t->max_x||z<t->min_z||z>t->max_z)continue;
        float y=t->a.y-(t->normal.x*(x-t->a.x)+t->normal.z*(z-t->a.z))/t->normal.y;
        float score=fabsf(s->reference_y-1.0f-y);
        if(y>s->reference_y+2.35f||score>=best||!inside(t,(SA1Vec3){x,y,z}))continue;
        best=score;result=(SA1Surface){.ground_y=y,.grad_x=-t->normal.x/t->normal.y,
            .grad_z=-t->normal.z/t->normal.y,.normal=t->normal,
            .slope_radians=sa1_surface_slope_radians(t->normal),.surface_id=(int32_t)s->references[j],.valid=true};
    }
    return result;
}
SA1Contact sa3ds_ec_collision_contact(void*ctx,SA1Vec3 p,SA1Vec3 probe,float range){
    SA3DSECCollision*s=ctx;SA1Contact result={.surface_id=-1};float best=1e30f;
    if(!s||!finite3(p)||!finite3(probe)||!isfinite(range)||range<0||p.x<GRID_MIN||p.x>GRID_MAX||p.z<GRID_MIN||p.z>GRID_MAX)return result;
    s->queries++;unsigned c=cell(p.z)*16+cell(p.x);probe=sa1_vec3_normalize(probe);
    for(uint32_t j=s->cells[c];j<s->cells[c+1];j++){
        SA3DSECTriangle*t=&s->triangles[s->references[j]];s->triangle_tests++;
        float d=sa1_vec3_dot(sa1_vec3_sub(p,t->a),t->normal);
        if(t->normal.y<=0.45f||d< -0.35f||d>range||sa1_vec3_dot(probe,t->normal)>-0.15f)continue;
        SA1Vec3 hit=sa1_vec3_sub(p,sa1_vec3_scale(t->normal,d));
        float score=fabsf(d-1.0f);
        if(score>=best||!inside(t,hit))continue;
        best=score;result=(SA1Contact){.point=hit,.normal=t->normal,
            .signed_distance=d,.distance=fabsf(d),.surface_id=(int32_t)s->references[j],
            .surface_flags=t->flags,.valid=true};
    }
    return result;
}
int sa3ds_ec_collision_spawn(SA3DSECCollision*s,float x,float z,float hint_y){
    if(!s||!isfinite(hint_y))return 0;
    s->reference_y=hint_y;SA1Surface floor=sa3ds_ec_collision_surface(s,x,z);
    if(!floor.valid)return 0;
    s->world.reset_position=(SA1Vec3){x,floor.ground_y+1.0f,z};return 1;
}

void sa3ds_ec_collision_resolve_walls(SA3DSECCollision*s,SA1Vec3 previous,SA1PlayerState*p){
    if(!s||!p||!finite3(previous)||!finite3(p->pos))return;
    /* Normal PSetPosition uses task + rotated (0,height/2,0), radius height/2.
     * Bridge from the inherited 1-unit support center without changing its
     * movement convention. Planar sliding is still a partial contact adapter;
     * exact retail arbitration, edge/corner capsules and probes are pending. */
    float radius=SA1_STEAM_SONIC_PARAMETER_EVIDENCE.height*0.5f;
    SA1Vec3 up=actor_frame(p).up;
    SA1Vec3 shift=sa1_vec3_scale(up,radius-SA3DS_EC_CONTROLLER_SUPPORT);
    previous=sa1_vec3_add(previous,shift);
    SA1Vec3 center=sa1_vec3_add(p->pos,shift);
    if(++s->query_serial==0){memset(s->visited,0,(size_t)s->triangle_count*4);s->query_serial=1;}
    int x0=cell(fminf(previous.x,center.x)-radius),x1=cell(fmaxf(previous.x,center.x)+radius);
    int z0=cell(fminf(previous.z,center.z)-radius),z1=cell(fmaxf(previous.z,center.z)+radius);
    s->queries++;
    for(int z=z0;z<=z1;z++)for(int x=x0;x<=x1;x++){
        unsigned c=z*16+x;
        for(uint32_t j=s->cells[c];j<s->cells[c+1];j++){
            uint32_t ti=s->references[j];if(s->visited[ti]==s->query_serial)continue;s->visited[ti]=s->query_serial;
            SA3DSECTriangle*t=&s->triangles[ti];s->triangle_tests++;
            if(fabsf(t->normal.y)>0.45f)continue;
            float old=sa1_vec3_dot(sa1_vec3_sub(previous,t->a),t->normal);
            SA1Vec3 n=old<0?sa1_vec3_scale(t->normal,-1):t->normal;old=fabsf(old);
            float now=sa1_vec3_dot(sa1_vec3_sub(center,t->a),n);
            if(now>=radius||now>=old)continue;
            float den=old-now;
            float fraction=den>1e-6f?clamp((old-radius)/den,0,1):0;
            SA1Vec3 hit=sa1_vec3_add(previous,sa1_vec3_scale(sa1_vec3_sub(center,previous),fraction));
            hit=sa1_vec3_sub(hit,sa1_vec3_scale(n,sa1_vec3_dot(sa1_vec3_sub(hit,t->a),n)));
            if(!inside(t,hit))continue;
            SA1Vec3 correction=sa1_vec3_scale(n,radius-now);
            p->pos=sa1_vec3_add(p->pos,correction);
            center=sa1_vec3_add(center,correction);
            float inward=sa1_vec3_dot(p->vel,n);
            if(inward<0)p->vel=sa1_vec3_sub(p->vel,sa1_vec3_scale(n,inward));
            s->wall_hits++;
        }
    }
    /* The controller resolves its floor BEFORE this wall pass. An inclined
     * wall can then push a supported actor underneath that same floor. Repair
     * that adapter-ordering inconsistency using imported support, not gravity
     * or movement tuning. Do not snap airborne actors or invent missing floors.
     * Retail joint polygon arbitration/edge handling are still unresolved. */
    if(p->grounded){
        SA1Surface floor=sa3ds_ec_collision_surface(s,p->pos.x,p->pos.z);
        if(floor.valid){
            float distance=(p->pos.y-floor.ground_y)*floor.normal.y;
            if(distance<SA3DS_EC_CONTROLLER_SUPPORT){
                p->pos=sa1_vec3_add(p->pos,sa1_vec3_scale(floor.normal,
                    SA3DS_EC_CONTROLLER_SUPPORT-distance));
                float inward=sa1_vec3_dot(p->vel,floor.normal);
                if(inward<0)p->vel=sa1_vec3_sub(p->vel,sa1_vec3_scale(floor.normal,inward));
            }
        }
    }
}
