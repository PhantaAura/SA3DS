#ifndef SA3DS_SPRING_PLAYER_H
#define SA3DS_SPRING_PLAYER_H
#include "spring_runtime.h"
typedef struct {SA3DSSpringCommand command;uint16_t status;uint8_t valid;} SA3DSSpringPendingCommand;
/* Object phase writes placement/velocity immediately; Sonic consumes the
 * pending command during its following selection phase. Ordinary Sonic only. */
int sa3ds_spring_player_apply(SA1PlayerState *player,
    const SA3DSMovementPlacement *placement,const SA3DSSpringTrig *trig,
    const SA3DSSpringLaunch *launch,float controller_support,
    SA3DSSpringPendingCommand *pending);
/* Before movement: consume command, select spring rise/apex, then query the
 * signed control timer. The tail is deliberately separate. */
int sa3ds_spring_player_begin(SA1PlayerState *player,
    SA3DSSpringPendingCommand *pending,SA1Input *input);
void sa3ds_spring_player_tail(SA1PlayerState *player);
#endif
