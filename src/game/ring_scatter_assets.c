#include "ring_scatter_assets.h"
#include "../sa3dspak/sa3dspak.h"
#include <stdint.h>
#include <string.h>
static uint32_t r32(const unsigned char*p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int sa3ds_ring_scatter_assets_open(SA3DSRingScatterAssets*out,const void*data,size_t size){
    if(!out)return 0;
    memset(out,0,sizeof(*out));if(!data||size>8192)return 0;
    SA3DSPak pak;SA3DSPakChunk chunk;if(sa3dspak_open(&pak,data,size)!=SA3DSPAK_OK||pak.toc_count>2)return 0;
    unsigned seen=0;
    for(uint32_t n=0;n<pak.toc_count;n++){
        SA3DSPakChunk c;if(sa3dspak_chunk_at(&pak,n,&c)!=SA3DSPAK_OK)return 0;
        unsigned bit=!strcmp(c.type,"SCATTER")?1:!strcmp(c.type,"METADATA")?2:0;
        if(!bit||(seen&bit))return 0;
        seen|=bit;
    }
    if(!(seen&1)||sa3dspak_find(&pak,"SCATTER",&chunk)!=SA3DSPAK_OK||chunk.size!=72)return 0;
    const unsigned char*p=chunk.data;
    if(memcmp(p,"SA3DSCAT",8)||r32(p+8)!=1||r32(p+12))return 0;
    SA3DSCCLShape sphere;if(!sa3ds_ccl_shape_decode(&sphere,p+16,(SA1Vec3){0})||sphere.form!=0||sphere.radius<=0)return 0;
    if(p[67])return 0;
    *out=(SA3DSRingScatterAssets){p+16,{p[64],p[65],p[66],r32(p+68)}};return 1;
}
