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
static int cell(float x,float center){return (int)clamp(floorf(((x-center)-GRID_MIN)/CELL_SIZE),0,15);}
static int cell_x(const SA3DSECCollision*s,float x){return cell(x,s->center_x);}
static int cell_z(const SA3DSECCollision*s,float z){return cell(z,s->center_z);}
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
static SA1SurfaceFrame actor_frame(const SA1PlayerState*p){
    SA1Vec3 up=p->ground_normal;
    if(!finite3(up)||sa1_vec3_length(up)<0.5f)up=(SA1Vec3){0,1,0};
    SA1Vec3 forward=p->surface_frame_active?p->surface_forward:
        (SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)};
    if(!finite3(forward)||sa1_vec3_length(forward)<.5f)
        forward=(SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)};
    return sa1_surface_frame_make(up,forward);
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
     * airborne orientation remain partial. PC2004 movement 0x43ECE3 uses
     * -taskY, while display 0x494A41 uses 0x8000-taskY: model -X is
     * forward, +Y is up. Preserve that half-turn about the support normal. */
    SA1Vec3 o=sa1_vec3_add(sa3ds_ec_task_origin(p),
        sa1_vec3_scale(f.up,SA1_STEAM_SONIC_PARAMETER_EVIDENCE.center_height));
    memset(out,0,sizeof(*out));
    out->m[0]=-f.forward.x;out->m[1]=-f.forward.y;out->m[2]=-f.forward.z;
    out->m[4]=f.up.x;out->m[5]=f.up.y;out->m[6]=f.up.z;
    out->m[8]=f.right.x;out->m[9]=f.right.y;out->m[10]=f.right.z;
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
static int same_point(SA1Vec3 a,SA1Vec3 b){
    SA1Vec3 d=sa1_vec3_sub(a,b);return sa1_vec3_dot(d,d)<1e-6f;
}
static void mark_contact_edges_linear(SA3DSECCollision*s){
    for(uint32_t i=0;i<s->triangle_count;i++){
        SA3DSECTriangle*t=&s->triangles[i];
        int wall=fabsf(t->normal.y)<=0.45f,support=t->normal.y>0.45f;
        if(!wall&&!support)continue;
        SA1Vec3 v[3]={t->a,t->b,t->c};
        for(unsigned e=0;e<3;e++){
            int neighbor=0;SA1Vec3 a=v[e],b=v[(e+1)%3];
            unsigned c=(unsigned)cell_z(s,(a.z+b.z)*.5f)*16u+(unsigned)cell_x(s,(a.x+b.x)*.5f);
            for(uint32_t q=s->cells[c];q<s->cells[c+1];q++){
                uint32_t j=s->references[q];SA3DSECTriangle*n=&s->triangles[j];if(i==j)continue;
                /*Only one neighbor class can affect this triangle's bits.
                 *Reject irrelevant normals before endpoint distance checks.*/
                if(wall?!(fabsf(n->normal.y)<=0.45f):!(n->normal.y>0.45f))continue;
                float ndot=sa1_vec3_dot(t->normal,n->normal);
                if(wall?!(fabsf(ndot)>0.92f):!(ndot>0.9f))continue;
                SA1Vec3 nv[3]={n->a,n->b,n->c};int found_a=0,found_b=0;
                for(unsigned k=0;k<3;k++){found_a|=same_point(a,nv[k]);found_b|=same_point(b,nv[k]);}
                if(!found_a||!found_b)continue;
                /*The previous scan only ORed this predicate. Its first true
                 *result fixes the edge mask; later candidates cannot change it.*/
                neighbor=1;break;
            }
            if(wall&&!neighbor)t->wall_edge_mask|=(uint8_t)(1u<<e);
            if(support&&neighbor)t->support_edge_mask|=(uint8_t)(1u<<e);
        }
    }
}
/*Region loads previously searched an entire64-unit grid bucket for every
 *edge. Large beach triangles put thousands of unrelated candidates there.
 *Index endpoints in .01-unit bins, keeping the original .001-unit strict
 *same_point predicate and grid membership as the final authority.*/
