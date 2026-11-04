#include "whale_ccl.h"
#include "sa3dspak.h"
#include <math.h>
#include <string.h>
static uint32_t r32(const unsigned char*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const unsigned char*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static int finite3(SA1Vec3 p){return isfinite(p.x)&&isfinite(p.y)&&isfinite(p.z);}
int sa3ds_whale_ccl_assets_open(SA3DSWhaleCCLAssets*out,const void*bytes,size_t size){
 if(!out)return 0;
 memset(out,0,sizeof(*out));SA3DSPak p;SA3DSPakChunk c,m;
 if(!bytes||size>65536||sa3dspak_open(&p,bytes,size)!=SA3DSPAK_OK||p.toc_count!=2||
    sa3dspak_find(&p,"METADATA",&m)!=SA3DSPAK_OK||m.flags||
    sa3dspak_find(&p,"WHALE_CCL",&c)!=SA3DSPAK_OK||c.flags||c.size!=144)return 0;
 const unsigned char*b=c.data;
 if(memcmp(b,"SA3WCCL\0",8)||r32(b+8)!=1||r32(b+12)!=2||r32(b+16)!=48||
    r32(b+20)!=48||r32(b+24)!=144||r32(b+28)||b[32]!=2||r32(b+44))return 0;
 SA3DSWhaleCCLAssets next={0};next.rules=(SA3DSCCLRules){b[33],b[34],b[35],r32(b+36)};
 next.broad_radius=rf(b+40);if(!isfinite(next.broad_radius)||next.broad_radius<0)return 0;
 for(unsigned k=0;k<2;k++){
  const unsigned char*r=b+48+k*48;
  if(r[0]||r[1]||!sa3ds_ccl_shape_decode(&next.shapes[k],r,(SA1Vec3){0,0,0}))return 0;
  /*Only the proven absolute-tip / zero-local-origin sphere profile. Reject
   *nonzero local centers or unsupported dimensions/rotations.*/
  for(unsigned j=8;j<20;j+=4)if(rf(r+j)!=0)return 0;
  for(unsigned j=24;j<48;j+=4)if(r32(r+j))return 0;
  if((next.shapes[k].attributes&0x20)!=(k?0u:0x20u))return 0;
 }
 *out=next;return 1;
}
int sa3ds_whale_ccl_init(SA3DSWhaleCCL*out,const SA3DSWhaleCCLAssets*a,uint32_t id,SA1Vec3 p){
 if(!out||!a||!id||!finite3(p)||!isfinite(a->broad_radius)||a->broad_radius<0)return 0;
 SA3DSWhaleCCL next={0};sa3ds_ccl_body_init(&next.body,id);
 next.body.rules=a->rules;next.body.broad_radius=a->broad_radius;next.body.task_position=p;
 next.body.outer_flags=0x8040;memcpy(next.shapes,a->shapes,sizeof(next.shapes));
 sa3ds_ccl_body_clear(&next.body,next.shapes,2);
 next.shapes[1].attributes|=0x10;next.shapes[1].center=p;
 *out=next;return 1;
}
void sa3ds_whale_ccl_emerge(SA3DSWhaleCCL*c){if(c)c->shapes[1].attributes&=~0x10u;}
int sa3ds_whale_ccl_publish(SA3DSWhaleCCL*c,SA1Vec3 p,SA1Vec3 tip){
 if(!c||!c->body.hits.actor_id||!finite3(p)||!finite3(tip))return 0;
 c->body.task_position=p;c->shapes[0].center=tip;c->shapes[1].center=p;return 1;
}
