#ifndef SA3DS_RING_GROUPS_H
#define SA3DS_RING_GROUPS_H
#include "ring_runtime.h"
#include "sa3dspak.h"
int sa3ds_ring_groups_open(SA3DSRings *rings,const SA3DSPak *package);
void sa3ds_ring_groups_reset(SA3DSRings *rings,uint32_t act);
void sa3ds_ring_groups_begin(SA3DSRings *rings,SA1Vec3 task_origin,unsigned clip);
void sa3ds_ring_groups_end(SA3DSRings *rings);
double sa3ds_ring_group_sin(const SA3DSRings *rings,uint16_t bams);
#endif
