#include "texture_override.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void*read(const char*root,const char*name,size_t*size){
    char path[2048];assert(snprintf(path,sizeof(path),"%s/%s",root,name)>0);
    FILE*f=fopen(path,"rb");assert(f);assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
    void*b=malloc((size_t)n);assert(b&&fread(b,1,(size_t)n,f)==(size_t)n);assert(!fclose(f));*size=(size_t)n;return b;
}
int main(int argc,char**argv){
    assert(argc==2);size_t n;void*base=read(argv[1],"baseline.sa3t",&n);SA3TArchive original;
    assert(sa3t_open(&original,base,n)==SA3T_OK);
    struct{const char*file;SA3DSTextureOverrideResult result;} cases[]={
        {"valid.sa3mod",SA3DS_TEXTURE_OVERRIDE_OK},{"wrong_act.sa3mod",SA3DS_TEXTURE_OVERRIDE_TARGET},
        {"wrong_hash.sa3mod",SA3DS_TEXTURE_OVERRIDE_TARGET},{"wrong_target.sa3mod",SA3DS_TEXTURE_OVERRIDE_TARGET},
        {"wrong_dimensions.sa3mod",SA3DS_TEXTURE_OVERRIDE_TEXTURE},{"corrupt.sa3mod",SA3DS_TEXTURE_OVERRIDE_INTEGRITY},
        {"truncated.sa3mod",SA3DS_TEXTURE_OVERRIDE_HEADER},{"bad_header.sa3mod",SA3DS_TEXTURE_OVERRIDE_HEADER},
        {"too_large.sa3mod",SA3DS_TEXTURE_OVERRIDE_HEADER}};
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){
        void*b=read(argv[1],cases[i].file,&n);SA3DSTextureOverride view;memset(&view,0xaa,sizeof(view));
        assert(sa3ds_texture_override_open(&view,b,n,0,&original)==cases[i].result);
        if(cases[i].result==SA3DS_TEXTURE_OVERRIDE_OK){
            SA3TTexture t;const uint8_t*rgba;assert(view.act==0&&view.texture_id==0);
            assert(sa3t_texture(&view.texture,0,&t,&rgba)==SA3T_OK&&t.width==8&&rgba[0]==4);
            assert(original.data==base); /* baseline view/data unchanged */
        }else{SA3DSTextureOverride zero={0};assert(!memcmp(&view,&zero,sizeof(view)));}
        free(b);
    }
    free(base);puts("Texture override: PASS (CRC/header/act/archive/index/dimensions/cap, zero-view fallback, immutable baseline)");return 0;
}