typedef struct {int32_t x,y,z;uint32_t triangle,next;} EdgeVertex;
static int edge_key(SA1Vec3 p,int32_t key[3]){
    if(!finite3(p)||fabsf(p.x)>10000000||fabsf(p.y)>10000000||fabsf(p.z)>10000000)return 0;
    key[0]=(int32_t)floor((double)p.x*100.0);
    key[1]=(int32_t)floor((double)p.y*100.0);
    key[2]=(int32_t)floor((double)p.z*100.0);return 1;
}
static uint32_t edge_bucket(int32_t x,int32_t y,int32_t z,uint32_t mask){
    uint32_t h=(uint32_t)x*73856093u^(uint32_t)y*19349663u^(uint32_t)z*83492791u;
    h^=h>>16;return h&mask;
}
static void mark_contact_edges(SA3DSECCollision*s){
    uint32_t count=s->triangle_count*3u,buckets=1;
    if(!count)return;
    while(buckets<count*2u)buckets*=2u;
    EdgeVertex*nodes=malloc((size_t)count*sizeof(*nodes));
    uint32_t*heads=malloc((size_t)buckets*sizeof(*heads));
    if(!nodes||!heads)goto fallback;
    for(uint32_t h=0;h<buckets;h++)heads[h]=UINT32_MAX;
    for(uint32_t i=0;i<s->triangle_count;i++){
        SA3DSECTriangle*t=&s->triangles[i];SA1Vec3 v[3]={t->a,t->b,t->c};
        for(unsigned k=0;k<3;k++){
            int32_t key[3];if(!edge_key(v[k],key))goto fallback;
            uint32_t n=i*3+k,h=edge_bucket(key[0],key[1],key[2],buckets-1);
            nodes[n]=(EdgeVertex){key[0],key[1],key[2],i,heads[h]};heads[h]=n;
        }
    }
    for(uint32_t i=0;i<s->triangle_count;i++){
        SA3DSECTriangle*t=&s->triangles[i];
        int wall=fabsf(t->normal.y)<=0.45f,support=t->normal.y>0.45f;
        if(!wall&&!support)continue;
        SA1Vec3 v[3]={t->a,t->b,t->c};
        for(unsigned e=0;e<3;e++){
            SA1Vec3 a=v[e],b=v[(e+1)%3];int32_t key[3];edge_key(a,key);
            int cx=cell_x(s,(a.x+b.x)*.5f),cz=cell_z(s,(a.z+b.z)*.5f),neighbor=0;
            /*Adjacent bins include tolerance matches across either side of
             *zero and bin boundaries. Bins are ten times the point tolerance.*/
            for(int x=-1;x<=1&&!neighbor;x++)for(int y=-1;y<=1&&!neighbor;y++)for(int z=-1;z<=1&&!neighbor;z++){
                int32_t qx=key[0]+x,qy=key[1]+y,qz=key[2]+z;
                for(uint32_t q=heads[edge_bucket(qx,qy,qz,buckets-1)];q!=UINT32_MAX;q=nodes[q].next){
                    EdgeVertex*nkey=&nodes[q];uint32_t j=nkey->triangle;
                    if(nkey->x!=qx||nkey->y!=qy||nkey->z!=qz||j==i)continue;
                    SA3DSECTriangle*n=&s->triangles[j];
                    if(wall?!(fabsf(n->normal.y)<=0.45f):!(n->normal.y>0.45f))continue;
                    float ndot=sa1_vec3_dot(t->normal,n->normal);
                    if(wall?!(fabsf(ndot)>0.92f):!(ndot>0.9f))continue;
                    /*Retain exactly the old midpoint grid-bucket membership,
                     *including clamped outer cells and rounded AABB limits.*/
                    if(cx<cell_x(s,n->min_x)||cx>cell_x(s,n->max_x)||cz<cell_z(s,n->min_z)||cz>cell_z(s,n->max_z))continue;
                    SA1Vec3 nv[3]={n->a,n->b,n->c};int found_a=0,found_b=0;
                    for(unsigned k=0;k<3;k++){found_a|=same_point(a,nv[k]);found_b|=same_point(b,nv[k]);}
                    if(found_a&&found_b){neighbor=1;break;}
                }
            }
            if(wall&&!neighbor)t->wall_edge_mask|=(uint8_t)(1u<<e);
            if(support&&neighbor)t->support_edge_mask|=(uint8_t)(1u<<e);
        }
    }
    free(nodes);free(heads);return;
fallback:
    /*No masks have been touched during index construction. Memory pressure
     *or coordinates outside the index domain retain the existing scan.*/
    free(nodes);free(heads);mark_contact_edges_linear(s);
}
static SA1Vec3 edge_point(const SA3DSECTriangle*t,unsigned e);
/* Uniform-grid references for every resident triangle (static + dynamic). */
static int build_index(SA3DSECCollision*s){
    uint32_t*references=NULL,*visited=NULL;
    memset(s->cells,0,sizeof(s->cells));
    for(uint32_t i=0;i<s->triangle_count;i++){
        SA3DSECTriangle*t=&s->triangles[i];
        for(int z=cell_z(s,t->min_z);z<=cell_z(s,t->max_z);z++)for(int x=cell_x(s,t->min_x);x<=cell_x(s,t->max_x);x++)s->cells[z*16+x+1]++;
    }
    for(unsigned i=1;i<=256;i++)s->cells[i]+=s->cells[i-1];
    if(s->cells[256]>MAX_REFERENCES)return 0;
    references=malloc((size_t)(s->cells[256]?s->cells[256]:1)*sizeof(*references));
    visited=calloc(s->triangle_count?s->triangle_count:1,sizeof(*visited));
    if(!references||!visited){free(references);free(visited);return 0;}
    uint32_t cursor[256];memcpy(cursor,s->cells,sizeof(cursor));
    for(uint32_t i=0;i<s->triangle_count;i++){
        SA3DSECTriangle*t=&s->triangles[i];
        for(int z=cell_z(s,t->min_z);z<=cell_z(s,t->max_z);z++)for(int x=cell_x(s,t->min_x);x<=cell_x(s,t->max_x);x++)references[cursor[z*16+x]++]=i;
    }
    free(s->references);free(s->visited);
    s->references=references;s->visited=visited;s->reference_count=s->cells[256];s->query_serial=0;
    return 1;
}
void sa3ds_ec_dynamic_clear(SA3DSECDynamicSet*set){if(set){set->triangle_count=set->owner_count=0;set->changes++;}}
int sa3ds_ec_dynamic_add(SA3DSECDynamicSet*set,uint32_t owner,uint32_t flags,const float(*points)[3],
    uint32_t point_count,const uint16_t*indices,uint32_t index_count,const SA3CMatrix*m){
    if(!set||!points||!indices||!m||index_count%3||set->owner_count>=SA3DS_EC_DYNAMIC_MAX_OWNERS)return 0;
    for(uint32_t i=0;i<set->owner_count;i++)if(set->owners[i].owner==owner)return 0;
    uint32_t first=set->triangle_count,count=0;
    for(uint32_t j=0;j<index_count;j+=3){
        SA3DSECTriangle t={0};SA1Vec3*corner[]={&t.a,&t.b,&t.c};
        for(unsigned k=0;k<3;k++){
            if(indices[j+k]>=point_count)return 0;
            const float*p=points[indices[j+k]];*corner[k]=xform(m,(SA1Vec3){p[0],p[1],p[2]});
            if(!finite3(*corner[k]))return 0;
        }
        SA1Vec3 cross=sa1_vec3_cross(sa1_vec3_sub(t.b,t.a),sa1_vec3_sub(t.c,t.a));
        if(sa1_vec3_length(cross)<0.00001f)continue;
        if(first+count>=SA3DS_EC_DYNAMIC_MAX_TRIANGLES)return 0; /*nothing committed yet*/
        t.normal=sa1_vec3_normalize(cross);
        t.min_x=fminf(t.a.x,fminf(t.b.x,t.c.x));t.max_x=fmaxf(t.a.x,fmaxf(t.b.x,t.c.x));
        t.min_z=fminf(t.a.z,fminf(t.b.z,t.c.z));t.max_z=fmaxf(t.a.z,fmaxf(t.b.z,t.c.z));
        t.source_instance=SA3DS_EC_DYNAMIC_SOURCE|owner;t.source_triangle=j/3;t.flags=flags;
        set->triangles[first+count++]=t;
    }
    /*Owner-local edges: a shared, near-coplanar edge supports and is not a
     *wall; every other edge is a boundary ledge (static landtable rules).*/
    for(uint32_t a=first;a<first+count;a++){
        SA3DSECTriangle*t=&set->triangles[a];t->wall_edge_mask=7;t->support_edge_mask=0;
        for(unsigned e=0;e<3;e++){
            SA1Vec3 p0=edge_point(t,e),p1=edge_point(t,(e+1)%3);
            for(uint32_t b=first;b<first+count;b++){
                if(b==a)continue;
                const SA3DSECTriangle*u=&set->triangles[b];
                for(unsigned f=0;f<3;f++){
                    SA1Vec3 q0=edge_point(u,f),q1=edge_point(u,(f+1)%3);
                    if(!((same_point(p0,q1)&&same_point(p1,q0))||(same_point(p0,q0)&&same_point(p1,q1))))continue;
                    if(sa1_vec3_dot(t->normal,u->normal)>0.999f){
                        t->wall_edge_mask&=(uint8_t)~(1u<<e);
                        if(t->normal.y>0.45f)t->support_edge_mask|=(uint8_t)(1u<<e);
                    }
                }
            }
        }
    }
    set->owners[set->owner_count++]=(SA3DSECDynamicOwner){owner,first,count};
    set->triangle_count=first+count;set->changes++;return 1;
}
int sa3ds_ec_dynamic_remove(SA3DSECDynamicSet*set,uint32_t owner){
    if(!set)return 0;
    for(uint32_t i=0;i<set->owner_count;i++){
        SA3DSECDynamicOwner o=set->owners[i];if(o.owner!=owner)continue;
        memmove(&set->triangles[o.first],&set->triangles[o.first+o.count],
            (size_t)(set->triangle_count-o.first-o.count)*sizeof(*set->triangles));
        set->triangle_count-=o.count;
        memmove(&set->owners[i],&set->owners[i+1],(size_t)(set->owner_count-i-1)*sizeof(*set->owners));
        set->owner_count--;
        for(uint32_t k=i;k<set->owner_count;k++)set->owners[k].first-=o.count;
        set->changes++;return 1;
    }
    return 0;
}
int sa3ds_ec_object_support_add(SA3DSECDynamicSet*set,const SA3DSECObjectSupport*o){
    if(!set||!o||set->support_count>=SA3DS_EC_OBJECT_SUPPORT_MAX||!isfinite(o->top_y)||!isfinite(o->center_x)||!isfinite(o->center_z)||
       !finite3(o->normal)||!(o->normal.y>0))return 0;
    set->supports[set->support_count++]=*o;return 1;
}
void sa3ds_ec_object_support_clear(SA3DSECDynamicSet*set){if(set)set->support_count=0;}
int sa3ds_ec_object_support_inside(const SA3DSECObjectSupport*o,float px,float pz){
    if(!o)return 0;
    float a=o->half_x,b=o->half_z,sn=o->sin_yaw,cs=o->cos_yaw;
    const float x[4]={a,a,-a,-a},z[4]={b,-b,-b,b};float cx[4],cz[4];
    for(unsigned i=0;i<4;i++){
        cx[i]=(float)((double)o->center_x+(float)((double)x[i]*cs+(double)z[i]*sn));
        cz[i]=(float)((double)o->center_z+(float)((double)z[i]*cs-(double)x[i]*sn));
    }
    for(unsigned i=0;i<4;i++){
        unsigned j=(i+1)&3;
        float ax=(float)((double)cx[i]-px),az=(float)((double)cz[i]-pz),bx=(float)((double)cx[j]-px),bz=(float)((double)cz[j]-pz);
        if(!((float)((double)az*bx-(double)ax*bz)>0))return 0;
    }
    return 1;
}
int sa3ds_ec_collision_apply_dynamic(SA3DSECCollision*s,SA1PlayerState*p){
    if(!s||!s->triangles||!s->static_triangle_count)return 0;
    uint32_t instance=UINT32_MAX,triangle=UINT32_MAX;
    if(p&&p->surface_id>=0&&(uint32_t)p->surface_id<s->triangle_count){
        instance=s->triangles[p->surface_id].source_instance;triangle=s->triangles[p->surface_id].source_triangle;
    }
    const SA3DSECDynamicSet*set=s->dynamic;uint32_t extra=0;
    if(set)for(uint32_t i=0;i<set->triangle_count;i++){
        const SA3DSECTriangle*t=&set->triangles[i];
        if(!(t->max_x<s->min_x||t->min_x>s->max_x||t->max_z<s->min_z||t->min_z>s->max_z))extra++;
    }
    SA3DSECTriangle*grown=realloc(s->triangles,(size_t)(s->static_triangle_count+extra)*sizeof(*grown));
    if(!grown)return 0;
    s->triangles=grown;s->triangle_count=s->static_triangle_count;
    if(set)for(uint32_t i=0;i<set->triangle_count;i++){
        const SA3DSECTriangle*t=&set->triangles[i];
        if(!(t->max_x<s->min_x||t->min_x>s->max_x||t->max_z<s->min_z||t->min_z>s->max_z))s->triangles[s->triangle_count++]=*t;
    }
    if(!build_index(s))return 0;
    if(p&&instance!=UINT32_MAX){
        p->surface_id=-1;
        for(uint32_t i=0;i<s->triangle_count;i++)
            if(s->triangles[i].source_instance==instance&&s->triangles[i].source_triangle==triangle){p->surface_id=(int32_t)i;break;}
    }
    return 1;
}
void sa3ds_ec_collision_destroy(SA3DSECCollision*s){
    if(!s)return;
    free(s->triangles);free(s->references);free(s->visited);memset(s,0,sizeof(*s));
}
int sa3ds_ec_collision_open(SA3DSECCollision*s,const void*data,size_t size){
    return sa3ds_ec_collision_open_region(s,data,size,0,0.0f,0.0f);
}
int sa3ds_ec_collision_open_region(SA3DSECCollision*s,const void*data,size_t size,
    uint32_t act_index,float center_x,float center_z){
    if(!s)return 0;
    memset(s,0,sizeof(*s));
    SA3DSECCollisionArchive archive;
    if(!sa3ds_ec_collision_archive_open(&archive,data,size))return 0;
    return sa3ds_ec_collision_open_archive_region(s,&archive,act_index,center_x,center_z);
}
int sa3ds_ec_collision_archive_open(SA3DSECCollisionArchive*out,const void*data,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));
    SA3DSPak pak;SA3DSPakChunk chunk;
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
    for(uint32_t i=0;i<na;i++){
        const uint8_t*a=b+ao+(size_t)i*32;
        if(r32(a+4)>ni||r32(a+8)>ni-r32(a+4))return 0;
    }
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
    *out=(SA3DSECCollisionArchive){data,size,b,na,nm,ni,nv,nx,ao,mo,io,vo,xo};
    return 1;
}
int sa3ds_ec_collision_open_archive_region(SA3DSECCollision*s,
    const SA3DSECCollisionArchive*archive,uint32_t act_index,float center_x,float center_z){
    if(!s)return 0;
    /* Permit callers to rebuild from their current stage's borrowed view. */
    SA3DSECCollisionArchive source={0};if(archive)source=*archive;
    memset(s,0,sizeof(*s));
    float min_x=center_x+GRID_MIN,max_x=center_x+GRID_MAX;
    float min_z=center_z+GRID_MIN,max_z=center_z+GRID_MAX;
    if(!source.package||!source.collision||!isfinite(center_x)||!isfinite(center_z)||
       !isfinite(min_x)||!isfinite(max_x)||!isfinite(min_z)||!isfinite(max_z)||
       !(min_x<max_x)||!(min_z<max_z))return 0;
    const uint8_t*b=source.collision;
    uint32_t na=source.act_count,ni=source.instance_count,mo=source.model_offset,io=source.instance_offset;
    uint32_t ao=source.act_offset,vo=source.vertex_offset,xo=source.index_offset;
    uint32_t first=0,count=0;
    for(uint32_t i=0;i<na;i++){
        const uint8_t*a=b+ao+(size_t)i*32;
        if(r32(a+4)>ni||r32(a+8)>ni-r32(a+4))return 0;
        if(r32(a)==act_index){if(count)return 0;first=r32(a+4);count=r32(a+8);}
    }
    if(!count)return 0;
    s->archive=source;
    s->act_index=act_index;s->center_x=center_x;s->center_z=center_z;
    s->min_x=min_x;s->max_x=max_x;s->min_z=min_z;s->max_z=max_z;
    s->triangles=calloc(MAX_TRIANGLES,sizeof(*s->triangles));
    if(!s->triangles)goto fail;
    for(uint32_t i=first;i<first+count;i++){
        const uint8_t*r=b+io+(size_t)i*80;
        if(!(r32(r+4)&LAND_SOLID))continue;
        if(hypotf(rf(r+16)-center_x,rf(r+24)-center_z)-rf(r+28)>500.0f)continue;
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
            if(t.max_x<s->min_x||t.min_x>s->max_x||t.max_z<s->min_z||t.min_z>s->max_z)continue;
            t.source_instance=i;t.source_triangle=j/3;t.flags=r32(r+4);
            if(s->triangle_count==MAX_TRIANGLES)goto fail;
            s->triangles[s->triangle_count++]=t;
        }
    }
    if(!s->triangle_count)goto fail;
    SA3DSECTriangle *compact=realloc(s->triangles,(size_t)s->triangle_count*sizeof(*s->triangles));
    if(compact)s->triangles=compact;
    s->static_triangle_count=s->triangle_count;
    if(!build_index(s))goto fail;
    mark_contact_edges(s);
    s->world=(SA1PhysicsWorld){.query_surface=sa3ds_ec_collision_surface,
        .query_contact=sa3ds_ec_collision_contact,.context=s,
        .reset_min_x=s->min_x,.reset_max_x=s->max_x,.reset_min_y=-80.0f,
        .reset_min_z=s->min_z,.reset_max_z=s->max_z};
    return 1;
