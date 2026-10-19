/*
 * Host-only deterministic white-space raster gate for generated SA3C + SA3T.
 *
 * This is not the final 3DS GPU renderer.  It proves that the native package
 * readers, real animated hierarchy, indexed geometry, material texture slots,
 * normalized Basic UVs, and decoded owner-supplied texture pixels compose into
 * a visible character. Generated PPM output is diagnostic and not shipped.
 */
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sa3c_render.h"
#include "sa3t_texture.h"

#define W 400
#define H 240

typedef struct { float x,y,z,u,v; } SV;
typedef struct {
    const SA3CCharacter *c; const SA3TArchive *t;
    uint8_t *rgb; float *depth;
    uint32_t raster_triangles, shaded_pixels, alpha_rejects;
} Raster;

static uint16_t r16(const uint8_t*p){return(uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t r32(const uint8_t*p){return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static unsigned char *readall(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return 0;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);if(z<=0){fclose(f);return 0;}unsigned char*b=malloc((size_t)z);if(!b){fclose(f);return 0;}if(fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return 0;}fclose(f);*n=(size_t)z;return b;}
static void xform(const SA3CMatrix*m,float x,float y,float z,float*out){out[0]=x*m->m[0]+y*m->m[4]+z*m->m[8]+m->m[12];out[1]=x*m->m[1]+y*m->m[5]+z*m->m[9]+m->m[13];out[2]=x*m->m[2]+y*m->m[6]+z*m->m[10]+m->m[14];}
static SV vertex(const SA3CCharacter*c,const SA3CMatrix*m,uint16_t vi){const uint8_t*v=c->data+c->sections[3].offset+(size_t)vi*c->sections[3].stride;float p[3];xform(m,rf(v),rf(v+4),rf(v+8),p);float z=p[2]+18.0f;float f=175.0f;SV o;o.x=W*0.5f+(p[0]-1.2f)*f/z;o.y=H*0.52f-(p[1]+0.3f)*f/z;o.z=z;o.u=sa3c_basic_uv_normalize(rf(v+24));o.v=sa3c_basic_uv_normalize(rf(v+28));return o;}
static float edge(float ax,float ay,float bx,float by,float px,float py){return(px-ax)*(by-ay)-(py-ay)*(bx-ax);}
static int imod(int a,int n){int r=a%n;return r<0?r+n:r;}
static void tri(Raster*r,SV a,SV b,SV c,uint16_t texslot,uint32_t diffuse){
    if(a.z<=0.1f||b.z<=0.1f||c.z<=0.1f)return;
    float area=edge(a.x,a.y,b.x,b.y,c.x,c.y);if(fabsf(area)<1e-8f)return;
    int minx=(int)floorf(fminf(a.x,fminf(b.x,c.x))),maxx=(int)ceilf(fmaxf(a.x,fmaxf(b.x,c.x)));
    int miny=(int)floorf(fminf(a.y,fminf(b.y,c.y))),maxy=(int)ceilf(fmaxf(a.y,fmaxf(b.y,c.y)));
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= W) maxx = W - 1;
    if (maxy >= H) maxy = H - 1;
    if (minx > maxx || miny > maxy) return;
    SA3TTexture t;const uint8_t*pix;if(sa3t_texture(r->t,texslot,&t,&pix)!=SA3T_OK)return;
    float ia=1.0f/a.z,ib=1.0f/b.z,ic=1.0f/c.z;float inv_area=1.0f/area;
    uint8_t dr=(uint8_t)((diffuse>>16)&255),dg=(uint8_t)((diffuse>>8)&255),db=(uint8_t)(diffuse&255);
    /* Preserve legitimate black diffuse instead of silently whitening it. */
    for(int y=miny;y<=maxy;y++)for(int x=minx;x<=maxx;x++){
        float px0=x+0.5f,py0=y+0.5f;float wa=edge(b.x,b.y,c.x,c.y,px0,py0)*inv_area;float wb=edge(c.x,c.y,a.x,a.y,px0,py0)*inv_area;float wc=1.0f-wa-wb;
        if (wa < -1e-6f || wb < -1e-6f || wc < -1e-6f) continue;
        float iz = wa * ia + wb * ib + wc * ic;
        if (iz <= 0) continue;
        float z = 1.0f / iz;
        size_t di = (size_t)y * W + (size_t)x;
        if (z >= r->depth[di]) continue;
        float u=(wa*a.u*ia+wb*b.u*ib+wc*c.u*ic)/iz;float v=(wa*a.v*ia+wb*b.v*ib+wc*c.v*ic)/iz;
        int tx=imod((int)floorf(u*(float)t.width),(int)t.width);int ty=imod((int)floorf(v*(float)t.height),(int)t.height);const uint8_t*s=pix+((size_t)ty*t.width+(size_t)tx)*4u;
        unsigned effective_alpha=((unsigned)s[3]*((diffuse>>24)&255u)+127u)/255u;
        if(effective_alpha<=15u){r->alpha_rejects++;continue;}r->depth[di]=z;uint8_t*out=r->rgb+di*3u;out[0]=(uint8_t)((unsigned)s[0]*dr/255u);out[1]=(uint8_t)((unsigned)s[1]*dg/255u);out[2]=(uint8_t)((unsigned)s[2]*db/255u);r->shaded_pixels++;
    }
    r->raster_triangles++;
}
static int visit(const SA3CDrawCommand*cmd,void*user){Raster*r=user;const SA3CCharacter*c=r->c;const uint8_t*ib=c->data+c->sections[4].offset;const uint8_t*mb=c->data+c->sections[5].offset+(size_t)cmd->material_index*c->sections[5].stride;uint32_t diffuse=r32(mb);
    for(uint32_t i=0;i+2<cmd->index_count;i+=3){uint16_t ia=r16(ib+(size_t)(cmd->first_index+i)*2u),ibv=r16(ib+(size_t)(cmd->first_index+i+1)*2u),ic=r16(ib+(size_t)(cmd->first_index+i+2)*2u);tri(r,vertex(c,&cmd->world,ia),vertex(c,&cmd->world,ibv),vertex(c,&cmd->world,ic),cmd->texture_slot,diffuse);}return 0;}
static int writeppm(const char*p,const uint8_t*rgb){FILE*f=fopen(p,"wb");if(!f)return 0;fprintf(f,"P6\n%d %d\n255\n",W,H);int ok=fwrite(rgb,1,(size_t)W*H*3u,f)==(size_t)W*H*3u;fclose(f);return ok;}
int main(int argc,char**argv){if(argc<4||argc>6){fprintf(stderr,"usage: %s sonic.sa3c sonic.sa3t out.ppm [action] [frame]\n",argv[0]);return 2;}uint16_t action=(uint16_t)(argc>=5?strtoul(argv[4],0,0):0);float frame=argc>=6?strtof(argv[5],0):0.0f;size_t cn,tn;unsigned char*cb=readall(argv[1],&cn),*tb=readall(argv[2],&tn);if(!cb||!tb)return 2;SA3CCharacter c;SA3TArchive ta;if(sa3c_character_open(&c,cb,cn)!=SA3C_OK||sa3c_character_validate_references(&c)!=SA3C_OK||sa3t_open(&ta,tb,tn)!=SA3T_OK)return 1;if(ta.texture_count!=c.sections[6].count)return 1;size_t nn=c.sections[0].count;SA3CTransform*local=calloc(nn,sizeof(*local));SA3CMatrix*world=calloc(nn,sizeof(*world));uint8_t*rgb=malloc((size_t)W*H*3u);float*depth=malloc((size_t)W*H*sizeof(float));if(!local||!world||!rgb||!depth)return 2;for(size_t i=0;i<(size_t)W*H;i++){rgb[i*3]=248;rgb[i*3+1]=250;rgb[i*3+2]=252;depth[i]=FLT_MAX;}uint16_t root;SA3CPoseResult pr=sa3c_pose_sample_action(&c,action,frame,1,local,nn,&root);if(pr!=SA3C_POSE_OK){fprintf(stderr,"sample: %s\n",sa3c_pose_result_name(pr));return 1;}if(sa3c_pose_build_world(&c,root,local,nn,world,nn)!=SA3C_POSE_OK)return 1;Raster r={.c=&c,.t=&ta,.rgb=rgb,.depth=depth};SA3CRenderStats st;if(sa3c_render_emit(&c,root,world,nn,visit,&r,&st)!=SA3C_POSE_OK)return 1;if(r.shaded_pixels<1000||r.raster_triangles<100)return 1;if(!writeppm(argv[3],rgb))return 2;printf("host textured white-space render: PASS action=%u frame=%.3f draws=%u source_tris=%u raster_tris=%u shaded_pixels=%u alpha_rejects=%u\n",action,frame,st.draw_calls,st.triangles,r.raster_triangles,r.shaded_pixels,r.alpha_rejects);free(depth);free(rgb);free(world);free(local);free(tb);free(cb);return 0;}
