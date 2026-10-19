#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "sonic_shbin.h"
#include "owner_assets.h"
#include "sonic_gpu.h"
#include "lab_gpu.h"
#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sa3t_texture.h"
#include "sa3c_gpu_preflight.h"
#include "../../src/game/sonic_action_preflight.h"
#include "../../src/game/parity_lab.h"
#include "../../src/game/sonic_animation_sync.h"
#include "../../src/game/sonic_animation_runtime.h"
#include "../../src/platform/3ds/input_adapter.h"
#include "../../src/sa1/physics/physics_constants.h"
#include "../../src/sa1/physics/physics_trace.h"
#include "../../src/sa1/physics/steam_flat.h"

#define CLEAR_COLOR 0xF8FAFCFFu
#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
#define INVALID_U16 0xFFFFu
#define PI 3.14159265358979323846f

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
static SA3DSSonicGpu g_sonic_gpu;
static SA3DSVisualReport g_visual_report;
static SA3DSGeometryReport g_geometry_report;
static SA3DSActionPreflightReport g_action_report;
static SA3DSLabGpu g_lab_gpu;
static SA3CRenderStats g_stats;

static SA1PlayerState g_player;
static SA3DSSonicAnimationSync g_anim_sync;
static SA3DSSonicAnimationRuntime g_anim_runtime;
static SA3DSSonicAnimationObservation g_observation;
static SA3DSSonicAnimationSelection g_selection;
static SA3DSParityZoneId g_selected_zone=SA3DS_PARITY_ZONE_CALIBRATION;
static float g_camera_yaw=0.0f;
static int g_dev_overlay=1;
static int g_paused=0;
static int g_cull_mode=0;
static uint64_t g_last_submit_ms=0;
static FILE *g_trace_file=0;
static unsigned g_trace_frames=0;

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

static float wrap_angle(float a) {
    while (a > PI) a -= 2.0f*PI;
    while (a < -PI) a += 2.0f*PI;
    return a;
}

static void trace_stop(void) {
    if (g_trace_file) fclose(g_trace_file);
    g_trace_file=0;
}

static void trace_toggle(void) {
    if (g_trace_file) { trace_stop(); return; }
    mkdir("sdmc:/3ds/SA3DS",0777);
    mkdir("sdmc:/3ds/SA3DS/traces",0777);
    g_trace_file=fopen("sdmc:/3ds/SA3DS/traces/white_zone_trace.csv","wb");
    g_trace_frames=0;
    if (g_trace_file && sa1_trace_write_header(g_trace_file)!=0) trace_stop();
}

