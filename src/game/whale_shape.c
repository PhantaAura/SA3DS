#include "whale_shape.h"
#include "sa3dspak.h"
#include <math.h>
#include <string.h>
static uint32_t r32(const uint8_t*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static float rf(const uint8_t*p){uint32_t u=r32(p);float f;memcpy(&f,&u,4);return f;}
static int channel(const uint8_t*b,size_t size,uint32_t vertices,uint32_t offset,uint32_t count,SA3DSWhaleShapeChannel*out,uint32_t duration){
 if(count<2||count>64||offset<80||offset>size||count>(size-offset)/8)return 0;
 uint32_t previous=0;
 for(uint32_t k=0;k<count;k++){
  const uint8_t*row=b+offset+k*8;uint32_t frame=r32(row),v=r32(row+4);
  if((k?frame<=previous:frame!=0)||frame>duration||v<80||v>size||vertices>(size-v)/12)return 0;
  for(uint32_t j=0;j<vertices*3;j++)if(!isfinite(rf(b+v+j*4)))return 0;
  previous=frame;
 }
 *out=(SA3DSWhaleShapeChannel){b+offset,count};return 1;
}
int sa3ds_whale_shape_open(SA3DSWhaleShape*out,const void*bytes,size_t size){
 if(!out)return 0;
 memset(out,0,sizeof(*out));SA3DSPak pak;SA3DSPakChunk c,m;
 if(!bytes||size>4u*1024u*1024u||sa3dspak_open(&pak,bytes,size)!=SA3DSPAK_OK||pak.toc_count!=2||
    sa3dspak_find(&pak,"METADATA",&m)!=SA3DSPAK_OK||m.flags||
    sa3dspak_find(&pak,"WHALE_SHAPE",&c)!=SA3DSPAK_OK||c.flags||c.size<80)return 0;
 const uint8_t*b=c.data;SA3DSWhaleShape next={.data=b,.size=(size_t)c.size,.vertices=r32(b+16)};
 if(memcmp(b,"SA3WSHP\0",8)||r32(b+8)!=1||r32(b+12)!=2||!next.vertices||next.vertices>1024||
    r32(b+20)!=32||r32(b+24)!=24||r32(b+28)!=c.size)return 0;
 for(unsigned bank=0;bank<2;bank++){
  const uint8_t*row=b+32+bank*24;SA3DSWhaleShapeBank*p=&next.banks[bank];p->duration=r32(row);
  if(!p->duration||p->duration>4096||r32(row+20)||
     !channel(b,next.size,next.vertices,r32(row+8),r32(row+4),&p->position,p->duration)||
     !channel(b,next.size,next.vertices,r32(row+16),r32(row+12),&p->normal,p->duration))return 0;
 }
 *out=next;return 1;
}
typedef struct {const uint8_t *first,*second;float weight;} Sample;
static Sample select_channel(const SA3DSWhaleShape*s,const SA3DSWhaleShapeChannel*c,uint32_t duration,float frame){
 uint32_t lo=0,hi=c->count,integer=(uint32_t)frame;
 while(hi-lo>1){uint32_t mid=(lo+hi)/2;if(r32(c->keys+mid*8)<=integer)lo=mid;else hi=mid;}
 uint32_t next=(lo+1)%c->count;
 const uint8_t*a=c->keys+lo*8,*b=c->keys+next*8;
 uint32_t first=r32(a),second=r32(b),delta=second-first;
 /*4054C0 converts a negative signed difference to unsigned magnitude before
  *the float store; last->first therefore uses a huge denominator, not the
  *usual duration-last+first. Preserve the source quirk.*/
 float denominator=(float)(delta?delta:duration-second);
 Sample result={s->data+r32(a+4),s->data+r32(b+4),(float)(((double)frame-first)/denominator)};return result;
}
int sa3ds_whale_shape_sample(const SA3DSWhaleShape*s,unsigned bank,float frame,
 float(*positions)[3],float(*normals)[3],size_t capacity){
 if(!s||!s->data||bank>1||!positions||!normals||capacity<s->vertices||!isfinite(frame)||frame<0||frame>=4294967296.0)return 0;
 const SA3DSWhaleShapeBank*b=&s->banks[bank];
 frame=(float)((double)frame-((uint32_t)frame/b->duration)*(double)b->duration);
 Sample p=select_channel(s,&b->position,b->duration,frame),n=select_channel(s,&b->normal,b->duration,frame);
 for(uint32_t v=0;v<s->vertices;v++){
  double nz=0;
  for(unsigned k=0;k<3;k++){
   uint32_t offset=(v*3+k)*4;
   positions[v][k]=(float)((1.0-p.weight)*rf(p.first+offset)+(double)p.weight*rf(p.second+offset));
   double value=(1.0-n.weight)*rf(n.first+offset)+(double)n.weight*rf(n.second+offset);
   normals[v][k]=(float)value;if(k==2)nz=value;
  }
  /*Z remains extended for length, while X/Y have already been stored. The
   *final scaling then reloads all three stored float components.*/
  double length=(double)normals[v][0]*normals[v][0]+(double)normals[v][1]*normals[v][1]+nz*nz;
  if(length>(double)1e-5f){double inverse=1.0/sqrt(length);for(unsigned k=0;k<3;k++)normals[v][k]=(float)(inverse*normals[v][k]);}
  else {normals[v][0]=normals[v][1]=0;normals[v][2]=1;}
 }
 return 1;
}
