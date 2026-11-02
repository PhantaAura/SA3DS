#include "texture_override.h"
#include <string.h>
static uint16_t r16(const uint8_t*p){return(uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static uint32_t r32(const uint8_t*p){return(uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static uint32_t checksum(const uint8_t*p,size_t size){
    uint32_t crc=UINT32_MAX;
    for(size_t i=0;i<size;i++){
        crc^=(i>=28&&i<32)?0:p[i];
        for(unsigned b=0;b<8;b++)crc=(crc>>1)^(UINT32_C(0xedb88320)&(uint32_t)-(int32_t)(crc&1u));
    }
    return ~crc;
}
static int dimension(uint16_t n){return n>=8&&n<=256&&!(n&(n-1u));}
SA3DSTextureOverrideResult sa3ds_texture_override_open(
    SA3DSTextureOverride*out,const void*data,size_t size,uint32_t act,
    const SA3TArchive*baseline){
    if(!out)return SA3DS_TEXTURE_OVERRIDE_ARGUMENT;
    memset(out,0,sizeof(*out));
    if(!data||!baseline||!baseline->data)return SA3DS_TEXTURE_OVERRIDE_ARGUMENT;
    const uint8_t*p=data;
    if(size<64||size>SA3DS_TEXTURE_OVERRIDE_MAX_BYTES||memcmp(p,"SA3DMOD\0",8)||
       r16(p+8)!=1||r16(p+10)!=0||r32(p+12)!=64||r32(p+16)!=size)
        return SA3DS_TEXTURE_OVERRIDE_HEADER;
    if(r32(p+28)!=checksum(p,size))return SA3DS_TEXTURE_OVERRIDE_INTEGRITY;
    uint32_t target_act=r32(p+20),id=r32(p+24);
    if(act>1||target_act!=act||id>=baseline->texture_count||
       memcmp(p+32,baseline->source_sha256,32))return SA3DS_TEXTURE_OVERRIDE_TARGET;
    SA3TArchive archive;SA3TTexture replacement,original;const uint8_t*rgba;
    if(sa3t_open(&archive,p+64,size-64)!=SA3T_OK||archive.texture_count!=1||
       sa3t_texture(&archive,0,&replacement,&rgba)!=SA3T_OK||
       sa3t_texture(baseline,id,&original,&rgba)!=SA3T_OK||
       !dimension(replacement.width)||!dimension(replacement.height)||
       replacement.width!=original.width||replacement.height!=original.height||
       replacement.flags!=0)return SA3DS_TEXTURE_OVERRIDE_TEXTURE;
    out->act=target_act;out->texture_id=id;out->texture=archive;
    return SA3DS_TEXTURE_OVERRIDE_OK;
}
const char*sa3ds_texture_override_result_name(SA3DSTextureOverrideResult r){
    static const char*names[]={"OK","ARGUMENT","HEADER","INTEGRITY","TARGET","TEXTURE"};
    return (unsigned)r<sizeof(names)/sizeof(names[0])?names[r]:"UNKNOWN";
}