static void reset_selected_zone(void) {
    (void)sa3ds_parity_lab_reset_player(&g_player,g_selected_zone);
    memset(&g_anim_sync,0,sizeof(g_anim_sync));
    memset(&g_anim_runtime,0,sizeof(g_anim_runtime));
    /* A zone reset can happen while paused, so the first post-reset draw
     * must use the new player's pose, never the previous zone's clip. */
    g_observation=sa3ds_sonic_animation_observe(&g_anim_sync,&g_player,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
    g_selection=sa3ds_sonic_animation_select(&g_anim_runtime,&g_player,&g_observation);
    g_camera_yaw=g_player.yaw;
}

static void cycle_zone(int delta) {
    int n=(int)g_selected_zone+delta;
    while(n<0) n+=(int)SA3DS_PARITY_ZONE_COUNT;
    while(n>=(int)SA3DS_PARITY_ZONE_COUNT) n-=(int)SA3DS_PARITY_ZONE_COUNT;
    g_selected_zone=(SA3DSParityZoneId)n;
    reset_selected_zone();
}

static void print_help(void) {
    printf("\x1b[2J\x1b[1;1H");
    printf("SA3DS 0.0.9.1E-WIP6 - WHITE ZONE\n\n");
    printf("Circle Pad  move/steer\n");
    printf("A           jump\n");
    printf("B / X       action\n");
    printf("L / R       camera\n");
    printf("Y           recenter camera\n");
    printf("SELECT      overlay\n");
    printf("SELECT+L/R  prev/next test zone\n");
    printf("SELECT+Y    reset selected zone\n");
    printf("SELECT+X    trace on/off\n");
    printf("START       exit\n\n");
    printf("Developer-only original test geometry.\n");
    printf("No whole-engine 1:1 claim.\n");
}

static void print_status(void) {
    if (!g_dev_overlay) return;
    const SA3DSParityZone *zone=sa3ds_parity_lab_zone_for_z(g_player.pos.z);
    const SA3DSParityZone *selected=sa3ds_parity_lab_zone_by_id(g_selected_zone);
    uint16_t frames=action_frame_count(&g_character,g_selection.binding.action_slot);
    printf("\x1b[15;1HZone: %-17s ",zone?zone->name:"OUTSIDE");
    printf("\x1b[16;1HSel : %-17s ",selected?selected->name:"?");
    printf("\x1b[17;1HPos:%6.1f %5.1f %6.1f ",g_player.pos.x,g_player.pos.y,g_player.pos.z);
    printf("\x1b[18;1HSpd:%5.3f  State:%d %s ",sa1_horizontal_speed(&g_player),(int)g_player.action_state,g_player.grounded?"GND":"AIR");
    printf("\x1b[19;1HAnim:%-11s ID:%3u A:%3u ",g_selection.binding.name?g_selection.binding.name:"?",g_selection.binding.runtime_id,g_selection.binding.action_slot);
    printf("\x1b[20;1HFrame:%6.2f/%-3u %s ",g_selection.sample_frame,frames,g_paused?"PAUSE":"PLAY ");
    printf("\x1b[21;1HDraws:%3lu+%2lu Tris:%4lu+%3lu ",
        (unsigned long)g_stats.draw_calls,(unsigned long)g_lab_gpu.frame_draws,
        (unsigned long)g_stats.triangles,(unsigned long)g_lab_gpu.frame_triangles);
    printf("\x1b[22;1HTrace:%s %5u  CPU:%3llums ",g_trace_file?"REC":"OFF",g_trace_frames,(unsigned long long)g_last_submit_ms);
    printf("\x1b[23;1HSelector:PROVISIONAL  Lin:%4luKB ",(unsigned long)(linearSpaceFree()/1024u));
    printf("\x1b[24;1HTex:%2lu Ref:%2lu RGBA:%4lluKB ",
        (unsigned long)g_visual_report.textures,
        (unsigned long)g_visual_report.referenced_textures,
        (unsigned long long)(g_visual_report.rgba8_bytes/1024u));
    printf("\x1b[25;1HAlpha0:%5llu Partial:%5llu ",
        (unsigned long long)g_visual_report.fully_transparent_texels,
        (unsigned long long)g_visual_report.partially_transparent_texels);
    printf("\x1b[26;1HGPU arena:%5lu/%-5lu %s ",
        (unsigned long)g_sonic_gpu.batch.cursor,
        (unsigned long)g_sonic_gpu.batch.vertex_capacity,
        sa3ds_gpu_batch_result_name(g_sonic_gpu.last_batch_error));
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
    SA3DSVisualResult visual=sa3ds_visual_preflight(&g_character,&g_texture_archive,&g_visual_report);
    if (visual!=SA3DS_VISUAL_OK) {
        printf("\nVisual preflight: %s\n",sa3ds_visual_result_name(visual));
        return 0;
    }
    SA3DSGeometryResult geometry_result = sa3ds_geometry_preflight(
        &g_character, &g_geometry_report);
    if (geometry_result != SA3DS_GEOMETRY_OK) {
        printf("\nSonic geometry preflight: %s\nMesh:%lu Index:%lu Vertex:%lu Component:%u\n",
            sa3ds_geometry_result_name(geometry_result),
            (unsigned long)g_geometry_report.failing_mesh_index,
            (unsigned long)g_geometry_report.failing_index_index,
            (unsigned long)g_geometry_report.failing_vertex_index,
            (unsigned)g_geometry_report.failing_component);
        return 0;
    }
    g_node_count=g_character.sections[0].count;
    g_local=(SA3CTransform*)calloc(g_node_count,sizeof(*g_local));
    g_world=(SA3CMatrix*)calloc(g_node_count,sizeof(*g_world));
    if (!g_local || !g_world) return 0;
    size_t binding_count = sa3ds_sonic_animation_binding_count();
    SA3DSSonicAnimationBinding *bindings = (SA3DSSonicAnimationBinding *)
        calloc(binding_count, sizeof(*bindings));
    if (!bindings) return 0;
    for (size_t i = 0; i < binding_count; ++i) {
        if (!sa3ds_sonic_animation_binding_at(i, &bindings[i])) {
            free(bindings); return 0;
        }
    }
    SA3DSActionPreflightResult action_result = sa3ds_action_preflight(
        &g_character, bindings, binding_count, &g_action_report);
    free(bindings);
    if (action_result != SA3DS_ACTION_PREFLIGHT_OK) {
        printf("\nSonic ACTION preflight: %s\nRuntime:%u Slot:%u Frame:%.2f Pose:%s\n",
            sa3ds_action_preflight_result_name(action_result),
            g_action_report.failing_runtime_id, g_action_report.failing_action_slot,
            g_action_report.failing_frame, sa3c_pose_result_name(g_action_report.pose_error));
        if (g_action_report.failing_track) {
            printf("Channel:%lu Track:%u Key:%lu\n",
                (unsigned long)g_action_report.failing_channel_index,
                (unsigned)g_action_report.failing_track,
                (unsigned long)g_action_report.failing_key_index);
        }
        return 0;
    }
    return 1;
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
    AttrInfo_AddLoader(attr,0,GPU_FLOAT,3);
    AttrInfo_AddLoader(attr,1,GPU_FLOAT,2);
    Mtx_PerspTilt(&g_projection,C3D_AngleFromDegrees(55.0f),C3D_AspectRatioTop,0.1f,1000.0f,false);
    C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL);
    C3D_AlphaTest(true,GPU_GREATER,15);
    C3D_CullFace(GPU_CULL_NONE);
    if (!sa3ds_sonic_gpu_init(&g_sonic_gpu,&g_character,&g_texture_archive)) return 0;
    if (!sa3ds_lab_gpu_init(&g_lab_gpu)) return 0;
    return 1;
}

