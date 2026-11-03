#ifndef SA3DS_PLAYER_DAMAGE_PLAYER_H
#define SA3DS_PLAYER_DAMAGE_PLAYER_H
#include "player_damage.h"
#include "../sa1/physics/movement.h"
/*Run before native command processing/movement. Previous PlayAnimation request
 * is a source gameplay input; pose selection remains read-only. Returns1 for
 * ordinary prepared movement,2 for source death command50 awaiting executor,
 *3 for special water/carried-object effects not handled by this bridge,0 bad.
 *Ring scatter/audio requests are preserved for the world executor.*/
int sa3ds_player_damage_prepare(SA1PlayerState *player,const SA1Input *input,
    uint16_t prior_ccl_status,uint16_t previous_animation,int16_t *rings,
    const SA3DSPlayerDamageWorld *world,const SA1SteamPlayerParameters *parameters,
    SA3DSPlayerDamageResult *out);
#endif
