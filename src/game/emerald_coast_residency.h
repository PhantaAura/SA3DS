#ifndef SA3DS_EMERALD_COAST_RESIDENCY_H
#define SA3DS_EMERALD_COAST_RESIDENCY_H
#include "emerald_coast_collision.h"
#include "emerald_coast_route.h"
/* Loading margin within the existing1024-unit collision window, not a physics
 * or retail activation constant. Original world coordinates remain unchanged. */
#define SA3DS_EC_REGION_RELOAD_MARGIN 256.0f
int sa3ds_ec_residency_needs_recenter(const SA3DSECCollision *stage,SA1Vec3 position);
int sa3ds_ec_residency_recenter(SA3DSECCollision *stage,const void *package,size_t size,
    float center_x,float center_z,SA1PlayerState *player);
/* Use the source task position/yaw through the existing support-center bridge.
 * Initial contact/action determination remains the ordinary controller's job. */
void sa3ds_ec_residency_start(SA3DSECCollision *stage,const SA3DSECStart *start,
    SA1PlayerState *player);
#endif
