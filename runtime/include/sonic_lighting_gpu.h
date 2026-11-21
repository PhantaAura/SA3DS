#ifndef SA3DS_SONIC_LIGHTING_GPU_H
#define SA3DS_SONIC_LIGHTING_GPU_H
#include "sonic_gpu.h"
#include "pc2004_lighting.h"
typedef struct SA3DSSonicLightingGpu SA3DSSonicLightingGpu;
SA3DSSonicLightingGpu *sa3ds_sonic_lighting_gpu_create(const SA3DSSonicGpu *base,const SA3DSLightingAssets *assets);
void sa3ds_sonic_lighting_gpu_destroy(SA3DSSonicLightingGpu *lighting);
int sa3ds_sonic_lighting_gpu_draw(SA3DSSonicLightingGpu *lighting,SA3DSSonicGpu *base,unsigned act,
 const C3D_Mtx *projection,const C3D_Mtx *view,const SA3CMatrix *actor,const float eye[3],
 uint16_t root,const SA3CMatrix *world,size_t count,SA3CRenderStats *stats);
#endif
