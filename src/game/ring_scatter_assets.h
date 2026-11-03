#ifndef SA3DS_RING_SCATTER_ASSETS_H
#define SA3DS_RING_SCATTER_ASSETS_H
#include <stddef.h>
#include "ccl_contact.h"
typedef struct {const unsigned char *collider;SA3DSCCLRules rules;} SA3DSRingScatterAssets;
/*Borrowed owner-generated CCI/category7 payload. Caller retains the package.*/
int sa3ds_ring_scatter_assets_open(SA3DSRingScatterAssets *out,const void *data,size_t size);
#endif
