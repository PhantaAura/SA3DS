#include "whale_shape.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){
 if(argc<2)return 1;
 FILE*f=fopen(argv[1],"rb");if(!f)return 2;fseek(f,0,SEEK_END);long size=ftell(f);rewind(f);
 void*data=malloc(size);if(!data||fread(data,1,size,f)!=(size_t)size)return 3;fclose(f);
 SA3DSWhaleShape s;int valid=sa3ds_whale_shape_open(&s,data,size);
 if(argc==2){free(data);return valid?4:0;} /*Malformed-package mode.*/
 if(!valid)return 5;
 f=fopen(argv[2],"wb");if(!f)return 6;
 float(*p)[3]=malloc(s.vertices*12),(*n)[3]=malloc(s.vertices*12);if(!p||!n)return 7;
 if(fwrite(&s.vertices,4,1,f)!=1)return 8;
 for(unsigned b=0;b<2;b++){
  if(fwrite(&s.banks[b].duration,4,1,f)!=1)return 8;
  const SA3DSWhaleShapeChannel*channels[]={&s.banks[b].position,&s.banks[b].normal};
  for(unsigned k=0;k<2;k++){
   const SA3DSWhaleShapeChannel*c=channels[k];if(fwrite(&c->count,4,1,f)!=1)return 8;
   for(unsigned key=0;key<c->count;key++){
    const unsigned char*row=c->keys+key*8;uint32_t off=(uint32_t)row[4]|((uint32_t)row[5]<<8)|((uint32_t)row[6]<<16)|((uint32_t)row[7]<<24);
    if(fwrite(row,4,1,f)!=1||fwrite(s.data+off,12,s.vertices,f)!=s.vertices)return 8;
   }
  }
  for(unsigned tick=0;tick<=s.banks[b].duration*8;tick++){
   if(!sa3ds_whale_shape_sample(&s,b,tick*.125f,p,n,s.vertices))return 9;
   if(fwrite(p,12,s.vertices,f)!=s.vertices||fwrite(n,12,s.vertices,f)!=s.vertices)return 8;
  }
 }
 free(p);free(n);free(data);return fclose(f)?10:0;
}
