#ifndef SA3DS_SA3T_TEXTURE_H
#define SA3DS_SA3T_TEXTURE_H
#include <stddef.h>
#include <stdint.h>

typedef enum { SA3T_OK=0,SA3T_ERR_ARGUMENT,SA3T_ERR_TRUNCATED,SA3T_ERR_HEADER,SA3T_ERR_RANGE,SA3T_ERR_FORMAT } SA3TResult;
typedef struct { uint16_t width,height,format,flags;uint32_t data_offset,data_size;uint16_t source_pixel_format,source_data_format;uint32_t global_index; } SA3TTexture;
typedef struct { const uint8_t*data;size_t size;uint8_t source_sha256[32];uint32_t texture_count; } SA3TArchive;
SA3TResult sa3t_open(SA3TArchive*out,const void*data,size_t size);
SA3TResult sa3t_texture(const SA3TArchive*a,uint32_t index,SA3TTexture*out,const uint8_t**rgba8);
const char*sa3t_result_name(SA3TResult r);
#endif
