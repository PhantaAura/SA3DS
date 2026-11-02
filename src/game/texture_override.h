#ifndef SA3DS_TEXTURE_OVERRIDE_H
#define SA3DS_TEXTURE_OVERRIDE_H
#include <stddef.h>
#include <stdint.h>
#include "sa3t_texture.h"

#define SA3DS_TEXTURE_OVERRIDE_MAX_BYTES (64u+96u+256u*256u*4u)
typedef enum {
    SA3DS_TEXTURE_OVERRIDE_OK=0, SA3DS_TEXTURE_OVERRIDE_ARGUMENT,
    SA3DS_TEXTURE_OVERRIDE_HEADER, SA3DS_TEXTURE_OVERRIDE_INTEGRITY,
    SA3DS_TEXTURE_OVERRIDE_TARGET, SA3DS_TEXTURE_OVERRIDE_TEXTURE
} SA3DSTextureOverrideResult;
typedef struct {
    uint32_t act,texture_id;
    SA3TArchive texture;
} SA3DSTextureOverride;

/* Allocation-free, all-or-nothing view. Input bytes must live until upload.
 * File CRC protects header and payload; target act/archive/index/dimensions
 * must match the immutable original package. Invalid inputs zero the view. */
SA3DSTextureOverrideResult sa3ds_texture_override_open(
    SA3DSTextureOverride *out,const void *data,size_t size,uint32_t act,
    const SA3TArchive *baseline);
const char *sa3ds_texture_override_result_name(SA3DSTextureOverrideResult result);
#endif
