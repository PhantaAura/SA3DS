#ifndef SA3DS_ENEMY_DAMAGE_H
#define SA3DS_ENEMY_DAMAGE_H
#include "../sa1/physics/player_state.h"
#define SA3DS_CCL_HIT_CAPACITY 16u
typedef struct {int8_t self_shape,other_shape;uint32_t actor_id;} SA3DSCCLHit;
typedef struct {uint32_t actor_id;int has_ccl;SA3DSCCLHit hits[SA3DS_CCL_HIT_CAPACITY];} SA3DSCCLHits;
typedef struct {uint32_t actor_id,query_id;int index;} SA3DSCCLQueryCursor;
typedef int (*SA3DSCCLResolveCategory)(void *context,uint32_t actor_id,unsigned *category);
void sa3ds_ccl_query_reset(SA3DSCCLQueryCursor *cursor);
void sa3ds_ccl_hits_reset(SA3DSCCLHits *hits,uint32_t actor_id);
/*41C6E0/41C750 share the original41B9F0 cursor across ALL actors/queries.
 * Actor/selector change resets search; repeat starts after last cached index.
 * Failed scans do not cache their final local index. -1 sentinel terminates.
 * Returns hit index,-1 absent,-2 invalid. Uses current resolved category. */
int sa3ds_ccl_query_category(SA3DSCCLQueryCursor *cursor,const SA3DSCCLHits *hits,
    unsigned category,SA3DSCCLResolveCategory resolve,void *context);
typedef struct {uint16_t enemy_flags;float squash_velocity,velocity_y;} SA3DSEnemyDamageState;
typedef struct {int present;uint16_t status;uint8_t character;} SA3DSEnemyDamagePlayer;
typedef struct {
    uint16_t task_status;SA1Vec3 position;
    float explosion_radius;SA1Vec3 explosion_center; /*negative disables4C0550*/
    const SA3DSCCLHits *hits;SA3DSCCLResolveCategory resolve;void *resolve_context;
    /*Source CCL+A4/A2 cache is independent of the category query's hit. */
    int cached_damage_other_valid,cached_owner_is_player0;
    uint32_t cached_other_shape_attributes;
    SA3DSEnemyDamagePlayer players[2];
} SA3DSEnemyDamageInput;
typedef struct {
    uint8_t killed,score_delta,rumble_request;
    int bounce_player; /*-1 absent;0/1 from cached owner's task identity*/
    SA1Vec3 bounce_local_velocity; /*441280 writes local(0,2,0)*/
} SA3DSEnemyDamageResult;
/*4CE030/4CDFE0 plus4C0550 explosion decision. Updates enemy fields and emits
 * original score/bounce/rumble requests; does not generate CCL attack flags,
 * award score, mutate native player state or execute death/effect systems. */
int sa3ds_enemy_damage_check(SA3DSEnemyDamageState *state,const SA3DSEnemyDamageInput *input,
    SA3DSCCLQueryCursor *cursor,SA3DSEnemyDamageResult *out);
#endif
