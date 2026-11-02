#ifndef SA3DS_EMERALD_COAST_BACKGROUND_H
#define SA3DS_EMERALD_COAST_BACKGROUND_H
#include "emerald_coast_visual.h"
#include "sa3c_pose.h"
#define SA3DS_EC_BACKGROUND_MAX_BYTES (2u*1024u*1024u)
#define SA3DS_EC_BACKGROUND_ROWS 6u
#define SA3DS_EC_BACKGROUND_CAMERA_XZ 1u
#define SA3DS_EC_BACKGROUND_FOG_OFF 2u

typedef struct {
    uint32_t act,clip_level;
    float scale[3];
    /* Positive distance representation of the source's negative clip planes. */
    float background_near,background_far,terrain_near,terrain_far;
    float fog_start,fog_end;
    uint32_t fog_argb,fog_enabled,clear_rgb,flags;
} SA3DSECBackgroundConfig;
typedef struct {
    SA3DSPak package;
    SA3DSECVisual visual;
    const uint8_t *rows;
} SA3DSECBackground;

/* Allocation-free view of owner-generated original background geometry and
 * act/clip configuration. File bytes remain owned by caller. No collision. */
int sa3ds_ec_background_open(SA3DSECBackground *out,const void *data,size_t size);
int sa3ds_ec_background_select(const SA3DSECBackground *background,uint32_t act,
    uint32_t clip_level,SA3DSECBackgroundConfig *out);
/* Source 4F6DB0: Translate(camera.x,0,camera.z) * Scale(act,cliplevel).
 * Camera altitude deliberately does not anchor the sky/horizon vertically. */
int sa3ds_ec_background_transform(const SA3DSECBackground *background,uint32_t act,
    uint32_t clip_level,const float camera[3],SA3CMatrix *out);
#endif
