#ifndef SA3DS_WHALE_VISUAL_H
#define SA3DS_WHALE_VISUAL_H
#include "emerald_coast_visual.h"
#include "whale_shape.h"
typedef struct {SA3DSECVisual visual;SA3DSWhaleShape shape;const uint8_t *source_indices;} SA3DSWhaleVisual;
int sa3ds_whale_visual_open(SA3DSWhaleVisual *out,const void *bytes,size_t size);
/*Borrowed original source identity for each UV-split render vertex.*/
int sa3ds_whale_visual_source_index(const SA3DSWhaleVisual *visual,uint32_t vertex,uint16_t *out);
/*Caller-owned scratch/output; preserves UV while updating BOTH positions and
 *normals. Output local coordinates still need original actor/root transforms.*/
int sa3ds_whale_visual_sample(const SA3DSWhaleVisual *visual,unsigned bank,float frame,
 float (*positions)[3],float (*normals)[3],size_t scratch_count,
 SA3DSECVertex *vertices,size_t vertex_capacity);
#endif