static int sample_pose(void) {
    uint16_t root=INVALID_U16;
    SA3CPoseResult pr=sa3c_pose_sample_action(&g_character,
        g_selection.binding.action_slot,g_selection.sample_frame,1,g_local,g_node_count,&root);
    if (pr!=SA3C_POSE_OK || root==INVALID_U16) return 0;
    pr=sa3c_pose_build_world(&g_character,root,g_local,g_node_count,g_world,g_node_count);
    if (pr!=SA3C_POSE_OK) return 0;

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_projection,&g_projection);

    /* White Zone geometry is transformed CPU-side into player-relative camera
       space. The 3DS matrix only supplies a simple chase-camera offset. */
    C3D_Mtx lab_mv;
    Mtx_Identity(&lab_mv);
    Mtx_Translate(&lab_mv,0.0f,-2.2f,-18.0f,true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&lab_mv);
    if (!sa3ds_lab_gpu_draw(&g_lab_gpu,&g_player,g_camera_yaw)) return 0;

    C3D_Mtx sonic_mv;
    Mtx_Identity(&sonic_mv);
    Mtx_Translate(&sonic_mv,-1.2f,-0.3f,-18.0f,true);
    Mtx_RotateY(&sonic_mv,wrap_angle(g_player.yaw-g_camera_yaw),true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&sonic_mv);
    switch(g_cull_mode) {
        default: case 0: C3D_CullFace(GPU_CULL_NONE); break;
        case 1: C3D_CullFace(GPU_CULL_BACK_CCW); break;
        case 2: C3D_CullFace(GPU_CULL_FRONT_CCW); break;
    }
    return sa3ds_sonic_gpu_draw(&g_sonic_gpu,root,g_world,g_node_count,&g_stats);
}

