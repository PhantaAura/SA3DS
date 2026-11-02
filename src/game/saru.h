#ifndef SA3DS_SARU_H
#define SA3DS_SARU_H
#include "movement_objects.h"
#define SA3DS_SARU_PACKAGE_MAX_BYTES (256u*1024u)
typedef struct {
    SA3DSMovementObjects placements;
    const uint8_t *collider; /*48 source CCL bytes, borrowed owner payload */
    float load_squared,unload_squared,visibility_radius,display_scale;
} SA3DSSaruDefinitions;
typedef struct {
    SA1Vec3 spawn_origin,task_position,task_scale;
    int32_t task_angles[3],initial_angle_y;
    uint16_t packed_tag_low,task_status,enemy_flags;
    uint8_t mode,index_seed;
    float animation_frame;
    uint32_t behavior_flags;
    float parameter_cc,parameter_d0,parameter_3c;
} SA3DSSaruInitial;
int sa3ds_saru_definitions_open(SA3DSSaruDefinitions *out,const void *data,size_t size);
/*4AD140/4AB240 fresh zeroed-allocation profile. sine/cosine of SET angleX
 * come from the owner's source trig. Do not treat X as physical pitch or Z as
 * an ordinary rotation: Z's two bytes are consumed separately by the source.
 * Shared enemy allocation/AI/update, animation clips and rendering are not
 * performed by this initializer. Failure clears out. */
int sa3ds_saru_initialize(const SA3DSMovementPlacement *placement,uint16_t random15,
    double sine_x,double cosine_x,SA3DSSaruInitial *out);
#endif