fail:sa3ds_ec_collision_destroy(s);return 0;
}

int sa3ds_ec_collision_floor_below_frame(SA3DSECCollision*s,float x,float ceiling,float z,float*height,SA1Vec3*normal){
    if(!s||!height||!s->triangles||!isfinite(x)||!isfinite(z)||!isfinite(ceiling)||
       hypotf(x-s->center_x,z-s->center_z)>490.f)return 0;
    float best=-1000000.f;if(normal)*normal=(SA1Vec3){0,1,0};unsigned c=cell_z(s,z)*16u+cell_x(s,x);
    s->queries++;
    for(uint32_t j=s->cells[c];j<s->cells[c+1];j++){
        SA3DSECTriangle*t=&s->triangles[s->references[j]];s->triangle_tests++;
        if((t->flags&0x400002u)||t->normal.y<=0||x<t->min_x||x>t->max_x||z<t->min_z||z>t->max_z)continue;
        float y=t->a.y-(t->normal.x*(x-t->a.x)+t->normal.z*(z-t->a.z))/t->normal.y;
        if(y<=ceiling&&y>best&&inside(t,(SA1Vec3){x,y,z})){best=y;if(normal)*normal=t->normal;}
    }
    *height=best;return 1;
}

int sa3ds_ec_collision_floor_below(SA3DSECCollision*s,float x,float ceiling,float z,float*height){
    return sa3ds_ec_collision_floor_below_frame(s,x,ceiling,z,height,NULL);
}

