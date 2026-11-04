#ifndef SA3DS_WHALE_CCL_H
#define SA3DS_WHALE_CCL_H
#include "ccl_contact.h"
#include <stddef.h>
typedef struct {SA3DSCCLShape shapes[2];SA3DSCCLRules rules;float broad_radius;} SA3DSWhaleCCLAssets;
typedef struct {SA3DSCCLBody body;SA3DSCCLShape shapes[2];} SA3DSWhaleCCL;
/*Decodes owner-generated two-sphere profile. No retained package pointers.*/
int sa3ds_whale_ccl_assets_open(SA3DSWhaleCCLAssets *out,const void *bytes,size_t size);
/*502AD2..AEF: initialize category2 CCL, then disable ONLY second shape.
 *Position is the constructor's already displaced task origin.*/
int sa3ds_whale_ccl_init(SA3DSWhaleCCL *out,const SA3DSWhaleCCLAssets *assets,
 uint32_t task_id,SA1Vec3 task_position);
/*Emergence clears second CCI attribute0x10, never first CCI.*/
void sa3ds_whale_ccl_emerge(SA3DSWhaleCCL *ccl);
/*502906 common tail writes pre-horizontal-movement tip into first CCI.
 *419160: first is absolute (attribute0x20); zero-local second follows current
 *task origin, independent of drawY/root pitch/roll. Does NOT clear old hits,
 *run contact pairs or enqueue; caller retains original CCL phase ordering.*/
int sa3ds_whale_ccl_publish(SA3DSWhaleCCL *ccl,SA1Vec3 task_position,SA1Vec3 tip);
#endif
