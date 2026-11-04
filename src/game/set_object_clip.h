#ifndef SA3DS_SET_OBJECT_CLIP_H
#define SA3DS_SET_OBJECT_CLIP_H
#include <stdint.h>
typedef struct {float position[3];uint8_t present;} SA3DSSetClipReference;
typedef struct {
 float position[3],unload_distance_squared;
 uint8_t has_set_data,set_flags,player1_character;
 SA3DSSetClipReference camera,player0,player1;
} SA3DSSetClipInput;
/*46B750 is inclusive, with X/X+Y early-outs and no float sum store.*/
int sa3ds_set_position_in_range(const float position[3],const float reference[3],float distance_squared,int *inside);
/*46BCB4: runtime SET data+0xC receives stored float(load distance+100).*/
int sa3ds_set_unload_distance(float load_distance_squared,float *out);
/*46C360 ->46C010. Output1 means replace main callback with40B570 and stop
 *ordinary work now; it is NOT permission to free task storage immediately.*/
int sa3ds_set_object_clip(const SA3DSSetClipInput *input,uint8_t *defer_delete);
#endif
