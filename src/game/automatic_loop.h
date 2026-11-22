#ifndef SA3DS_AUTOMATIC_LOOP_H
#define SA3DS_AUTOMATIC_LOOP_H
#include "../sa1/physics/pc2004_loop.h"
#include <stddef.h>
#define SA3DS_LOOP_MAX_PATHS 16u
#define SA3DS_LOOP_MAX_POINTS 2048u
typedef struct {
    SA1PC2004Loop path;
    SA1Vec3 minimum,maximum;
    uint32_t act,ordinal;
    uint16_t cooldown;
    uint8_t busy;
} SA3DSAutomaticLoopActor;
typedef struct {
    uint32_t count,point_count;
    SA3DSAutomaticLoopActor actors[SA3DS_LOOP_MAX_PATHS];
    SA1PC2004LoopPoint points[SA3DS_LOOP_MAX_POINTS];
} SA3DSAutomaticLoops;
int sa3ds_automatic_loops_open(SA3DSAutomaticLoops *,const void *,size_t);
void sa3ds_automatic_loops_reset(SA3DSAutomaticLoops *,SA1PlayerState *);
/* Returns command26 written, replacing the shared player command slot. */
int sa3ds_automatic_loops_tick(SA3DSAutomaticLoops *,uint32_t act,
    SA1Vec3 task_origin,SA1PlayerState *);
#endif