static void cleanup(void) {
    trace_stop();
    sa3ds_lab_gpu_destroy(&g_lab_gpu);
    sa3ds_sonic_gpu_destroy(&g_sonic_gpu);
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
        fail_screen("owner SA3C/SA3T missing or invalid"); cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 1;
    }
    if (!init_scene()) {
        fail_screen("Citro3D White Zone init failed"); cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 1;
    }
    reset_selected_zone();
    g_observation=sa3ds_sonic_animation_observe(&g_anim_sync,&g_player,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
    g_selection=sa3ds_sonic_animation_select(&g_anim_runtime,&g_player,&g_observation);

    while (aptMainLoop()) {
        hidScanInput();
        u32 down=hidKeysDown(), held=hidKeysHeld(), up=hidKeysUp();
        if (down&KEY_START) break;
        int modifier=(held&KEY_SELECT)!=0;
        if ((down&KEY_SELECT) && !(held&(KEY_L|KEY_R|KEY_X|KEY_Y))) g_dev_overlay=!g_dev_overlay;
        if (modifier && (down&KEY_L)) cycle_zone(-1);
        if (modifier && (down&KEY_R)) cycle_zone(1);
        if (modifier && (down&KEY_Y)) reset_selected_zone();
        if (modifier && (down&KEY_X)) trace_toggle();
        if (!modifier && (down&KEY_Y)) g_camera_yaw=g_player.yaw;
        if (!modifier && (held&KEY_L)) g_camera_yaw-=0.035f;
        if (!modifier && (held&KEY_R)) g_camera_yaw+=0.035f;

        SA1Input in=sa3ds_read_logical_input(down,held,up);
        if (modifier) {
            in.action_pressed=in.action_held=in.action_released=false;
            in.spindash_shortcut_pressed=in.spindash_shortcut_held=in.spindash_shortcut_released=false;
            in.lightspeed_shortcut_pressed=in.lightspeed_shortcut_held=in.lightspeed_shortcut_released=false;
        }
        if (!g_paused) {
            sa1_physics_step_steam_sonic_flat(&g_player,&in,g_camera_yaw,
                sa3ds_parity_lab_world(),&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,NULL);
            SA1PlayerState physics_done=g_player;
            g_observation=sa3ds_sonic_animation_observe(&g_anim_sync,&g_player,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
            g_selection=sa3ds_sonic_animation_select(&g_anim_runtime,&g_player,&g_observation);
            /* Hard architecture gate: animation observation/selection is an
               observer. Any write back into physics state is a runtime error. */
            if (memcmp(&physics_done,&g_player,sizeof(g_player))!=0) {
                fail_screen("animation mutated physics state");
                break;
            }
            if (g_trace_file) {
                if (sa1_trace_write_frame(g_trace_file,&g_player,&in)!=0) trace_stop();
                else { g_trace_frames++; if ((g_trace_frames&63u)==0) fflush(g_trace_file); }
            }
        }

        float d=wrap_angle(g_player.yaw-g_camera_yaw);
        if (!modifier && !(held&(KEY_L|KEY_R))) g_camera_yaw+=d*0.025f;

        uint64_t t0=osGetTime();
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(g_target,C3D_CLEAR_ALL,CLEAR_COLOR,0);
        C3D_FrameDrawOn(g_target);
        if (!sample_pose()) {
            C3D_FrameEnd(0); fail_screen("White Zone pose/draw failed"); break;
        }
        C3D_FrameEnd(0);
        g_last_submit_ms=osGetTime()-t0;
        print_status();
    }

    cleanup();
    C3D_Fini(); romfsExit(); gfxExit();
    return 0;
}
