#include <stdio.h>
#include <stdlib.h>
#include "sa3t_texture.h"
static unsigned char*readall(const char*p,size_t*n){FILE*f=fopen(p,"rb");if(!f)return 0;fseek(f,0,SEEK_END);long z=ftell(f);rewind(f);unsigned char*b=malloc((size_t)z);if(!b||fread(b,1,(size_t)z,f)!=(size_t)z){free(b);fclose(f);return 0;}fclose(f);*n=(size_t)z;return b;}
int main(int argc,char**argv){if(argc!=2)return 2;size_t n;unsigned char*b=readall(argv[1],&n);if(!b)return 2;SA3TArchive a;SA3TResult r=sa3t_open(&a,b,n);if(r!=SA3T_OK){fprintf(stderr,"open %s\n",sa3t_result_name(r));return 1;}if(a.texture_count!=16)return 1;unsigned long long total=0;for(unsigned i=0;i<a.texture_count;i++){SA3TTexture t;const uint8_t*p=0;if(sa3t_texture(&a,i,&t,&p)!=SA3T_OK||!p||!t.width||!t.height)return 1;total+=t.data_size;}if(total!=232192ull)return 1;printf("SA3T real Sonic RGBA8 texture package: PASS (%u textures, %llu pixel bytes)\n",a.texture_count,total);free(b);return 0;}
