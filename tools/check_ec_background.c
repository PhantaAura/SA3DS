/* Read-only owner-generated original-background reader check. */
#include "emerald_coast_background.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){
 if(argc!=2){fprintf(stderr,"usage: check_ec_background OWNER_BACKGROUND_PACKAGE\n");return 2;}
 FILE*f=fopen(argv[1],"rb");if(!f)return 1;
 if(fseek(f,0,SEEK_END)){fclose(f);return 1;}long n=ftell(f);
 if(n<=0||(unsigned long)n>SA3DS_EC_BACKGROUND_MAX_BYTES){fclose(f);return 1;}rewind(f);
 void*b=malloc((size_t)n);if(!b){fclose(f);return 1;}
 int ok=fread(b,1,(size_t)n,f)==(size_t)n;if(fclose(f))ok=0;
 SA3DSECBackground bg;if(!ok||!sa3ds_ec_background_open(&bg,b,(size_t)n)){free(b);return 1;}
 printf("EC background OK: models=%u meshes=%u textures=%u triangles=%u\n",bg.visual.model_count,bg.visual.mesh_count,bg.visual.textures.texture_count,bg.visual.index_count/3);
 for(unsigned act=0;act<2;act++)for(unsigned clip=0;clip<3;clip++){
  SA3DSECBackgroundConfig c;sa3ds_ec_background_select(&bg,act,clip,&c);
  printf("act%u clip%u scale=(%.9g,%.9g,%.9g) bgclip=(%.9g,%.9g) terrainclip=(%.9g,%.9g)\n",act,clip,c.scale[0],c.scale[1],c.scale[2],c.background_near,c.background_far,c.terrain_near,c.terrain_far);
 }
 free(b);return 0;
}
