#include "sonic_pose_metrics.h"
#include "sa3c_render.h"
#include <float.h>
#include <math.h>
#include <string.h>

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t rd32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
        ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static float rdf(const uint8_t *p) {
    uint32_t v=rd32(p); float f;
    memcpy(&f,&v,sizeof(f)); return f;
}

typedef struct {
    const SA3CCharacter *character;
    SA3DSPoseMetrics *metrics;
    int failed;
} BoundsVisitor;
static int accumulate(const SA3CDrawCommand *cmd, void *opaque) {
    BoundsVisitor *ctx=(BoundsVisitor*)opaque;
    const SA3CSection *vs=&ctx->character->sections[3];
    const SA3CSection *is=&ctx->character->sections[4];
    const uint8_t *verts=ctx->character->data+vs->offset;
    const uint8_t *indices=ctx->character->data+is->offset;
    if (cmd->first_index>is->count || cmd->index_count>is->count-cmd->first_index) {
        ctx->failed=1; return 1;
    }
    for (uint32_t k=0;k<cmd->index_count;k++) {
        uint32_t index=rd16(indices+2u*(cmd->first_index+k));
        if (index>=vs->count) { ctx->failed=1; return 1; }
        const uint8_t *v=verts+(size_t)index*vs->stride;
        float x=rdf(v),y=rdf(v+4),z=rdf(v+8);
        float p[3]={
            x*cmd->world.m[0]+y*cmd->world.m[4]+z*cmd->world.m[8]+cmd->world.m[12],
            x*cmd->world.m[1]+y*cmd->world.m[5]+z*cmd->world.m[9]+cmd->world.m[13],
            x*cmd->world.m[2]+y*cmd->world.m[6]+z*cmd->world.m[10]+cmd->world.m[14]};
        for(int a=0;a<3;a++) {
            if (!isfinite(p[a])) { ctx->failed=1; return 1; }
            if(p[a]<ctx->metrics->min[a])ctx->metrics->min[a]=p[a];
            if(p[a]>ctx->metrics->max[a])ctx->metrics->max[a]=p[a];
        }
        ctx->metrics->visible_vertices++;
    }
    return 0;
}

int sa3ds_pose_metrics(const SA3CCharacter *c,uint16_t root,
    const SA3CMatrix *world,size_t world_count,SA3DSPoseMetrics *out) {
    if(!c||!world||!out||root>=c->sections[0].count||world_count<c->sections[0].count)
        return 0;
    SA3DSPoseMetrics m={0};
    m.root_node=root;
    for(int a=0;a<3;a++) {
        m.min[a]=FLT_MAX; m.max[a]=-FLT_MAX;
        m.root[a]=world[root].m[12+a];
        if(!isfinite(m.root[a])) return 0;
    }
    BoundsVisitor visitor={c,&m,0};
    SA3CRenderStats stats;
    if(sa3c_render_emit(c,root,world,world_count,accumulate,&visitor,&stats)!=SA3C_POSE_OK ||
        visitor.failed || !m.visible_vertices) return 0;
    m.triangles=stats.triangles;
    m.draw_calls=stats.draw_calls;
    *out=m;
    return 1;
}
