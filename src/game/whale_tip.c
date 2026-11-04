#include "whale_tip.h"
#include "sa3dspak.h"
#include <math.h>
#include <string.h>
static uint32_t r32(const unsigned char*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static int32_t ri(const unsigned char*p){uint32_t u=r32(p);int32_t i;memcpy(&i,&u,4);return i;}
int sa3ds_whale_tip_assets_open(SA3DSWhaleTipAssets*out,const void*bytes,size_t size){
 if(!out)return 0;
 memset(out,0,sizeof(*out));SA3DSPak p;SA3DSPakChunk c,m;
 if(!bytes||size>65536||sa3dspak_open(&p,bytes,size)!=SA3DSPAK_OK||p.toc_count!=2||
    sa3dspak_find(&p,"METADATA",&m)!=SA3DSPAK_OK||m.flags||
    sa3dspak_find(&p,"WHALE_TIP",&c)!=SA3DSPAK_OK||c.flags||c.size!=160)return 0;
 const unsigned char*b=c.data;
 uint32_t version=r32(b+8),profile=r32(b+28);
 if(memcmp(b,"SA3WTIP\0",8)||!((version==1&&profile==0)||(version==2&&profile==1))||r32(b+12)!=32||r32(b+16)!=32||r32(b+20)!=4||r32(b+24)!=160)return 0;
 /*Owner source uses signed BAMS keys. Reject unsupported unbounded input
  *rather than converting an out-of-range interpolated double to integer.*/
 for(unsigned k=0;k<32;k++)if(ri(b+32+k*4)<-32768||ri(b+32+k*4)>32767)return 0;
 out->pitch=b+32;out->exit_root_bind=version==2;return 1;
}
int sa3ds_whale_tip(const SA3DSWhaleMotion*m,const SA3DSWhaleTipAssets*a,
 const SA3DSWhaleMotionServices*s,float out[3],int32_t*sampled_pitch){
 if(!m||!a||!a->pitch||!s||!s->sine||!out||!isfinite(m->frame)||m->frame<0||m->frame>30||!isfinite(m->draw_y))return 0;
 for(unsigned k=0;k<3;k++)if(!isfinite(m->position[k]))return 0;
 unsigned key=(unsigned)floorf(m->frame),next=key==29?0:key+1;
 float fraction=(float)((double)m->frame-key);
 int32_t first=ri(a->pitch+key*4),second=ri(a->pitch+next*4);
 int32_t pitch=(int32_t)((double)first*(1.0-fraction)+(double)second*fraction);
 double sy=s->sine(s->context,(uint16_t)m->yaw),cy=s->sine(s->context,(uint16_t)((uint32_t)m->yaw+0x4000));
 double sp=s->sine(s->context,(uint16_t)pitch),cp=s->sine(s->context,(uint16_t)((uint32_t)pitch+0x4000));
 if(!isfinite(sy)||!isfinite(cy)||!isfinite(sp)||!isfinite(cp))return 0;
 /*Ninja rotateY stores sine/matrix values to float; rotateX retains cosine
  *during products, then stores each resulting matrix element to float.*/
 float sin_y=(float)sy,cos_y=(float)cy,sin_p=(float)sp;
 float m10=(float)((double)sin_p*sin_y),m12=(float)((double)sin_p*cos_y);
 float m20=(float)(cp*sin_y),m22=(float)(cp*cos_y),m11=(float)cp;
 float local[3]={(float)(-5.0*m10+120.0*m20),
                 (float)(-5.0*m11-120.0*sin_p),
                 (float)(-5.0*m12+120.0*m22)};
 float result[3]={(float)((double)local[0]+m->position[0]),
                  (float)((double)m->draw_y+m->position[1]+local[1]),
                  (float)((double)local[2]+m->position[2])};
 if(m->action==1){result[0]=result[2]=0;result[1]=-100000;}
 memcpy(out,result,sizeof(result));if(sampled_pitch)*sampled_pitch=pitch;return 1;
}
