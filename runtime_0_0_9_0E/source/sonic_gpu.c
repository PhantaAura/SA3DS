#include "sonic_gpu.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TEX_UPLOAD_FLAGS \
    (GX_TRANSFER_FLIP_VERT(1) | GX_TRANSFER_OUT_TILED(1) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static float rdf(const uint8_t *p) {
    uint32_t u = rd32(p); float f; memcpy(&f, &u, sizeof(f)); return f;
}

/*
 * SA3T stores conventional row-major RGBA8. PICA textures are tiled. This
 * follows the well-established 3DS path used by sf2d: bswap each RGBA32 pixel,
 * then hardware-display-transfer into an OUT_TILED RGBA8 texture with vertical
 * flip. No Sega data is embedded; input comes from owner-generated SA3T.
 */
static int upload_rgba8(C3D_Tex *tex, uint16_t width, uint16_t height, const uint8_t *rgba) {
    if (!tex || !rgba) return 0;
    if (!C3D_TexInit(tex, width, height, GPU_RGBA8)) return 0;

    const size_t bytes = (size_t)width * (size_t)height * 4u;
    uint32_t *tmp = (uint32_t*)linearAlloc(bytes);
    if (!tmp) { C3D_TexDelete(tex); memset(tex,0,sizeof(*tex)); return 0; }

    for (size_t i=0;i<(size_t)width*(size_t)height;i++) {
        uint32_t px;
        memcpy(&px, rgba + i*4u, 4u);
        tmp[i] = __builtin_bswap32(px);
    }

    GSPGPU_FlushDataCache(tmp, bytes);
    GSPGPU_FlushDataCache(tex->data, tex->size);
    C3D_SyncDisplayTransfer(
        tmp, GX_BUFFER_DIM(width,height),
        (uint32_t*)tex->data, GX_BUFFER_DIM(width,height),
        TEX_UPLOAD_FLAGS);
    gspWaitForPPF();
    linearFree(tmp);

    C3D_TexSetFilter(tex, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(tex, GPU_REPEAT, GPU_REPEAT);
    return 1;
}

static void xform(const SA3CMatrix *m, float x,float y,float z, float *out) {
    out[0]=x*m->m[0]+y*m->m[4]+z*m->m[8]+m->m[12];
    out[1]=x*m->m[1]+y*m->m[5]+z*m->m[9]+m->m[13];
    out[2]=x*m->m[2]+y*m->m[6]+z*m->m[10]+m->m[14];
}

static uint32_t material_rgba(const SA3CCharacter *c, uint16_t material_index) {
    const SA3CSection *s=&c->sections[5];
    if ((uint32_t)material_index >= s->count) return 0xFFFFFFFFu;
    const uint8_t *m=c->data+s->offset+(size_t)material_index*s->stride;
    uint32_t d=rd32(m);
    uint8_t r=(uint8_t)((d>>16)&255u), g=(uint8_t)((d>>8)&255u), b=(uint8_t)(d&255u);
    if (r==0 && g==0 && b==0) r=g=b=255;
    return ((uint32_t)r<<24)|((uint32_t)g<<16)|((uint32_t)b<<8)|0xFFu;
}

typedef struct { SA3DSSonicGpu *r; } EmitCtx;

static int draw_command(const SA3CDrawCommand *cmd, void *user) {
    EmitCtx *ctx=(EmitCtx*)user;
    SA3DSSonicGpu *r=ctx->r;
    const SA3CCharacter *c=r->character;
    if (!cmd || cmd->index_count==0 || cmd->index_count>r->scratch_capacity) return 1;
    if ((uint32_t)cmd->texture_slot>=r->texture_count) return 1;

    const SA3CSection *vs=&c->sections[3], *is=&c->sections[4];
    const uint8_t *vertices=c->data+vs->offset;
    const uint8_t *indices=c->data+is->offset;

    for (uint32_t i=0;i<cmd->index_count;i++) {
        uint16_t vi=rd16(indices+(size_t)(cmd->first_index+i)*2u);
        if ((uint32_t)vi>=vs->count) return 1;
        const uint8_t *v=vertices+(size_t)vi*vs->stride;
        float p[3];
        xform(&cmd->world,rdf(v),rdf(v+4),rdf(v+8),p);
        r->scratch[i].position[0]=p[0];
        r->scratch[i].position[1]=p[1];
        r->scratch[i].position[2]=p[2];
        float u=sa3c_basic_uv_normalize(rdf(v+24));
        float vv=sa3c_basic_uv_normalize(rdf(v+28));
        r->scratch[i].texcoord[0]=u;
        r->scratch[i].texcoord[1]=r->v_flip_debug ? -vv : vv;
    }

    GSPGPU_FlushDataCache(r->scratch,(size_t)cmd->index_count*sizeof(*r->scratch));
    C3D_BufInfo *buf=C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf,r->scratch,sizeof(*r->scratch),2,0x10);

    C3D_TexBind(0,&r->gpu_textures[cmd->texture_slot]);
    C3D_TexEnv *env=C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env,C3D_Both,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvFunc(env,C3D_Both,GPU_MODULATE);
    C3D_TexEnvColor(env,material_rgba(c,cmd->material_index));

    C3D_DrawArrays(GPU_TRIANGLES,0,cmd->index_count);
    r->frame_draws++;
    r->frame_vertices+=cmd->index_count;
    r->frame_triangles+=cmd->index_count/3u;
    return 0;
}

int sa3ds_sonic_gpu_init(SA3DSSonicGpu *r, const SA3CCharacter *c, const SA3TArchive *t) {
    if (!r || !c || !t || t->texture_count==0 || t->texture_count!=c->sections[6].count) return 0;
    memset(r,0,sizeof(*r)); r->character=c; r->textures=t; r->texture_count=t->texture_count;

    r->gpu_textures=(C3D_Tex*)calloc(r->texture_count,sizeof(C3D_Tex));
    if (!r->gpu_textures) return 0;
    for (uint32_t i=0;i<r->texture_count;i++) {
        SA3TTexture desc; const uint8_t *rgba=NULL;
        if (sa3t_texture(t,i,&desc,&rgba)!=SA3T_OK || !upload_rgba8(&r->gpu_textures[i],desc.width,desc.height,rgba)) {
            sa3ds_sonic_gpu_destroy(r); return 0;
        }
    }

    /* 1,993 triangles => <=5,979 submitted vertices for normal Sonic, but use
       package-wide INDX count as a safe upper bound for any one mesh. */
    uint32_t max_mesh_indices=0;
    const SA3CSection *ms=&c->sections[2]; const uint8_t *mesh=c->data+ms->offset;
    for (uint32_t i=0;i<ms->count;i++) {
        uint32_t n=rd32(mesh+(size_t)i*ms->stride+8);
        if (n>max_mesh_indices) max_mesh_indices=n;
    }
    if (max_mesh_indices<3) { sa3ds_sonic_gpu_destroy(r); return 0; }
    r->scratch_capacity=max_mesh_indices;
    r->scratch=(SA3DSGpuVertex*)linearAlloc((size_t)r->scratch_capacity*sizeof(*r->scratch));
    if (!r->scratch) { sa3ds_sonic_gpu_destroy(r); return 0; }
    return 1;
}

void sa3ds_sonic_gpu_destroy(SA3DSSonicGpu *r) {
    if (!r) return;
    if (r->gpu_textures) {
        for (uint32_t i=0;i<r->texture_count;i++) if (r->gpu_textures[i].data) C3D_TexDelete(&r->gpu_textures[i]);
        free(r->gpu_textures);
    }
    if (r->scratch) linearFree(r->scratch);
    memset(r,0,sizeof(*r));
}

int sa3ds_sonic_gpu_draw(SA3DSSonicGpu *r, uint16_t root_node, const SA3CMatrix *world, size_t world_count, SA3CRenderStats *out_stats) {
    if (!r || !world) return 0;
    r->frame_draws=r->frame_triangles=r->frame_vertices=0;
    EmitCtx ctx={r}; SA3CRenderStats st;
    SA3CPoseResult pr=sa3c_render_emit(r->character,root_node,world,world_count,draw_command,&ctx,&st);
    if (pr!=SA3C_POSE_OK) return 0;
    if (out_stats) *out_stats=st;
    return 1;
}
