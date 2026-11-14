#include "ccl_wall.h"
#include <math.h>

SA3DSWallResponse sa3ds_ccl_wall_sample(SA1Vec3 sample, float player_radius,
    float player_half_height, SA1Vec3 extents, float *corrected_z){
    if(!corrected_z||!isfinite(sample.z)||player_radius<0||player_half_height<0||
       !(extents.x>0&&extents.y>0&&extents.z>0))return SA3DS_WALL_MISS;

    if(!(sample.x+player_radius>-extents.x&&sample.x-player_radius<extents.x&&
         sample.y+player_half_height>-extents.y&&sample.y-player_half_height<extents.y))
        return SA3DS_WALL_MISS;

    /* 41A580 uses a soft displacement in the 50-unit band beyond the
       front face, and a position write closer to the plane. */
    float depth=sample.z-extents.z;
    if(depth>=0&&depth<50.f){
        *corrected_z=(float)(((double)extents.z+50.0-sample.z)*0.06+sample.z);
        return SA3DS_WALL_SOFT;
    }
    if(depth<0&&sample.z>-.2f){
        *corrected_z=(float)((double)extents.z+0.1);
        return SA3DS_WALL_HARD;
    }
    return SA3DS_WALL_MISS;
}
