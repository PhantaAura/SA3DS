#ifndef SA3DS_WHALE_TRIGGER_ASSETS_H
#define SA3DS_WHALE_TRIGGER_ASSETS_H
#include <stddef.h>
#include <stdint.h>
typedef struct {uint32_t act,source_row;uint16_t type;int16_t angles[3];float position[3],parameters[3];} SA3DSWhalePlacement;
typedef struct {const uint8_t *rows;uint32_t count;float radius,load_distance_squared;} SA3DSWhaleTriggerAssets;
int sa3ds_whale_trigger_assets_open(SA3DSWhaleTriggerAssets *out,const void *bytes,size_t size);
int sa3ds_whale_trigger_placement(const SA3DSWhaleTriggerAssets *assets,uint32_t index,SA3DSWhalePlacement *out);
/*441840 uses the updated CCL center, not task origin or an expanded player
 *radius. Both the player task and its CCL work must be present.*/
typedef struct {float center[3];uint8_t task_present,ccl_present;} SA3DSWhaleContactPlayer;
/*Success writes first-inside ordinal0/1/2. This is separate from clip/debug
 *gates; caller feeds the ordinal to the existing summon/kill task rules.*/
int sa3ds_whale_trigger_inside(const float center[3],float radius,
 const SA3DSWhaleContactPlayer players[2],uint8_t *inside_player);
#endif