SA1Surface sa3ds_ec_collision_surface(void*ctx,float x,float z){
    SA3DSECCollision*s=ctx;SA1Surface result={0};float best=1e30f;
    if(!s||!isfinite(x)||!isfinite(z)||x<s->min_x||x>s->max_x||z<s->min_z||z>s->max_z)return result;
    s->queries++;unsigned c=cell_z(s,z)*16+cell_x(s,x);
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
    /* Object supports (status0x20 ground) join the same reference selection;
     * they are flat planes with no landtable triangle id. */
    for(uint32_t i=0;s->dynamic&&i<s->dynamic->support_count;i++){
        const SA3DSECObjectSupport*o=&s->dynamic->supports[i];float y=o->top_y;
        float score=fabsf(s->reference_y-1.0f-y);
        if(y>s->reference_y+2.35f||score>=best||!sa3ds_ec_object_support_inside(o,x,z))continue;
        best=score;result=(SA1Surface){.ground_y=y,.grad_x=-o->normal.x/o->normal.y,.grad_z=-o->normal.z/o->normal.y,.normal=o->normal,
            .slope_radians=sa1_surface_slope_radians(o->normal),.surface_id=-1,.valid=true};
    }
    return result;
}
SA1Contact sa3ds_ec_collision_contact(void*ctx,SA1Vec3 p,SA1Vec3 probe,float range){
    SA3DSECCollision*s=ctx;SA1Contact result={.surface_id=-1};float best=1e30f;
    if(!s||!finite3(p)||!finite3(probe)||!isfinite(range)||range<0||p.x<s->min_x||p.x>s->max_x||p.z<s->min_z||p.z>s->max_z)return result;
    s->queries++;probe=sa1_vec3_normalize(probe);
    if(++s->query_serial==0){memset(s->visited,0,(size_t)s->triangle_count*sizeof(*s->visited));s->query_serial=1;}
    int x0=cell_x(s,p.x-range),x1=cell_x(s,p.x+range),z0=cell_z(s,p.z-range),z1=cell_z(s,p.z+range);
    for(int z=z0;z<=z1;z++)for(int x=x0;x<=x1;x++){
      unsigned c=(unsigned)z*16u+(unsigned)x;
      for(uint32_t j=s->cells[c];j<s->cells[c+1];j++){
        uint32_t ti=s->references[j];if(s->visited[ti]==s->query_serial)continue;s->visited[ti]=s->query_serial;
        SA3DSECTriangle*t=&s->triangles[ti];s->triangle_tests++;
        float d=sa1_vec3_dot(sa1_vec3_sub(p,t->a),t->normal);
        if(t->normal.y<=0.45f||d< -0.35f||d>range||sa1_vec3_dot(probe,t->normal)>-0.15f)continue;
        SA1Vec3 hit=sa1_vec3_sub(p,sa1_vec3_scale(t->normal,d));
        float score=fabsf(d-1.0f);
        if(score<best&&inside(t,hit)){
            best=score;result=(SA1Contact){.point=hit,.normal=t->normal,
                .signed_distance=d,.distance=fabsf(d),.surface_id=(int32_t)ti,
                .surface_flags=t->flags,.valid=true};
        }
        /* Offset planes of adjacent slopes leave a small convex gap around
         * their shared edge. Accept only marked shared support edges; boundary
         * edges remain unsupported and still detach. */
        for(unsigned e=0;e<3;e++)if(t->support_edge_mask&(1u<<e)){
            SA1Vec3 a=e==0?t->a:(e==1?t->b:t->c);
            SA1Vec3 b=e==0?t->b:(e==1?t->c:t->a);
            SA1Vec3 edge=sa1_vec3_sub(b,a);float ee=sa1_vec3_dot(edge,edge);
            float q=ee>1e-8f?clamp(sa1_vec3_dot(sa1_vec3_sub(p,a),edge)/ee,0,1):0;
            SA1Vec3 edge_hit=sa1_vec3_add(a,sa1_vec3_scale(edge,q));
            float edge_distance=sa1_vec3_length(sa1_vec3_sub(p,edge_hit));
            float edge_score=fabsf(edge_distance-1.0f);
            if(edge_distance>range||edge_score>=best)continue;
            best=edge_score;result=(SA1Contact){.point=edge_hit,.normal=t->normal,
                .signed_distance=edge_distance,.distance=edge_distance,.surface_id=(int32_t)ti,
                .surface_flags=t->flags,.valid=true};
        }
      }
    }
    for(uint32_t i=0;s->dynamic&&i<s->dynamic->support_count;i++){
        const SA3DSECObjectSupport*o=&s->dynamic->supports[i];
        float d=p.y-o->top_y;
        if(d< -0.35f||d>range||probe.y>-0.15f||!sa3ds_ec_object_support_inside(o,p.x,p.z))continue;
        float score=fabsf(d-1.0f);if(score>=best)continue;
        best=score;result=(SA1Contact){.point={p.x,o->top_y,p.z},.normal=o->normal,.signed_distance=d,.distance=fabsf(d),
            .surface_id=-1,.surface_flags=1,.valid=true};
    }
    return result;
}
int sa3ds_ec_collision_spawn(SA3DSECCollision*s,float x,float z,float hint_y){
    if(!s||!isfinite(hint_y))return 0;
    s->reference_y=hint_y;SA1Surface floor=sa3ds_ec_collision_surface(s,x,z);
    if(!floor.valid)return 0;
    s->world.reset_position=(SA1Vec3){x,floor.ground_y+1.0f,z};return 1;
}

