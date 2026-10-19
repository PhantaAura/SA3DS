#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sonic_shbin.h"
#include "owner_assets.h"
#include "sonic_gpu.h"
#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sa3t_texture.h"

#define CLEAR_COLOR 0xF8FAFCFFu
#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

#define INVALID_U16 0xFFFFu

typedef struct {
    const char *name;
    uint16_t dense_id;
    uint16_t action_slot;
} AnimPreset;

/* Exact dense-runtime -> CHRMODELS ACTION remap recovered from 2004 sonic.exe. */
static const AnimPreset kPresets[] = {
    {"Stand",       0,  1},
    {"Walk1",       9,  3},
    {"Run1",       12,  6},
    {"JumpOrSpin", 14, 14},
    {"Roll",       15, 14},
    {"Fall",       18, 10},
    {"Land",       19, 11},
    {"Brake",      20,  7},
};
#define PRESET_COUNT ((int)(sizeof(kPresets)/sizeof(kPresets[0])))

static DVLB_s *g_dvlb;
static shaderProgram_s g_program;
static int g_u_projection=-1, g_u_modelview=-1;
static C3D_Mtx g_projection;
static C3D_RenderTarget *g_target;

static uint8_t *g_sa3c_bytes, *g_sa3t_bytes;
static size_t g_sa3c_size, g_sa3t_size;
static SA3CCharacter g_character;
static SA3TArchive g_texture_archive;
static SA3CTransform *g_local;
static SA3CMatrix *g_world;
static size_t g_node_count;
static SA3DSSonicGpu g_gpu;

static int g_preset=0;
static float g_anim_frame=0.0f;
static int g_paused=0;
static float g_model_yaw=0.0f;
static float g_camera_distance=18.0f;
static int g_cull_mode=0; /* 0 none, 1 back-ccw, 2 front-ccw */
static uint64_t g_last_submit_ms=0;
static SA3CRenderStats g_stats;

static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint16_t action_frame_count(const SA3CCharacter *c, uint16_t action_slot) {
    const SA3CSection *s=&c->sections[7];
    if ((uint32_t)action_slot>=s->count) return 0;
    const uint8_t *r=c->data+s->offset+(size_t)action_slot*s->stride;
    if (rd16(r+2)==INVALID_U16) return 0;
    return rd16(r+4);
}

static void print_help(void) {
    printf("\x1b[2J\x1b[1;1H");
    printf("SA3DS 0.0.9.0E - REAL SONIC 3DS\n\n");
    printf("L/R      previous/next animation\n");
    printf("A        pause animation\n");
    printf("B        reset animation frame\n");
    printf("X        cycle GPU culling mode\n");
    printf("Y        toggle debug V inversion\n");
    printf("D-Left   rotate Sonic left\n");
    printf("D-Right  rotate Sonic right\n");
    printf("D-Up     zoom in\n");
    printf("D-Down   zoom out\n");
    printf("START    exit\n\n");
    printf("Owner-generated SA3C/SA3T only.\n");
    printf("No Sega assets built into checkpoint.\n");
}

static void print_status(void) {
    const AnimPreset *p=&kPresets[g_preset];
    uint16_t frames=action_frame_count(&g_character,p->action_slot);
    const char *cull=g_cull_mode==0?"NONE":(g_cull_mode==1?"BACK_CCW":"FRONT_CCW");
    printf("\x1b[14;1HAnim: %-11s             ",p->name);
    printf("\x1b[15;1HDense ID:%3u  ACTION:%3u      ",p->dense_id,p->action_slot);
    printf("\x1b[16;1HFrame:%6.2f / %-3u %s      ",g_anim_frame,frames,g_paused?"PAUSE":"PLAY ");
    printf("\x1b[17;1HNodes:%3lu Draws:%3lu Tris:%4lu ",
        (unsigned long)g_stats.visited_nodes,(unsigned long)g_stats.draw_calls,(unsigned long)g_stats.triangles);
    printf("\x1b[18;1HSubmitted vertices:%5lu       ",(unsigned long)g_gpu.frame_vertices);
    printf("\x1b[19;1HTextures:%2lu  VFlip:%s          ",(unsigned long)g_gpu.texture_count,g_gpu.v_flip_debug?"DBG-ON":"OFF   ");
    printf("\x1b[20;1HCull:%-10s Dist:%4.1f       ",cull,g_camera_distance);
    printf("\x1b[21;1HCPU submit:%3llums  Linear:%4luKB ",
        (unsigned long long)g_last_submit_ms,(unsigned long)(linearSpaceFree()/1024u));
    printf("\x1b[22;1HSA3C:%luKB SA3T:%luKB           ",
        (unsigned long)(g_sa3c_size/1024u),(unsigned long)(g_sa3t_size/1024u));
}

static void fail_screen(const char *msg) {
    printf("\nERROR: %s\nPress START to exit.\n",msg?msg:"unknown");
    while (aptMainLoop()) {
        hidScanInput(); if (hidKeysDown()&KEY_START) break; gspWaitForVBlank();
    }
}

static int load_owner_packages(void) {
    if (!sa3ds_read_file("romfs:/sa3ds/sonic.sa3c",&g_sa3c_bytes,&g_sa3c_size)) return 0;
    if (!sa3ds_read_file("romfs:/sa3ds/sonic.sa3t",&g_sa3t_bytes,&g_sa3t_size)) return 0;
    if (sa3c_character_open(&g_character,g_sa3c_bytes,g_sa3c_size)!=SA3C_OK) return 0;
    if (sa3c_character_validate_references(&g_character)!=SA3C_OK) return 0;
    if (sa3t_open(&g_texture_archive,g_sa3t_bytes,g_sa3t_size)!=SA3T_OK) return 0;
    if (g_texture_archive.texture_count!=g_character.sections[6].count) return 0;
    g_node_count=g_character.sections[0].count;
    g_local=(SA3CTransform*)calloc(g_node_count,sizeof(*g_local));
    g_world=(SA3CMatrix*)calloc(g_node_count,sizeof(*g_world));
    return g_local && g_world;
}

