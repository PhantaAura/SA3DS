#ifndef SA3DS_WHALE_TRIGGER_H
#define SA3DS_WHALE_TRIGGER_H
#include <stdint.h>
/*4FBDA0 summon and4FBC90 kill tasks. Caller resolves original clip/debug and
 *441840's first-inside-player result using updated player collision centers.
 *The result is0 for none,1 for player0,2 for player1; it is not a bitmask.*/
typedef struct {uint8_t action;} SA3DSWhaleTrigger;
typedef struct {uint8_t clipped,debug_mode,debug_selected,inside_player;} SA3DSWhaleTriggerInput;
typedef struct {uint8_t summon,request_kill,debug_display;} SA3DSWhaleTriggerResult;
int sa3ds_whale_summon_tick(SA3DSWhaleTrigger *state,const SA3DSWhaleTriggerInput *input,SA3DSWhaleTriggerResult *out);
int sa3ds_whale_kill_tick(SA3DSWhaleTrigger *state,const SA3DSWhaleTriggerInput *input,SA3DSWhaleTriggerResult *out);
/*Summon uses SET position + each raw parameter, and unchanged angleY.
 *Returned request must replace the current AO task; rendering/AI are separate.*/
int sa3ds_whale_summon_position(const float position[3],const float parameters[3],float out[3]);
#endif