/* Port contact safety, NOT reconstructed retail polygon arbitration. The
 * inherited controller only probes its endpoint; a descending tick can cross
 * a floor completely. Stop at the earliest finite imported support crossing.
 * No fabricated floor, expanded snap range, or movement tuning is involved. */
static void resolve_floor_crossing(SA3DSECCollision*s,SA1Vec3 previous,SA1PlayerState*p){
    if(p->grounded||p->pos.y>=previous.y)return;
    if(++s->query_serial==0){memset(s->visited,0,(size_t)s->triangle_count*sizeof(*s->visited));s->query_serial=1;}
    float best=2.0f;int32_t hit_id=-1;SA1Vec3 motion=sa1_vec3_sub(p->pos,previous);
    /* Contact point is at most SUPPORT away from the center's swept bounds. */
    float r=SA3DS_EC_CONTROLLER_SUPPORT;
    int x0=cell_x(s,fminf(previous.x,p->pos.x)-r),x1=cell_x(s,fmaxf(previous.x,p->pos.x)+r);
    int z0=cell_z(s,fminf(previous.z,p->pos.z)-r),z1=cell_z(s,fmaxf(previous.z,p->pos.z)+r);
    s->queries++;
    for(int z=z0;z<=z1;z++)for(int x=x0;x<=x1;x++){
        unsigned c=z*16+x;
        for(uint32_t j=s->cells[c];j<s->cells[c+1];j++){
            uint32_t ti=s->references[j];if(s->visited[ti]==s->query_serial)continue;
            s->visited[ti]=s->query_serial;s->triangle_tests++;
            SA3DSECTriangle*t=&s->triangles[ti];if(t->normal.y<=0.45f)continue;
            float a=sa1_vec3_dot(sa1_vec3_sub(previous,t->a),t->normal)-r;
            float b=sa1_vec3_dot(sa1_vec3_sub(p->pos,t->a),t->normal)-r;
            /* A tick starting on the support plane may be leaving its ledge;
             * do not reattach at t=0 to a polygon behind the player. */
            if(a<=0||b>0||a<=b)continue;
            float fraction=a/(a-b);if(fraction>=best)continue;
            SA1Vec3 center=sa1_vec3_add(previous,sa1_vec3_scale(motion,fraction));
            if(!inside(t,sa1_vec3_sub(center,sa1_vec3_scale(t->normal,r))))continue;
            best=fraction;hit_id=(int32_t)ti;
        }
    }
    if(hit_id<0)return;
    SA3DSECTriangle*t=&s->triangles[hit_id];
    p->pos=sa1_vec3_add(previous,sa1_vec3_scale(motion,best));
    float inward=sa1_vec3_dot(p->vel,t->normal);
    if(inward<0)p->vel=sa1_vec3_sub(p->vel,sa1_vec3_scale(t->normal,inward));
    SA1SurfaceFrame frame=sa1_surface_frame_make(t->normal,(SA1Vec3){sinf(p->yaw),0,cosf(p->yaw)});
    p->ground_normal=frame.up;p->surface_forward=frame.forward;p->surface_right=frame.right;
    p->ground_slope_radians=sa1_surface_slope_radians(frame.up);
    p->surface_id=hit_id;p->surface_flags=t->flags;p->support_distance=r;
    p->grounded=true;p->jump_ticks=0;
    if(p->action_state==SA1_ACTION_AIRBORNE){p->action_state=SA1_ACTION_GROUNDED;p->ball_form=false;}
}

