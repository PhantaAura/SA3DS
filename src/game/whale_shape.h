#ifndef SA3DS_WHALE_SHAPE_H
#define SA3DS_WHALE_SHAPE_H
#include <stddef.h>
#include <stdint.h>
typedef struct {const uint8_t *keys;uint32_t count;} SA3DSWhaleShapeChannel;
typedef struct {uint32_t duration;SA3DSWhaleShapeChannel position,normal;} SA3DSWhaleShapeBank;
typedef struct {const uint8_t *data;size_t size;uint32_t vertices;SA3DSWhaleShapeBank banks[2];} SA3DSWhaleShape;
/*Borrowed external owner data. Bank0 cycle, bank1 exit.*/
int sa3ds_whale_shape_open(SA3DSWhaleShape *out,const void *bytes,size_t size);
/*405EF0/405AA0/4054C0/405590/405630; nonnegative display frame wraps at
 *bank duration. Output is source-vertex order, not UV-split render order.*/
int sa3ds_whale_shape_sample(const SA3DSWhaleShape *shape,unsigned bank,float frame,
 float (*positions)[3],float (*normals)[3],size_t capacity);
#endif
