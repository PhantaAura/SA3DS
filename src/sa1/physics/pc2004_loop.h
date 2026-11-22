#ifndef SA1_PC2004_LOOP_H
#define SA1_PC2004_LOOP_H
#include "surface_frame.h"
typedef struct { float length; SA1Vec3 position; } SA1PC2004LoopPoint;
typedef struct SA1PC2004Loop {
    const SA1PC2004LoopPoint *points;
    uint32_t count;
    float length;
    uint8_t wrap;
} SA1PC2004Loop;
int sa1_pc2004_loop_sample(const SA1PC2004Loop *,float,SA1Vec3 *);
float sa1_pc2004_loop_nearest(const SA1PC2004Loop *,SA1Vec3);
void sa1_pc2004_loop_begin(SA1PlayerState *,float run_speed);
/* 43C130 advances path distance and turns the task; it never rails position. */
int sa1_pc2004_loop_steer(SA1PlayerState *,SA1SurfaceFrame *,float *forward,
    float dash_speed,SA1Vec3 motion);
#endif