static SA1Vec3 edge_point(const SA3DSECTriangle*t,unsigned e){
    return e==0?t->a:(e==1?t->b:t->c);
}
static float segment_parameter(SA1Vec3 p,SA1Vec3 a,SA1Vec3 b){
    SA1Vec3 e=sa1_vec3_sub(b,a);float d=sa1_vec3_dot(e,e);
    return d>1e-8f?clamp(sa1_vec3_dot(sa1_vec3_sub(p,a),e)/d,0,1):0;
}
static SA1Vec3 closest_segment(SA1Vec3 p,SA1Vec3 a,SA1Vec3 b){
    return sa1_vec3_add(a,sa1_vec3_scale(sa1_vec3_sub(b,a),segment_parameter(p,a,b)));
}
static void sphere_root(SA1Vec3 m,SA1Vec3 v,float radius,float*qbest){
    float aa=sa1_vec3_dot(v,v),bb=2*sa1_vec3_dot(m,v),cc=sa1_vec3_dot(m,m)-radius*radius;
    float disc=bb*bb-4*aa*cc;if(aa<1e-8f||disc<0)return;
    float q=(-bb-sqrtf(disc))/(2*aa);if(q>=0&&q<=1&&q<*qbest)*qbest=q;
}
/* Earliest moving-sphere hit against a finite edge capsule. This is a bounded
 * port contact primitive, not a claim about PC2004 polygon arbitration. */
