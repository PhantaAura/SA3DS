#include "emerald_coast_background.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void*read(const char*dir,const char*name,size_t*size){
 char path[2048];snprintf(path,sizeof(path),"%s/%s.sa3dspak",dir,name);
 FILE*f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
 void*b=malloc((size_t)n);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);fclose(f);*size=(size_t)n;return b;
}
int main(int argc,char**argv){
 assert(argc==2);size_t size;void*data=read(argv[1],"valid",&size);
 SA3DSECBackground bg;assert(sa3ds_ec_background_open(&bg,data,size));
 assert(bg.visual.model_count==1&&bg.visual.textures.texture_count==1);
 for(unsigned act=0;act<2;act++)for(unsigned clip=0;clip<3;clip++){
  SA3DSECBackgroundConfig c;assert(sa3ds_ec_background_select(&bg,act,clip,&c));
  assert(c.act==act&&c.clip_level==clip&&c.scale[0]==1+act&&c.scale[1]==2+clip&&c.scale[2]==3+act+clip);
  assert(c.background_near==1&&c.background_far==2500+clip&&c.terrain_far==2000&&c.fog_argb==0xff123456u);
  float camera[3]={17,1234,-9};SA3CMatrix m;assert(sa3ds_ec_background_transform(&bg,act,clip,camera,&m));
  assert(m.m[0]==c.scale[0]&&m.m[5]==c.scale[1]&&m.m[10]==c.scale[2]);
  assert(m.m[12]==17&&m.m[13]==0&&m.m[14]==-9&&m.m[15]==1);
  camera[1]=-876;SA3CMatrix other;assert(sa3ds_ec_background_transform(&bg,act,clip,camera,&other));assert(!memcmp(&m,&other,sizeof(m)));
 }
 SA3DSECBackgroundConfig c;assert(!sa3ds_ec_background_select(&bg,2,0,&c));assert(!sa3ds_ec_background_select(&bg,0,3,&c));
 float invalid[3]={NAN,0,0};SA3CMatrix m;assert(!sa3ds_ec_background_transform(&bg,0,0,invalid,&m));
 const char*bad[]={"bad_act","bad_clip","nan_scale","zero_scale","clip_order","fog_order","fog_flag","flags","reserved","bad_nested","bad_count","missing","truncated","bad_crc"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);i++){
  size_t n;void*p=read(argv[1],bad[i],&n);SA3DSECBackground view,zero;
  memset(&zero,0,sizeof(zero));memset(&view,0xaa,sizeof(view));
  assert(!sa3ds_ec_background_open(&view,p,n));assert(!memcmp(&view,&zero,sizeof(view)));free(p);
 }
 free(data);puts("EC background: PASS (nested CRC/format,2acts x3clip,source-order XZ/scale transform,altitude independence,finite/config rejection)");return 0;
}
