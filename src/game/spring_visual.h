#ifndef SA3DS_SPRING_VISUAL_H
#define SA3DS_SPRING_VISUAL_H
#include "emerald_coast_visual.h"
#define SA3DS_SPRING_VISUAL_MAX_BYTES (2u*1024u*1024u)
typedef struct {
    SA3DSECVisual components[6];
    float offsets[6][3],cap_displacement[6];
} SA3DSSpringVisual;
int sa3ds_spring_visual_open(SA3DSSpringVisual *out,const void *bytes,size_t size);
#endif