static int sweep_edge(SA1Vec3 previous,SA1Vec3 current,SA1Vec3 a,SA1Vec3 b,
    float radius,float*fraction,SA1Vec3*normal){
    SA1Vec3 motion=sa1_vec3_sub(current,previous),edge=sa1_vec3_sub(b,a);
    float ee=sa1_vec3_dot(edge,edge);if(ee<1e-8f)return 0;
    SA1Vec3 old_delta=sa1_vec3_sub(previous,closest_segment(previous,a,b));
    SA1Vec3 now_delta=sa1_vec3_sub(current,closest_segment(current,a,b));
    float old2=sa1_vec3_dot(old_delta,old_delta),now2=sa1_vec3_dot(now_delta,now_delta);
    float best=2;
    if(old2<=radius*radius){
        if(now2>=old2)return 0;
        best=0;
    }else{
        float me=sa1_vec3_dot(sa1_vec3_sub(previous,a),edge)/ee;
        float ve=sa1_vec3_dot(motion,edge)/ee;
        /* Use the SAME start-distance projection as the overlap test. Two
         * algebraically equal float projections can straddle radius^2 and
         * turn a boundary entry into a negative root (missed collision). */
        SA1Vec3 mp=me>=0&&me<=1?old_delta:
            sa1_vec3_sub(sa1_vec3_sub(previous,a),sa1_vec3_scale(edge,me));
        SA1Vec3 vp=sa1_vec3_sub(motion,sa1_vec3_scale(edge,ve));
        float aa=sa1_vec3_dot(vp,vp),bb=2*sa1_vec3_dot(mp,vp),cc=sa1_vec3_dot(mp,mp)-radius*radius;
        float disc=bb*bb-4*aa*cc;
        if(aa>1e-8f&&disc>=0){
            float q=(-bb-sqrtf(disc))/(2*aa),along=me+ve*q;
            if(q>=0&&q<=1&&along>=0&&along<=1)best=q;
        }
        sphere_root(sa1_vec3_sub(previous,a),motion,radius,&best);
        sphere_root(sa1_vec3_sub(previous,b),motion,radius,&best);
    }
    if(best>1)return 0;
    SA1Vec3 hit=sa1_vec3_add(previous,sa1_vec3_scale(motion,best));
    SA1Vec3 n=sa1_vec3_sub(hit,closest_segment(hit,a,b));
    if(sa1_vec3_length(n)<1e-5f)return 0;
    n=sa1_vec3_normalize(n);
    if(sa1_vec3_dot(motion,n)>=0)return 0;
    *fraction=best;*normal=n;return 1;
}

