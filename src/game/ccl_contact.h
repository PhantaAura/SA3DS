#ifndef SA3DS_CCL_CONTACT_H
#define SA3DS_CCL_CONTACT_H
#include "enemy_damage.h"
typedef struct {uint8_t push_receive,push_inhibit,damage_receive;uint32_t excluded_attributes;} SA3DSCCLRules;
typedef struct {uint8_t form,push,damage;uint32_t attributes;SA1Vec3 center;float radius;} SA3DSCCLShape;
typedef struct {
    SA3DSCCLHits hits;
    SA3DSCCLRules rules;
    SA1Vec3 task_position;
    float broad_radius,direction_x,direction_z;
    uint16_t status,outer_flags,hit_count,hit_flags[16];
    int16_t cached_self_shape,cached_other_shape;
    uint32_t cached_other_id;
} SA3DSCCLBody;
typedef struct {uint8_t broad_overlap,eligible,overlap,supported,pushed_first,pushed_second;float task_distance_squared;} SA3DSCCLContactResult;
/*49AA48..CB writes the low attack/defense nibble before Sonic movement.
 * player_flags4/6 are distinct player-work words, not task status. */
uint8_t sa3ds_sonic_ccl_damage_byte(uint8_t previous,uint8_t action,uint16_t task_status,
    uint16_t player_flags4,uint16_t player_flags6);
uint32_t sa3ds_sonic_ccl_secondary_attributes(uint32_t previous,uint8_t action);
/*Read metadata/radius from owner's48-byte CCI; center is the already evaluated
 * source world center (4915F0/419160), not the raw local record center. */
int sa3ds_ccl_shape_decode(SA3DSCCLShape *shape,const void *owner_record,SA1Vec3 world_center);
void sa3ds_ccl_body_init(SA3DSCCLBody *body,uint32_t actor_id);
/*418B60: call at original CCL clear phase, not before consuming prior hits. */
void sa3ds_ccl_body_clear(SA3DSCCLBody *body,SA3DSCCLShape *shapes,unsigned count);
/*418E70/418DE0: contact side effects for an already confirmed shape pair.
 * Preserve duplicates/order/saturation and last-contact cache. Does not move
 * task positions or generate physical response/contact normal. */
int sa3ds_ccl_record_contact(SA3DSCCLBody *first,SA3DSCCLShape *first_shape,unsigned first_index,
    SA3DSCCLBody *second,SA3DSCCLShape *second_shape,unsigned second_index);
/*418D00/41FEC0/41AA00 broad/category/disabled and sphere-path predicate.
 * supported profile: evaluated world spheres; primary form7 reaches this path
 * only when41FD00 dispatches other form0/7. Sphere-path contact normal and
 * category/push-gated task displacement are included. Caller re-evaluates
 * relative shape centers from updated task positions before later pairs.
 * Callers still collect homing candidates independently of broad_overlap. */
int sa3ds_ccl_sphere_contact(SA3DSCCLBody *first,SA3DSCCLShape *first_shape,unsigned first_index,
    SA3DSCCLBody *second,SA3DSCCLShape *second_shape,unsigned second_index,SA3DSCCLContactResult *out);
#endif
