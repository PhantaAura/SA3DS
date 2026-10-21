#ifndef SA3DS_TEST_CITRO3D_H
#define SA3DS_TEST_CITRO3D_H
#include "3ds.h"
typedef struct { void *data; } C3D_Tex;
typedef struct { const void *data; } C3D_BufInfo;
typedef struct { uint32_t color; } C3D_TexEnv;
enum { GPU_GREATER=6, GPU_WRITE_COLOR=15, GPU_WRITE_ALL=31,
    GPU_BLEND_ADD=0, GPU_SRC_ALPHA=6, GPU_ONE_MINUS_SRC_ALPHA=7,
    GPU_TEXTURE0=3, GPU_CONSTANT=14, GPU_MODULATE=1,
    GPU_TRIANGLES=0, C3D_Both=2 };
void C3D_TexDelete(C3D_Tex *texture);
C3D_BufInfo *C3D_GetBufInfo(void);
void BufInfo_Init(C3D_BufInfo *info);
int BufInfo_Add(C3D_BufInfo *info,const void *data,ptrdiff_t stride,int attributes,uint64_t permutation);
void C3D_TexBind(int unit,C3D_Tex *texture);
C3D_TexEnv *C3D_GetTexEnv(int unit);
void C3D_TexEnvInit(C3D_TexEnv *env);
void C3D_TexEnvSrc(C3D_TexEnv *env,int mode,int source0,int source1,int source2);
void C3D_TexEnvFunc(C3D_TexEnv *env,int mode,int function);
void C3D_TexEnvColor(C3D_TexEnv *env,uint32_t color);
void C3D_DrawArrays(int primitive,int first,int count);
void C3D_DepthTest(bool enabled,int function,int write_mask);
void C3D_AlphaBlend(int color_eq,int alpha_eq,int source_color,int destination_color,
    int source_alpha,int destination_alpha);
#endif