void sa3ds_ec_collision_resolve_walls(SA3DSECCollision*s,SA1Vec3 previous,SA1PlayerState*p){
    if(!s||!p||!finite3(previous)||!finite3(p->pos))return;
    resolve_floor_crossing(s,previous,p);
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
    int x0=cell_x(s,fminf(previous.x,center.x)-radius),x1=cell_x(s,fmaxf(previous.x,center.x)+radius);
    int z0=cell_z(s,fminf(previous.z,center.z)-radius),z1=cell_z(s,fmaxf(previous.z,center.z)+radius);
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
    /* Face projection is insufficient at finite polygon corners. Sweep only
     * exposed/sharp edges; coplanar internal diagonals were removed at load. */
    if(++s->query_serial==0){memset(s->visited,0,(size_t)s->triangle_count*sizeof(*s->visited));s->query_serial=1;}
    float best=2;SA1Vec3 edge_normal={0};
    s->queries++;
    for(int z=z0;z<=z1;z++)for(int x=x0;x<=x1;x++){
        unsigned c=z*16+x;
        for(uint32_t j=s->cells[c];j<s->cells[c+1];j++){
            uint32_t ti=s->references[j];if(s->visited[ti]==s->query_serial)continue;s->visited[ti]=s->query_serial;
            SA3DSECTriangle*t=&s->triangles[ti];if(fabsf(t->normal.y)>0.45f||!t->wall_edge_mask)continue;
            s->triangle_tests++;
            for(unsigned e=0;e<3;e++)if(t->wall_edge_mask&(1u<<e)){
                float q;SA1Vec3 n;
                if(sweep_edge(previous,center,edge_point(t,e),edge_point(t,(e+1)%3),radius,&q,&n)&&q<best){best=q;edge_normal=n;}
            }
        }
    }
    if(best<=1){
        SA1Vec3 hit=sa1_vec3_add(previous,sa1_vec3_scale(sa1_vec3_sub(center,previous),best));
        SA1Vec3 correction=sa1_vec3_sub(hit,center);p->pos=sa1_vec3_add(p->pos,correction);center=hit;
        float inward=sa1_vec3_dot(p->vel,edge_normal);
        if(inward<0)p->vel=sa1_vec3_sub(p->vel,sa1_vec3_scale(edge_normal,inward));
        s->wall_hits++;
    }
    /* The controller resolves its floor BEFORE this wall pass. An inclined
     * wall can then push a supported actor underneath that same floor. Repair
     * that adapter-ordering inconsistency using imported support, not gravity
     * or movement tuning. Do not snap airborne actors or invent missing floors.
     * The exact retail joint-polygon arbitration remains unresolved; the
     * finite edge capsules above are a bounded port safety correction. */
    if(p->grounded){
        SA1Vec3 probe=sa1_vec3_scale(actor_frame(p).up,-1);
        SA1Contact floor=sa3ds_ec_collision_contact(s,p->pos,probe,2.35f);
        if(!floor.valid){
            /* A leaning wall can push the center below the contact probe's
             * negative tolerance. Reuse the pre-step reference-height surface
             * query before deciding support is absent. */
            SA1Surface surface=sa3ds_ec_collision_surface(s,p->pos.x,p->pos.z);
            if(surface.valid){
                SA1Vec3 point={p->pos.x,surface.ground_y,p->pos.z};
                float distance=sa1_vec3_dot(sa1_vec3_sub(p->pos,point),surface.normal);
                SA3DSECTriangle*t=&s->triangles[surface.surface_id];
                floor=(SA1Contact){.point=point,.normal=surface.normal,.signed_distance=distance,
                    .distance=fabsf(distance),.surface_id=surface.surface_id,.surface_flags=t->flags,.valid=true};
            }
        }
        if(!floor.valid){
            p->grounded=false;if(p->action_state!=SA1_ACTION_HURT&&p->action_state!=SA1_ACTION_DEAD)p->action_state=SA1_ACTION_AIRBORNE;p->jump_ticks=0;
        }else{
            if(floor.signed_distance<SA3DS_EC_CONTROLLER_SUPPORT){
                p->pos=sa1_vec3_add(p->pos,sa1_vec3_scale(floor.normal,
                    SA3DS_EC_CONTROLLER_SUPPORT-floor.signed_distance));
                float inward=sa1_vec3_dot(p->vel,floor.normal);
                if(inward<0)p->vel=sa1_vec3_sub(p->vel,sa1_vec3_scale(floor.normal,inward));
            }
            SA1SurfaceFrame support=sa1_surface_frame_make(floor.normal,p->surface_forward);
            p->ground_normal=support.up;p->surface_forward=support.forward;p->surface_right=support.right;
            p->ground_slope_radians=sa1_surface_slope_radians(support.up);
            p->surface_id=floor.surface_id;p->surface_flags=floor.surface_flags;
            p->support_distance=floor.signed_distance;
        }
    }
}
