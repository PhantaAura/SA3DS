/* Owner-side read-only checker for a real stage package + texture override. */
#include "texture_override.h"
#include "sa3dspak.h"
#include <stdio.h>
#include <stdlib.h>
static void*read(const char*path,size_t cap,size_t*size){
    FILE*f=fopen(path,"rb");if(!f)return NULL;
    if(fseek(f,0,SEEK_END)){fclose(f);return NULL;}long n=ftell(f);
    if(n<=0||(unsigned long)n>cap){fclose(f);return NULL;}rewind(f);
    void*b=malloc((size_t)n);if(!b){fclose(f);return NULL;}
    int ok=fread(b,1,(size_t)n,f)==(size_t)n;if(fclose(f))ok=0;
    if(!ok){free(b);return NULL;}*size=(size_t)n;return b;
}
int main(int argc,char**argv){
    if(argc!=4){fprintf(stderr,"usage: check_texture_override OWNER_STAGE_PACKAGE ACT OWNER_OVERRIDE\n");return 2;}
    char*end;long act=strtol(argv[2],&end,10);if(*end||act<0||act>1)return 2;
    size_t bs,ms;void*b=read(argv[1],64u*1024u*1024u,&bs);
    void*m=read(argv[3],SA3DS_TEXTURE_OVERRIDE_MAX_BYTES,&ms);
    SA3DSPak pak;SA3DSPakChunk chunk;SA3TArchive base;SA3DSTextureOverride view;
    if(!b||!m||sa3dspak_open(&pak,b,bs)!=SA3DSPAK_OK||
       sa3dspak_find(&pak,"TEXTURES",&chunk)!=SA3DSPAK_OK||
       sa3t_open(&base,chunk.data,(size_t)chunk.size)!=SA3T_OK){free(b);free(m);return 1;}
    SA3DSTextureOverrideResult r=sa3ds_texture_override_open(&view,m,ms,(uint32_t)act,&base);
    printf("texture override: %s",sa3ds_texture_override_result_name(r));
    if(r==SA3DS_TEXTURE_OVERRIDE_OK){SA3TTexture t;const uint8_t*p;
        if(sa3t_texture(&view.texture,0,&t,&p)!=SA3T_OK){free(b);free(m);return 1;}
        printf(" act=%u texture=%u %ux%u rgbaBytes=%u",view.act,view.texture_id,t.width,t.height,t.data_size);
    }
    puts("");free(b);free(m);return r==SA3DS_TEXTURE_OVERRIDE_OK?0:1;
}
