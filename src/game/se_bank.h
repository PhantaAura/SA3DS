#ifndef SA3DS_SE_BANK_H
#define SA3DS_SE_BANK_H
#include "adx.h"
#include <stddef.h>
#include <stdint.h>
/*PC2004 sound-effect id resolution (425120): scan the descending threshold
 *table for the first threshold below the id; bank = its slot, index =
 *id-threshold-1; no match (id <= smallest threshold) -> bank0, index=id.
 *Ids above 0x568 are voices (425710) and are not resolved here. The entry
 *is found by name "B%02d_00_%02d.wav" compared from its 4th character,
 *case-insensitive (423AD0), in the bank loaded for that slot. The 4246C0
 *second-player bank remap is not modeled (single player).*/
typedef struct {int32_t threshold,bank;} SA3DSSeThreshold;
int sa3ds_se_resolve(const SA3DSSeThreshold *table,size_t count,uint32_t id,uint8_t *bank,uint32_t *index);
/*Finds the entry for index in a loaded bank. Returns1 and fills out.*/
int sa3ds_se_find(const uint8_t *bank,size_t size,uint32_t index,SA3DSSoundBankEntry *out);
#endif