static int init_scene(void) {
    g_dvlb=DVLB_ParseFile((u32*)sonic_shbin,sonic_shbin_size);
    if (!g_dvlb) return 0;
    shaderProgramInit(&g_program);
    shaderProgramSetVsh(&g_program,&g_dvlb->DVLE[0]);
    C3D_BindProgram(&g_program);
    g_u_projection=shaderInstanceGetUniformLocation(g_program.vertexShader,"projection");
    g_u_modelview=shaderInstanceGetUniformLocation(g_program.vertexShader,"modelView");
    if (g_u_projection<0 || g_u_modelview<0) return 0;

    C3D_AttrInfo *attr=C3D_GetAttrInfo();
    AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr,0,GPU_FLOAT,3); /* position */
    AttrInfo_AddLoader(attr,1,GPU_FLOAT,2); /* UV */

    Mtx_PerspTilt(&g_projection,C3D_AngleFromDegrees(55.0f),C3D_AspectRatioTop,0.1f,1000.0f,false);
    C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL);
    C3D_AlphaTest(true,GPU_GREATER,15);
    C3D_CullFace(GPU_CULL_NONE);

    return sa3ds_sonic_gpu_init(&g_gpu,&g_character,&g_texture_archive);
}

static int sample_pose(void) {
    const AnimPreset *p=&kPresets[g_preset];
    uint16_t root=INVALID_U16;
    SA3CPoseResult pr=sa3c_pose_sample_action(&g_character,p->action_slot,g_anim_frame,1,g_local,g_node_count,&root);
    if (pr!=SA3C_POSE_OK || root==INVALID_U16) return 0;
    pr=sa3c_pose_build_world(&g_character,root,g_local,g_node_count,g_world,g_node_count);
    if (pr!=SA3C_POSE_OK) return 0;

    C3D_Mtx modelView;
    Mtx_Identity(&modelView);
    Mtx_Translate(&modelView,-1.2f,-0.3f,-g_camera_distance,true);
    Mtx_RotateY(&modelView,g_model_yaw,true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_projection,&g_projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&modelView);

    switch (g_cull_mode) {
        default: case 0: C3D_CullFace(GPU_CULL_NONE); break;
        case 1: C3D_CullFace(GPU_CULL_BACK_CCW); break;
        case 2: C3D_CullFace(GPU_CULL_FRONT_CCW); break;
    }
    return sa3ds_sonic_gpu_draw(&g_gpu,root,g_world,g_node_count,&g_stats);
}

static void cleanup(void) {
    sa3ds_sonic_gpu_destroy(&g_gpu);
    free(g_world); free(g_local); g_world=NULL; g_local=NULL;
    sa3ds_free_file(g_sa3t_bytes); sa3ds_free_file(g_sa3c_bytes);
    g_sa3t_bytes=g_sa3c_bytes=NULL;
    if (g_dvlb) { shaderProgramFree(&g_program); DVLB_Free(g_dvlb); g_dvlb=NULL; }
}

int main(void) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM,NULL);
    print_help();
    if (R_FAILED(romfsInit())) { fail_screen("romfsInit failed"); gfxExit(); return 1; }
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) { fail_screen("C3D_Init failed"); romfsExit(); gfxExit(); return 1; }

    g_target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    if (!g_target) { fail_screen("render target allocation failed"); C3D_Fini(); romfsExit(); gfxExit(); return 1; }
    C3D_RenderTargetSetOutput(g_target,GFX_TOP,GFX_LEFT,DISPLAY_TRANSFER_FLAGS);

    if (!load_owner_packages()) {
        fail_screen("owner SA3C/SA3T missing or invalid");
        cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 1;
    }
    if (!init_scene()) {
        fail_screen("Citro3D Sonic renderer init failed");
        cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 1;
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 down=hidKeysDown();
        u32 held=hidKeysHeld();
        if (down&KEY_START) break;
        if (down&KEY_L) { g_preset=(g_preset+PRESET_COUNT-1)%PRESET_COUNT; g_anim_frame=0; }
        if (down&KEY_R) { g_preset=(g_preset+1)%PRESET_COUNT; g_anim_frame=0; }
        if (down&KEY_A) g_paused=!g_paused;
        if (down&KEY_B) g_anim_frame=0.0f;
        if (down&KEY_X) g_cull_mode=(g_cull_mode+1)%3;
        if (down&KEY_Y) g_gpu.v_flip_debug=!g_gpu.v_flip_debug;
        if (held&KEY_DLEFT)  g_model_yaw-=C3D_AngleFromDegrees(1.5f);
        if (held&KEY_DRIGHT) g_model_yaw+=C3D_AngleFromDegrees(1.5f);
        if (held&KEY_DUP)    g_camera_distance=fmaxf(10.0f,g_camera_distance-0.12f);
        if (held&KEY_DDOWN)  g_camera_distance=fminf(30.0f,g_camera_distance+0.12f);
        if (!g_paused) g_anim_frame+=0.5f;

        uint64_t t0=osGetTime();
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(g_target,C3D_CLEAR_ALL,CLEAR_COLOR,0);
        C3D_FrameDrawOn(g_target);
        if (!sample_pose()) {
            C3D_FrameEnd(0);
            fail_screen("pose sample/draw failed");
            break;
        }
        C3D_FrameEnd(0);
        g_last_submit_ms=osGetTime()-t0;
        print_status();
    }

    cleanup();
    C3D_Fini();
    romfsExit();
    gfxExit();
    return 0;
}
