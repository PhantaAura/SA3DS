#include "lab_gpu.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/game/parity_lab.h"

#define PI 3.14159265358979323846f

static void camera_relative(const SA1PlayerState *p, float camera_yaw,
                            float x, float y, float z, float out[3]) {
    float dx = x - p->pos.x;
    float dy = y - p->pos.y;
    float dz = z - p->pos.z;
    float c = cosf(-camera_yaw), s = sinf(-camera_yaw);
    out[0] = dx*c - dz*s;
    out[1] = dy;
    out[2] = dx*s + dz*c;
}

static void write_vertex(SA3DSLabVertex *v, const SA1PlayerState *p,
                         float camera_yaw, float x, float y, float z) {
    camera_relative(p, camera_yaw, x, y, z, v->position);
    v->texcoord[0] = 0.0f;
    v->texcoord[1] = 0.0f;
}

static void write_quad(SA3DSLabVertex *v, const SA1PlayerState *p,
                       float camera_yaw,
                       float x0, float x1, float z0, float z1,
                       float y00, float y10, float y01, float y11) {
    write_vertex(&v[0],p,camera_yaw,x0,y00,z0);
    write_vertex(&v[1],p,camera_yaw,x1,y10,z0);
    write_vertex(&v[2],p,camera_yaw,x0,y01,z1);
    write_vertex(&v[3],p,camera_yaw,x0,y01,z1);
    write_vertex(&v[4],p,camera_yaw,x1,y10,z0);
    write_vertex(&v[5],p,camera_yaw,x1,y11,z1);
}

static void bind_vertices(SA3DSLabVertex *v) {
    C3D_BufInfo *buf = C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf, v, sizeof(*v), 2, 0x10);
}

static void set_constant_color(uint32_t rgba) {
    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_CONSTANT, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
    C3D_TexEnvColor(env, rgba);
}

static uint32_t zone_color(unsigned id) {
    /* Subtle developer-only White Zone bands. Exact hue is presentation-only. */
    static const uint32_t c[] = {
        0xF7F7F7FFu, 0xE9F2FAFFu, 0xF1F4F7FFu,
        0xEDF7F2FFu, 0xF7F1EDFFu, 0xEEF0F7FFu,
        0xF5F5F5FFu, 0xF2EEF7FFu, 0xEAF5F7FFu
    };
    return c[id % (sizeof(c)/sizeof(c[0]))];
}

int sa3ds_lab_gpu_init(SA3DSLabGpu *lab) {
    if (!lab) return 0;
    memset(lab, 0, sizeof(*lab));
    /* Zone draws and the marker batch must have DISJOINT memory ranges.
       The GPU may not read queued vertices until C3D_FrameEnd. */
    lab->scratch_capacity = 1024;
    lab->scratch = (SA3DSLabVertex*)linearAlloc(
        (size_t)lab->scratch_capacity * sizeof(*lab->scratch));
    return lab->scratch != 0;
}

void sa3ds_lab_gpu_destroy(SA3DSLabGpu *lab) {
    if (!lab) return;
    if (lab->scratch) linearFree(lab->scratch);
    memset(lab, 0, sizeof(*lab));
}

int sa3ds_lab_gpu_draw(SA3DSLabGpu *lab, const SA1PlayerState *player,
                       float camera_yaw) {
    if (!lab || !lab->scratch || !player) return 0;
    lab->frame_draws = 0;
    lab->frame_triangles = 0;
    C3D_CullFace(GPU_CULL_NONE);

    size_t zone_count = 0;
    const SA3DSParityZone *zones = sa3ds_parity_lab_zones(&zone_count);
    const float half = sa3ds_parity_lab_half_width();
    uint32_t cursor = 0;

    for (size_t i = 0; i < zone_count; ++i) {
        SA1Surface a = sa3ds_parity_lab_surface(0, -half, zones[i].z_start + 0.001f);
        SA1Surface b = sa3ds_parity_lab_surface(0,  half, zones[i].z_start + 0.001f);
        SA1Surface c = sa3ds_parity_lab_surface(0, -half, zones[i].z_end - 0.001f);
        SA1Surface d = sa3ds_parity_lab_surface(0,  half, zones[i].z_end - 0.001f);
        if (!a.valid || !b.valid || !c.valid || !d.valid) return 0;
        if (cursor > lab->scratch_capacity || 6u > lab->scratch_capacity - cursor)
            return 0;
        SA3DSLabVertex *zone_vertices = &lab->scratch[cursor];
        write_quad(zone_vertices, player, camera_yaw, -half, half,
                   zones[i].z_start, zones[i].z_end,
                   a.ground_y, b.ground_y, c.ground_y, d.ground_y);
        GSPGPU_FlushDataCache(zone_vertices, 6u*sizeof(*zone_vertices));
        bind_vertices(zone_vertices);
        cursor += 6u;
        set_constant_color(zone_color((unsigned)i));
        C3D_DrawArrays(GPU_TRIANGLES, 0, 6);
        lab->frame_draws++;
        lab->frame_triangles += 2;
    }

    /* Distance markers every 10 world units. Their exact placement is the
       measurement fixture; it is intentionally independent of stage art. */
    uint32_t n = 0;
    const float spacing = sa3ds_parity_lab_marker_spacing();
    float z0 = zones[0].z_start;
    float z1 = zones[zone_count-1].z_end;
    float first = ceilf(z0 / spacing) * spacing;
    /* Reserve after ALL zone submissions, not over their queued vertices. */
    uint32_t marker_start = cursor;
    for (float z = first; z <= z1 && n <= lab->scratch_capacity - cursor &&
                       6u <= lab->scratch_capacity - cursor - n; z += spacing) {
        SA1Surface s0 = sa3ds_parity_lab_surface(0, -half, fminf(z + 0.001f, z1 - 0.001f));
        SA1Surface s1 = sa3ds_parity_lab_surface(0,  half, fminf(z + 0.001f, z1 - 0.001f));
        if (!s0.valid || !s1.valid) continue;
        float dz = 0.055f;
        write_quad(&lab->scratch[marker_start+n], player, camera_yaw, -half, half,
                   z - dz, z + dz,
                   s0.ground_y + 0.012f, s1.ground_y + 0.012f,
                   s0.ground_y + 0.012f, s1.ground_y + 0.012f);
        n += 6u;
    }
    if (n) {
        SA3DSLabVertex *markers = &lab->scratch[marker_start];
        GSPGPU_FlushDataCache(markers, (size_t)n*sizeof(*markers));
        bind_vertices(markers);
        set_constant_color(0x8090A0FFu);
        C3D_DrawArrays(GPU_TRIANGLES, 0, n);
        lab->frame_draws++;
        lab->frame_triangles += n/3u;
    }
    return 1;
}
