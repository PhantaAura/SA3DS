#ifndef SA3DS_PC2004_SPHERE_SWEEP_H
#define SA3DS_PC2004_SPHERE_SWEEP_H
#include <stddef.h>
#include <stdint.h>
/*PC2004 453820 per-polygon sphere sweep. Each triangle is inflated by1.04
 *about its centroid, rotated so its normal is +Y (450810/4509D0, 4B8E80,
 *rotX(-ax) then rotZ(-az)), and the sweep is sampled every radius*.5 (last
 *sample exactly at start+delta). A sample within +-radius of the plane whose
 *XZ lies inside the triangle (cross-sign test) is a hit. Polygon flag
 *0x800000 is double sided: a start below the plane flips radius and normal.*/
typedef struct {float v[3][3];uint32_t flags;} SA3DSSweepTriangle;
typedef struct {
 float normal[3],signed_normal[3];
 float plane_point[3];   /*start projected onto the plane (world)*/
 float corrected[3];     /*start XZ at signed radius above the plane (world)*/
 float delta_xz[3],delta_y[3]; /*delta split along the plane / normal (world)*/
 float start_height,end_height;
 int32_t angle_x,angle_z;      /*4B8E80 output*/
} SA3DSSweepHit;
void sa3ds_pc2004_normal_angles(float x,float y,float z,int32_t *angle_x,int32_t *angle_z); /*4B8E80*/
/*Returns1 on a hit and fills out.*/
int sa3ds_pc2004_sweep_triangle(const SA3DSSweepTriangle *triangle,const float start[3],const float delta[3],
 float radius,SA3DSSweepHit *out);
/*Whole candidate list: up to16 hits are recorded in order; the chosen one has
 *the smallest end height (strict <, first wins). Returns the hit count.*/
int sa3ds_pc2004_sweep(const SA3DSSweepTriangle *triangles,size_t count,const float start[3],const float delta[3],
 float radius,SA3DSSweepHit *chosen,int *chosen_index);
#endif
