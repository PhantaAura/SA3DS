#ifndef SA3DS_WHALE_GPU_H
#define SA3DS_WHALE_GPU_H
#include "emerald_coast_gpu.h"
#include "whale_visual.h"
typedef struct {
 SA3DSECGpu gpu;const SA3DSWhaleVisual *visual;
 uint16_t *render_indices;SA3DSECVertex *sampled;
 float (*positions)[3],(*normals)[3];
} SA3DSWhaleGpu;
/*Uses the preserved CPU texture uploader. Asset views outlive the renderer.
 *Caller retires prior GPU work before init/update/destroy. Update exactly once
 *per synchronized frame, then reuse the same pending mesh for both eyes.*/
int sa3ds_whale_gpu_init(SA3DSWhaleGpu *gpu,const SA3DSWhaleVisual *visual,
 SA3DSSonicTextureTrace trace,void *userdata);
int sa3ds_whale_gpu_update(SA3DSWhaleGpu *gpu,unsigned bank,float frame);
/*Caller binds projection/modelview including original actor/root transforms.
 *Existing Basic material renderer only; original lighting/order remain open.*/
int sa3ds_whale_gpu_draw(SA3DSWhaleGpu *gpu);
void sa3ds_whale_gpu_destroy(SA3DSWhaleGpu *gpu);
#endif
