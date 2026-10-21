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
#include "emerald_coast_gpu.h"
#include "sa3c_character.h"
#include "sa3c_pose.h"
#include "sa3t_texture.h"
#include "sa3c_gpu_preflight.h"
#include "emerald_coast_visual.h"
#include "emerald_coast_collision.h"
#include "emerald_coast_replay.h"
#include "../../src/sa1/physics/movement.h"
#include "../../src/game/sonic_pose_metrics.h"
#include "../../src/game/sonic_action_preflight.h"
#include "../../src/game/parity_lab.h"
#include "../../src/game/parity_hud.h"
#include "../../src/game/parity_trace.h"
#include "../../src/game/sonic_animation_sync.h"
#include "../../src/game/sonic_animation_runtime.h"
#include "../../src/game/white_zone_tick.h"
#include "../../src/platform/3ds/input_adapter.h"
#include "../../src/sa1/physics/physics_constants.h"
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
static SA3DSOwnerSource g_owner_source=SA3DS_OWNER_SOURCE_NONE;
static SA3CCharacter g_character;
static SA3TArchive g_texture_archive;
static SA3CTransform *g_local;
static SA3CTransform *g_transition_local;
static SA3CMatrix *g_world;
static size_t g_node_count;
static SA3DSSonicGpu g_sonic_gpu;
static SA3DSVisualReport g_visual_report;
static SA3DSGeometryReport g_geometry_report;
static SA3DSActionPreflightReport g_action_report;
static SA3DSLabGpu g_lab_gpu;
static uint8_t *g_ec_bytes;
static size_t g_ec_size;
static SA3DSECVisual g_ec_visual;
static SA3DSECGpu g_ec_gpu;
static int g_emerald_coast;
static int g_ec_combined;
static int g_ec_replay;
static unsigned g_ec_replay_tick;
static int g_ec_replay_pause;
static unsigned g_rendered_frames;
static SA3DSECCollision g_ec_collision;
static C3D_Mtx g_ec_view;
static SA3CRenderStats g_stats;

static SA1PlayerState g_player;
static SA3DSSonicAnimationSync g_anim_sync;
static SA3DSSonicAnimationRuntime g_anim_runtime;
static SA3DSSonicAnimationObservation g_observation;
static SA3DSSonicAnimationSelection g_selection;
static SA3DSParityZoneId g_selected_zone=SA3DS_PARITY_ZONE_CALIBRATION;
static SA3DSParityCameraState g_camera={SA3DS_PARITY_CAMERA_FOLLOW,0.0f};
static int g_dev_overlay=1;
static int g_paused=0;
static SA3DSWhiteZoneTick g_tick_clock;
static uint64_t g_last_frame_ms=0;
static int g_cull_mode=0;
static uint64_t g_last_submit_ms=0;
static FILE *g_trace_file=0;
static unsigned g_trace_frames=0;
static PrintConsole g_boot_console;
static unsigned g_boot_steps=0;
static int g_boottrace_first_frame=1;

/* Pass10A: completely separate, deterministic visual inspection. The normal
   White Zone player/physics/animation selector is not modified. */
static int g_pose_lab=0;
static const struct { uint16_t slot; const char *name; } g_pose_actions[]={
    {1,"Stand"},{3,"Walk1"},{4,"Walk2"},{5,"Walk3"},
    {6,"Run1"},{18,"Run2"},{14,"JumpSpin"}
};
#define POSE_ACTION_COUNT (sizeof(g_pose_actions)/sizeof(g_pose_actions[0]))
static unsigned g_pose_action=0;
static float g_pose_frame=0.0f;
static float g_pose_yaw=0.0f;
static float g_pose_distance=17.0f;
static float g_pose_center_y=0.0f;
static int g_pose_center_set=0;
static int g_pose_bind=0;
static int g_pose_metrics_dirty=1;
static SA3DSPoseMetrics g_pose_metrics;


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

/* TextureSyncFix is an on-device diagnosis. No Sega assets are embedded.
 * Show AND PRESENT the last checkpoint before potentially failing work.
 * A plain-text SD log supplements the screen but is never required.
 */
static void boot_log(const char *phase, const char *detail) {
    mkdir("sdmc:/3ds/SA3DS",0777);
    FILE *f=fopen("sdmc:/3ds/SA3DS/pass10_boot.log","ab");
    if (!f) return;
    fprintf(f,"%03u %s %s\n",g_boot_steps,phase,detail?detail:"-");
    fflush(f);
    fclose(f);
}

static void boot_checkpoint(const char *step) {
    ++g_boot_steps;
    consoleSelect(&g_boot_console);
    printf("\x1b[2J\x1b[1;1H");
    printf("SA3DS Pass10A Pose Lab\n");
    printf("Stage %03u\n\n",g_boot_steps);
    printf("%s\n\n",step);
    printf("If stuck, report stage number.\n");
    printf("SD log: /3ds/SA3DS/\n");
    printf("pass10_boot.log\n\n");
    printf("START only works if app loops.\n");
    gfxFlushBuffers();
    gfxScreenSwapBuffers(GFX_BOTTOM,false);
    gspWaitForVBlank();
    boot_log("ENTER",step);
}

static void boot_note(const char *detail) {
    boot_log("OK",detail);
}

static int boot_menu(void) {
    boot_log("MENU","enter");
    /* Owner-side emulator/hardware automation hook.  The marker contains no
       game data and avoids relying on host keyboard-to-3DS mappings. */
    FILE *autoboot=fopen("sdmc:/3ds/SA3DS/emerald_coast_autoboot.flag","rb");
    boot_log("MENU","autoboot marker checked");
    if (autoboot) { int selected=fgetc(autoboot);fclose(autoboot);return selected=='W'?3:6; }
    consoleSelect(&g_boot_console);
    printf("\x1b[2J\x1b[1;1H");
    printf("SA3DS Pass10A Pose Lab\n\n");
    printf("Select ONE test:\n\n");
    printf("B = GPU-only colored screen\n");
    printf("X = Sonic assets without GPU\n");
    printf("Y = CPU texture upload only\n");
    printf("A = FULL White Zone WIP9\n");
    printf("L = Pass10A Pose Lab\n\n");
    printf("R = Emerald Coast Act 1\n\n");
    printf("START = exit\n\n");
    printf("Run B, then X, then A.\n");
    printf("No owner assets needed for B.\n");
    printf("Original physics preserved.\n");
    boot_log("MENU","ready for mode selection");
    while (aptMainLoop()) {
        hidScanInput();
        u32 down=hidKeysDown();
        if (down&KEY_START) return 0;
        if (down&KEY_B) return 1;
        if (down&KEY_X) return 2;
        if (down&KEY_Y) return 4;
        if (down&KEY_L) return 5;
        if (down&KEY_R) return 6;
        if (down&KEY_A) return 3;
        /* Present both screens: the emulator's frame boundary follows TOP.
         * Bottom-only swaps can leave the startup console black at 0 FPS. */
        gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();
    }
    return 0;
}

static void boot_result(const char *headline, const char *detail) {
    consoleSelect(&g_boot_console);
    printf("\x1b[2J\x1b[1;1H");
    printf("SA3DS Pass10A Pose Lab\n\n");
    printf("%s\n\n",headline);
    if (detail) printf("%s\n\n",detail);
    printf("A/B/X/Y: exit (relaunch)\n");
    printf("START: exit to Homebrew\n");
    boot_log("RESULT",headline);
    while (aptMainLoop()) {
        hidScanInput();
        u32 down=hidKeysDown();
        if (down&(KEY_START|KEY_A|KEY_B|KEY_X|KEY_Y)) break;
        gfxFlushBuffers(); gfxScreenSwapBuffers(GFX_BOTTOM,false); gspWaitForVBlank();
    }
}

static void trace_stop(void) {
    if (g_trace_file) fclose(g_trace_file);
    g_trace_file=0;
}

static void trace_toggle(void) {
    if (g_trace_file) { trace_stop(); return; }
    mkdir("sdmc:/3ds/SA3DS",0777);
    mkdir("sdmc:/3ds/SA3DS/traces",0777);
    g_trace_file=fopen(g_emerald_coast?
        "sdmc:/3ds/SA3DS/traces/emerald_coast_trace.csv":
        "sdmc:/3ds/SA3DS/traces/white_zone_trace.csv","wb");
    g_trace_frames=0;
    if (g_trace_file && sa3ds_parity_trace_write_header(g_trace_file)!=0) trace_stop();
}

static void reset_selected_zone(void) {
    sa3ds_wz_tick_reset(&g_tick_clock);
    g_last_frame_ms=osGetTime();
    if (g_emerald_coast && g_ec_combined) {
        sa1_player_reset(&g_player,&g_ec_collision.world);
        g_ec_collision.reference_y=g_player.pos.y;
    } else (void)sa3ds_parity_lab_reset_player(&g_player,g_selected_zone);
    memset(&g_anim_sync,0,sizeof(g_anim_sync));
    memset(&g_anim_runtime,0,sizeof(g_anim_runtime));
    /* Refresh the pose immediately, even when a zone reset occurs while paused. */
    g_observation=sa3ds_sonic_animation_observe(&g_anim_sync,&g_player,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
    g_selection=sa3ds_sonic_animation_select(&g_anim_runtime,&g_player,&g_observation);
    sa3ds_parity_camera_recenter(&g_camera,g_player.yaw);
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
    printf("SA3DS 0.0.9.1E-WIP7 - WHITE ZONE\n\n");
    printf("Circle Pad  move/steer\n");
    printf("A           jump\n");
    printf("B / X       action\n");
    printf("L / R       camera\n");
    printf("Y           recenter camera\n");
    printf("SELECT      overlay\n");
    printf("SELECT+L/R  prev/next test zone\n");
    printf("SELECT+DLeft FOLLOW / REFERENCE camera\n");
    printf("SELECT+Y    reset selected zone\n");
    printf("SELECT+X    trace on/off\n");
    printf("SELECT+A    pause/resume simulation\n");
    printf("START       exit\n\n");
    printf("Developer-only original test geometry.\n");
    printf("No whole-engine 1:1 claim.\n");
}

static void print_status(void) {
    if (!g_dev_overlay) return;
    const SA3DSParityZone *zone=sa3ds_parity_lab_zone_for_z(g_player.pos.z);
    const SA3DSParityZone *selected=sa3ds_parity_lab_zone_by_id(g_selected_zone);
    uint16_t frames=action_frame_count(&g_character,g_selection.binding.action_slot);
    SA3DSParityHudSnapshot hud;
    sa3ds_parity_hud_snapshot(&hud,&g_player,zone,selected,&g_selection,frames,
        g_trace_file!=0,g_trace_frames,
        (unsigned)(g_stats.draw_calls+g_lab_gpu.frame_draws),
        (unsigned)(g_stats.triangles+g_lab_gpu.frame_triangles),
        linearSpaceFree(),g_camera.mode);
    printf("\x1b[14;1HTest:%-12.12s Sel:%-10.10s ",hud.active_zone,hud.selected_zone);
    printf("\x1b[15;1HF:%6llu P:%5.1f %4.1f %5.1f ",(unsigned long long)hud.sim_frame,hud.pos_x,hud.pos_y,hud.pos_z);
    printf("\x1b[16;1HV:%5.2f %5.2f %5.2f H:%4.2f ",hud.vel_x,hud.vel_y,hud.vel_z,hud.horizontal_speed);
    printf("\x1b[17;1HYaw:%6.3f %s A:%d S:%ld ",hud.heading,hud.grounded?"GND":"AIR",hud.action_state,(long)hud.surface_id);
    printf("\x1b[18;1HN:%4.2f %4.2f %4.2f Sl:%5.3f ",hud.normal_x,hud.normal_y,hud.normal_z,hud.slope_radians);
    printf("\x1b[19;1HAnim:%3u ACTION:%3u Fr:%5.1f/%u ",hud.runtime_animation_id,hud.action_slot,hud.animation_frame,hud.animation_frame_count);
    printf("\x1b[20;1HTrace:%s %5u D:%u T:%u ",hud.trace_recording?"REC":"OFF",hud.trace_frames,hud.draw_calls,hud.triangles);
    printf("\x1b[21;1HLin:%5luKB Cam:%-9s ",(unsigned long)(hud.free_linear_bytes/1024u),sa3ds_parity_camera_mode_name(hud.camera_mode));
    printf("\x1b[22;1HSelector:%s CPU:%3llums ",hud.selector_is_provisional?"PROVISIONAL":"SOURCE",(unsigned long long)g_last_submit_ms);
    printf("\x1b[23;1HPHYSICS_1TO1:NO                     ");
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
    printf("\x1b[27;1HAssets:%s C:%luK T:%luK ",
        sa3ds_owner_source_name(g_owner_source),
        (unsigned long)(g_sa3c_size/1024u),
        (unsigned long)(g_sa3t_size/1024u));
    printf("\x1b[28;1H60Hz:%llu Drop:%llu %s ",
        (unsigned long long)g_tick_clock.total_ticks,
        (unsigned long long)g_tick_clock.dropped_ticks,
        g_paused ? "PAUSED" : "RUN");
}

static void emerald_coast_help(void) {
    consoleSelect(&g_boot_console);
    printf("\x1b[2J\x1b[1;1H");
    printf("SA3DS - EMERALD COAST ACT 1\n\n");
    printf("Authentic PC2004 opening subset.\n");
    printf("500-unit opening residency subset.\n");
    printf("Circle Pad move / A jump\n");
    printf("L/R camera / Y recenter\n");
    printf("SELECT+Y reset / SELECT+X trace\n");
    printf("START exit\n\n");
    printf("Owner package loaded from SD.\n");
    printf("No retail camera/collision claim.\n");
}

static void emerald_coast_status(void) {
    printf("\x1b[14;1HInstances:%4lu  Batches:%3lu       ",
        (unsigned long)g_ec_gpu.instance_count,(unsigned long)g_ec_gpu.draw_count);
    printf("\x1b[15;1HTris:%7lu  Vertices:%7lu    ",
        (unsigned long)g_ec_gpu.triangle_count,(unsigned long)g_ec_gpu.vertex_count);
    printf("\x1b[16;1HTextures:%3lu/%3lu %5luKB       ",
        (unsigned long)g_ec_gpu.resident_texture_count,(unsigned long)g_ec_gpu.texture_count,
        (unsigned long)(g_ec_gpu.texture_bytes/1024u));
    printf("\x1b[17;1HPackage:%5luKB Radius:%5.0f      ",
        (unsigned long)(g_ec_size/1024u),g_ec_gpu.subset_radius);
    printf("\x1b[18;1HLinear free:%5luKB Frame:%3llums ",
        (unsigned long)(linearSpaceFree()/1024u),(unsigned long long)g_last_submit_ms);
    printf("\x1b[19;1HP:%6.1f %6.1f %6.1f %s    ",g_player.pos.x,g_player.pos.y,g_player.pos.z,g_player.grounded?"GND":"AIR");
    printf("\x1b[20;1HV:%5.2f %5.2f %5.2f S:%ld    ",g_player.vel.x,g_player.vel.y,g_player.vel.z,(long)g_player.surface_id);
    printf("\x1b[21;1HN:%4.2f %4.2f %4.2f Wall:%lu  ",g_player.ground_normal.x,g_player.ground_normal.y,g_player.ground_normal.z,(unsigned long)g_ec_collision.wall_hits);
    printf("\x1b[22;1HCollision:%lu Q:%lu Tests:%lu  ",(unsigned long)g_ec_collision.triangle_count,(unsigned long)g_ec_collision.queries,(unsigned long)g_ec_collision.triangle_tests);
    printf("\x1b[23;1HSonic D:%lu Anim:%u Fr:%.1f    ",(unsigned long)g_sonic_gpu.frame_draws,g_selection.binding.runtime_id,g_selection.sample_frame);
    printf("\x1b[24;1HTick:%llu GPU frames:%u       ",(unsigned long long)g_player.sim_frame,g_rendered_frames);
    printf("\x1b[25;1HPartial collision/follow camera ");
    printf("\x1b[26;1HPHYSICS_1TO1:NO                ");
}

static void pose_lab_help(void) {
    consoleSelect(&g_boot_console);
    printf("\x1b[2J\x1b[1;1HSA3DS Pass10A - POSE LAB\n");
    printf("Real Sonic ACTIONs, frozen.\n");
    printf("L/R   rotate model by 45deg\n");
    printf("LEFT/RIGHT  step source frame\n");
    printf("X/B   previous/next ACTION\n");
    printf("A     compare bind vs ACTION\n");
    printf("Y     return Stand at frame 0\n");
    printf("UP/DOWN   zoom in/out\n");
    printf("START exit\n");
    printf("Source data, NOT retail parity.\n");
}

static void pose_lab_status(void) {
    if (!g_pose_lab) return;
    const uint16_t slot=g_pose_actions[g_pose_action].slot;
    printf("\x1b[14;1HAction:%-10.10s Slot:%u       ",g_pose_actions[g_pose_action].name,(unsigned)slot);
    printf("\x1b[15;1HFrame:%5.1f/%u Mode:%-6s       ",g_pose_frame,
        (unsigned)action_frame_count(&g_character,slot),g_pose_bind?"BIND":"ACTION");
    printf("\x1b[16;1HYaw:%6.1fdeg Dist:%4.1f       ",g_pose_yaw*180.0f/PI,g_pose_distance);
    printf("\x1b[17;1HVerts:%lu Tris:%lu Root:%u       ",
        (unsigned long)g_pose_metrics.visible_vertices,
        (unsigned long)g_pose_metrics.triangles,
        (unsigned)g_pose_metrics.root_node);
    printf("\x1b[18;1HX:%6.2f..%6.2f             ",g_pose_metrics.min[0],g_pose_metrics.max[0]);
    printf("\x1b[19;1HY:%6.2f..%6.2f             ",g_pose_metrics.min[1],g_pose_metrics.max[1]);
    printf("\x1b[20;1HZ:%6.2f..%6.2f             ",g_pose_metrics.min[2],g_pose_metrics.max[2]);
    printf("\x1b[21;1HRoot:%5.2f %5.2f %5.2f     ",
        g_pose_metrics.root[0],g_pose_metrics.root[1],g_pose_metrics.root[2]);
    printf("\x1b[22;1HBounds:%.1f x %.1f x %.1f     ",
        g_pose_metrics.max[0]-g_pose_metrics.min[0],
        g_pose_metrics.max[1]-g_pose_metrics.min[1],
        g_pose_metrics.max[2]-g_pose_metrics.min[2]);
    printf("\x1b[23;1HCPU draw:%3llums              ",(unsigned long long)g_last_submit_ms);
    printf("\x1b[24;1HNo gameplay physics in Pose Lab");
    printf("\x1b[25;1HSave metrics: SD:/3ds/SA3DS/");
    printf("\x1b[26;1Hpass10_pose_metrics.csv        ");
}

static void pose_lab_log_metrics(void) {
    mkdir("sdmc:/3ds/SA3DS",0777);
    FILE *f=fopen("sdmc:/3ds/SA3DS/pass10_pose_metrics.csv","ab");
    if (!f) return;
    fprintf(f,"%u,%.3f,%d,%.6f,%.3f,%u,%lu,%lu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n",
        (unsigned)g_pose_actions[g_pose_action].slot,g_pose_frame,g_pose_bind,
        g_pose_yaw,g_pose_distance,(unsigned)g_pose_metrics.root_node,
        (unsigned long)g_pose_metrics.visible_vertices,(unsigned long)g_pose_metrics.triangles,
        g_pose_metrics.min[0],g_pose_metrics.min[1],g_pose_metrics.min[2],
        g_pose_metrics.max[0],g_pose_metrics.max[1],g_pose_metrics.max[2]);
    fclose(f);
}

static void pose_lab_keys(u32 down) {
    int changed=0;
    if(down&KEY_L){g_pose_yaw=wrap_angle(g_pose_yaw-PI*0.25f);changed=1;}
    if(down&KEY_R){g_pose_yaw=wrap_angle(g_pose_yaw+PI*0.25f);changed=1;}
    if(down&KEY_X){g_pose_action=(g_pose_action+POSE_ACTION_COUNT-1u)%POSE_ACTION_COUNT;g_pose_frame=0;changed=1;}
    if(down&KEY_B){g_pose_action=(g_pose_action+1u)%POSE_ACTION_COUNT;g_pose_frame=0;changed=1;}
    const uint16_t frames=action_frame_count(&g_character,g_pose_actions[g_pose_action].slot);
    if((down&KEY_DLEFT) && g_pose_frame>0.0f){g_pose_frame-=1.0f;changed=1;}
    if((down&KEY_DRIGHT) && frames && g_pose_frame<(float)(frames-1u)){
        g_pose_frame+=1.0f;changed=1;
    }
    if((down&KEY_DUP) && g_pose_distance>8.0f){g_pose_distance-=1.0f;changed=1;}
    if((down&KEY_DDOWN) && g_pose_distance<30.0f){g_pose_distance+=1.0f;changed=1;}
    if(down&KEY_A){g_pose_bind=!g_pose_bind;changed=1;}
    if(down&KEY_Y){g_pose_bind=0;g_pose_action=0;g_pose_frame=0;g_pose_yaw=0;g_pose_distance=17.0f;changed=1;}
    if(changed)g_pose_metrics_dirty=1;
}

static void fail_screen(const char *msg) {
    boot_checkpoint(msg?msg:"unknown failure");
    consoleSelect(&g_boot_console);
    printf("\nERROR - see last stage!\n");
    printf("Press START to exit.\n");
    boot_log("FAIL",msg?msg:"unknown failure");
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown()&KEY_START) break;
        gfxFlushBuffers(); gfxScreenSwapBuffers(GFX_BOTTOM,false); gspWaitForVBlank();
    }
}

static int load_owner_packages(void) {
    boot_checkpoint("Loading owner SA3C/SA3T pair");
    if (!sa3ds_load_owner_pair("romfs:/sa3ds", "sdmc:/3ds/SA3DS/assets",
            &g_sa3c_bytes,&g_sa3c_size,&g_sa3t_bytes,&g_sa3t_size,
            &g_owner_source)) {
        printf("\nOwner pair missing in RomFS and SD assets folder.\n");
        return 0;
    }
    boot_checkpoint("Parsing SA3C character");
    if (sa3c_character_open(&g_character,g_sa3c_bytes,g_sa3c_size)!=SA3C_OK) return 0;
    boot_checkpoint("Checking SA3C references");
    if (sa3c_character_validate_references(&g_character)!=SA3C_OK) return 0;
    boot_checkpoint("Parsing SA3T textures");
    if (sa3t_open(&g_texture_archive,g_sa3t_bytes,g_sa3t_size)!=SA3T_OK) return 0;
    boot_checkpoint("Checking textures & materials");
    SA3DSVisualResult visual=sa3ds_visual_preflight(&g_character,&g_texture_archive,&g_visual_report);
    if (visual!=SA3DS_VISUAL_OK) {
        printf("\nVisual preflight: %s\n",sa3ds_visual_result_name(visual));
        return 0;
    }
    boot_checkpoint("Checking Sonic geometry");
    SA3DSGeometryResult geometry_result=sa3ds_geometry_preflight(&g_character,&g_geometry_report);
    if (geometry_result!=SA3DS_GEOMETRY_OK) {
        printf("\nGeometry preflight: %s\nMesh:%lu Index:%lu Vertex:%lu Component:%u\n",
            sa3ds_geometry_result_name(geometry_result),
            (unsigned long)g_geometry_report.failing_mesh_index,
            (unsigned long)g_geometry_report.failing_index_index,
            (unsigned long)g_geometry_report.failing_vertex_index,
            (unsigned)g_geometry_report.failing_component);
        return 0;
    }
    boot_checkpoint("Allocating Sonic pose arrays");
    g_node_count=g_character.sections[0].count;
    g_local=(SA3CTransform*)calloc(g_node_count,sizeof(*g_local));
    g_transition_local=(SA3CTransform*)calloc(g_node_count,sizeof(*g_transition_local));
    g_world=(SA3CMatrix*)calloc(g_node_count,sizeof(*g_world));
    if (!g_local || !g_transition_local || !g_world) return 0;
    size_t binding_count=sa3ds_sonic_animation_binding_count();
    SA3DSSonicAnimationBinding *bindings=(SA3DSSonicAnimationBinding*)
        calloc(binding_count,sizeof(*bindings));
    if (!bindings) return 0;
    for (size_t i=0;i<binding_count;i++) {
        if (!sa3ds_sonic_animation_binding_at(i,&bindings[i])) {
            free(bindings); return 0;
        }
    }
    boot_checkpoint("Checking Sonic ACTION frames");
    SA3DSActionPreflightResult action_result=sa3ds_action_preflight(
        &g_character,bindings,binding_count,&g_action_report);
    free(bindings);
    if (action_result!=SA3DS_ACTION_PREFLIGHT_OK) {
        printf("\nACTION preflight: %s\nRuntime:%u Slot:%u Frame:%.2f Pose:%s\n",
            sa3ds_action_preflight_result_name(action_result),
            g_action_report.failing_runtime_id,g_action_report.failing_action_slot,
            g_action_report.failing_frame,sa3c_pose_result_name(g_action_report.pose_error));
        if (g_action_report.failing_track) {
            printf("Channel:%lu Track:%u Key:%lu\n",
                (unsigned long)g_action_report.failing_channel_index,
                (unsigned)g_action_report.failing_track,
                (unsigned long)g_action_report.failing_key_index);
        }
        return 0;
    }
    boot_note("All Sonic asset preflights passed");
    return 1;
}

static int load_emerald_coast_package(void) {
    boot_checkpoint("Loading Emerald Coast act-0 package from SD");
    if (!sa3ds_read_file("sdmc:/3ds/SA3DS/assets/emerald_coast_act0.sa3dspak",
            &g_ec_bytes,&g_ec_size)) return 0;
    boot_checkpoint("Validating Emerald Coast package and references");
    SA3DSECVisualResult r=sa3ds_ec_visual_open(&g_ec_visual,g_ec_bytes,g_ec_size);
    if (r!=SA3DS_EC_VISUAL_OK) {
        printf("\nEmerald Coast visual: %s\n",sa3ds_ec_visual_result_name(r));
        return 0;
    }
    boot_note("Emerald Coast act-0 package preflight passed");
    if(g_ec_combined){
        uint8_t *collision_bytes=NULL;size_t collision_size=0;
        boot_checkpoint("Loading and indexing owner Emerald Coast collision");
        if(!sa3ds_read_file("sdmc:/3ds/SA3DS/assets/emerald_coast_collision.sa3dspak",
            &collision_bytes,&collision_size))return 0;
        int ok=sa3ds_ec_collision_open(&g_ec_collision,collision_bytes,collision_size);
        sa3ds_free_file(collision_bytes);
        if(!ok||!sa3ds_ec_collision_spawn(&g_ec_collision,SA3DS_EC_SPAWN_X,SA3DS_EC_SPAWN_Z,0.0f))return 0;
        boot_note("Provisional spawn: imported support at x=0 z=-50; not retail spawn");
    }
    return 1;
}

static void log_texture_stage(const char *phase, uint32_t texture_index,
    uint32_t texture_count, uint16_t width, uint16_t height, void *unused) {
    (void)unused;
    char line[192];
    snprintf(line,sizeof(line),"texture %lu/%lu (%u x %u): %s",
        (unsigned long)texture_index,(unsigned long)texture_count,
        (unsigned)width,(unsigned)height, phase?phase:"-");
    boot_log("TEX",line);
    /* Display actual sub-stage on bottom screen too: a dead SD log must not
     * hide which texture was last reached. No wait on any GPU PPF event. */
    consoleSelect(&g_boot_console);
    printf("\x1b[2J\x1b[1;1HSA3DS Pass10A Pose Lab\n");
    printf("Texture %lu / %lu\n",(unsigned long)texture_index,
        (unsigned long)texture_count);
    printf("%u x %u\n\n%s\n",width,height,phase?phase:"-");
    gfxFlushBuffers();
    gfxScreenSwapBuffers(GFX_BOTTOM,false);
}

static int init_scene(void) {
    boot_checkpoint("Parsing 3DS shader");
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
    Mtx_PerspTilt(&g_projection,C3D_AngleFromDegrees(55.0f),C3D_AspectRatioTop,0.1f,
        g_emerald_coast?5000.0f:1000.0f,false);
    C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL);
    C3D_AlphaTest(true,GPU_GREATER,15);
    C3D_CullFace(GPU_CULL_NONE);
    if (g_emerald_coast) {
        boot_checkpoint("CPU-swizzling Emerald Coast opening subset");
        if (!sa3ds_ec_gpu_init(&g_ec_gpu,&g_ec_visual,500.0f,log_texture_stage,NULL)) return 0;
        boot_note("Emerald Coast opening subset resident");
        if (g_ec_combined && !sa3ds_sonic_gpu_init_traced(&g_sonic_gpu,
                &g_character,&g_texture_archive,log_texture_stage,NULL)) return 0;
        if(g_ec_combined){
            uint16_t root=INVALID_U16;SA3DSPoseMetrics stand;
            if(sa3c_pose_sample_action(&g_character,1,0,0,g_local,g_node_count,&root)!=SA3C_POSE_OK||
               sa3c_pose_build_world(&g_character,root,g_local,g_node_count,g_world,g_node_count)!=SA3C_POSE_OK||
               !sa3ds_pose_metrics(&g_character,root,g_world,g_node_count,&stand))return 0;
            char scale_note[192];snprintf(scale_note,sizeof(scale_note),
                "Stand bounds=(%.3f,%.3f,%.3f)..(%.3f,%.3f,%.3f) height=%.3f unit-scale=1 center_height=5.4; partial contact adapter",
                stand.min[0],stand.min[1],stand.min[2],stand.max[0],stand.max[1],stand.max[2],
                stand.max[1]-stand.min[1]);boot_log("SCALE",scale_note);
        }
    } else {
        boot_checkpoint("CPU swizzle Sonic textures");
        if (!sa3ds_sonic_gpu_init_traced(&g_sonic_gpu,&g_character,&g_texture_archive,
                                       log_texture_stage,NULL)) return 0;
        boot_checkpoint("CPU-uploaded textures OK; allocating White Zone geometry");
        if (!sa3ds_lab_gpu_init(&g_lab_gpu)) return 0;
    }
    return 1;
}

static int sample_pose(void) {
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_projection,&g_projection);
    if (g_emerald_coast) {
        C3D_Mtx stage_mv;
        if(g_ec_combined){
            Mtx_LookAt(&stage_mv,FVec3_New(g_player.pos.x-sinf(g_camera.yaw)*40.0f,
                g_player.pos.y+18.0f,g_player.pos.z-cosf(g_camera.yaw)*40.0f),
                FVec3_New(g_player.pos.x,g_player.pos.y+3.0f,g_player.pos.z),
                FVec3_New(0,1,0),false);
        }else Mtx_LookAt(&stage_mv,FVec3_New(0.0f,95.0f,-260.0f),
            FVec3_New(0.0f,15.0f,80.0f),FVec3_New(0.0f,1.0f,0.0f),false);
        g_ec_view=stage_mv;
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&stage_mv);
        C3D_CullFace(GPU_CULL_NONE);
        if (!sa3ds_ec_gpu_draw(&g_ec_gpu,0)) return 0;
        if (!g_ec_combined) return sa3ds_ec_gpu_draw(&g_ec_gpu,1);
    }
    uint16_t root=INVALID_U16;
    SA3CPoseResult pr;
    if (g_pose_lab) {
        uint16_t slot=g_pose_actions[g_pose_action].slot;
        pr=sa3c_pose_sample_action(&g_character,slot,g_pose_frame,0,g_local,g_node_count,&root);
        if(pr==SA3C_POSE_OK && g_pose_bind)
            pr=sa3c_pose_bind(&g_character,g_local,g_node_count);
    } else {
        if (g_selection.transition_active) {
            pr=sa3c_pose_sample_transition(&g_character,
                g_selection.transition_from_action_slot,
                g_selection.transition_from_frame,
                g_selection.binding.action_slot,
                g_selection.transition_weight,
                g_local,g_transition_local,g_node_count,&root);
        } else {
            int loop=g_selection.binding.property!=4;
            pr=sa3c_pose_sample_action(&g_character,
                g_selection.binding.action_slot,g_selection.sample_frame,loop,
                g_local,g_node_count,&root);
        }
    }
    if (pr!=SA3C_POSE_OK || root==INVALID_U16) return 0;
    pr=sa3c_pose_build_world(&g_character,root,g_local,g_node_count,g_world,g_node_count);
    if (pr!=SA3C_POSE_OK) return 0;

    if (g_pose_lab && g_pose_metrics_dirty) {
        if (!sa3ds_pose_metrics(&g_character,root,g_world,g_node_count,&g_pose_metrics)) return 0;
        if (!g_pose_center_set) {
            g_pose_center_y=(g_pose_metrics.min[1]+g_pose_metrics.max[1])*0.5f;
            g_pose_center_set=1;
        }
        pose_lab_log_metrics();
        g_pose_metrics_dirty=0;
    }
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_projection,&g_projection);

    /* White Zone geometry is transformed CPU-side into player-relative camera
       space. The 3DS matrix only supplies a simple chase-camera offset. */
    C3D_Mtx lab_mv;
    Mtx_Identity(&lab_mv);
    Mtx_Translate(&lab_mv,0.0f,-2.2f,-18.0f,true);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&lab_mv);
    if (!g_pose_lab && !g_emerald_coast && !sa3ds_lab_gpu_draw(&g_lab_gpu,&g_player,g_camera.yaw)) return 0;

    C3D_Mtx sonic_mv;
    Mtx_Identity(&sonic_mv);
    if(g_emerald_coast){
        SA3CMatrix actor;C3D_Mtx model;
        sa3ds_ec_model_transform(&g_player,&actor);
        for(unsigned row=0;row<4;row++)model.r[row]=FVec4_New(
            actor.m[row],actor.m[4+row],actor.m[8+row],actor.m[12+row]);
        Mtx_Multiply(&sonic_mv,&g_ec_view,&model);
    } else if (g_pose_lab) {
        Mtx_Translate(&sonic_mv,0.0f,-g_pose_center_y,-g_pose_distance,true);
        Mtx_RotateY(&sonic_mv,g_pose_yaw,true);
    } else {
        Mtx_Translate(&sonic_mv,-1.2f,-0.3f,-18.0f,true);
        Mtx_RotateY(&sonic_mv,wrap_angle(g_player.yaw-g_camera.yaw),true);
    }
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&sonic_mv);
    switch(g_cull_mode) {
        default: case 0: C3D_CullFace(GPU_CULL_NONE); break;
        case 1: C3D_CullFace(GPU_CULL_BACK_CCW); break;
        case 2: C3D_CullFace(GPU_CULL_FRONT_CCW); break;
    }
    if(!sa3ds_sonic_gpu_draw(&g_sonic_gpu,root,g_world,g_node_count,&g_stats))return 0;
    if(g_emerald_coast){
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&g_ec_view);
        C3D_CullFace(GPU_CULL_NONE);
        return sa3ds_ec_gpu_draw(&g_ec_gpu,1);
    }
    return 1;
}

static void cleanup(void) {
    trace_stop();
    sa3ds_ec_gpu_destroy(&g_ec_gpu);
    sa3ds_ec_collision_destroy(&g_ec_collision);
    sa3ds_lab_gpu_destroy(&g_lab_gpu);
    sa3ds_sonic_gpu_destroy(&g_sonic_gpu);
    free(g_world); free(g_transition_local); free(g_local);
    g_world=NULL; g_transition_local=NULL; g_local=NULL;
    sa3ds_free_file(g_sa3t_bytes); sa3ds_free_file(g_sa3c_bytes);
    sa3ds_free_file(g_ec_bytes); g_ec_bytes=NULL; g_ec_size=0;
    g_sa3t_bytes=g_sa3c_bytes=NULL;
    if (g_dvlb) { shaderProgramFree(&g_program); DVLB_Free(g_dvlb); g_dvlb=NULL; }
}

int main(void) {
    gfxInitDefault();
    /* Stable console buffer: independent of the Citro3D TOP render target. */
    gfxSetDoubleBuffering(GFX_BOTTOM,false);
    consoleInit(GFX_BOTTOM,&g_boot_console);
    boot_log("BEGIN","Pass10A Pose Lab / CPUUpload2 base launched");
    int mode=boot_menu();
    if (!mode) { gfxExit(); return 0; }

    if (mode==1) { /* GPU-only: no Sonic assets, shaders or model draw. */
        boot_checkpoint("GPU-only: C3D_Init");
        if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
            fail_screen("GPU-only C3D_Init failed"); gfxExit(); return 1;
        }
        boot_checkpoint("GPU-only: RenderTargetCreate");
        g_target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
        if (!g_target) {
            fail_screen("GPU-only render target failed"); C3D_Fini(); gfxExit(); return 1;
        }
        C3D_RenderTargetSetOutput(g_target,GFX_TOP,GFX_LEFT,DISPLAY_TRANSFER_FLAGS);
        boot_checkpoint("GPU-only: draw solid blue top");
        /* Citro3D submits the TOP target. The bottom text is a diagnostic. */
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(g_target,C3D_CLEAR_ALL,0x3060B0FFu,0);
        C3D_FrameDrawOn(g_target);
        C3D_FrameEnd(0);
        boot_note("GPU-only frame submission completed");
        boot_result("GPU-ONLY: frame submitted", "Top should be BLUE.\nIf black, report black top.");
        C3D_Fini(); gfxExit(); return 0;
    }

    boot_checkpoint("romfsInit");
    if (R_FAILED(romfsInit())) { fail_screen("romfsInit failed"); gfxExit(); return 1; }
    boot_note("romfsInit passed");
    if (mode==2) { /* Assets with NO Citro3D; no graphics-memory allocations. */
        if (!load_owner_packages()) {
            fail_screen("SA3C/SA3T asset validation failed");
            cleanup(); romfsExit(); gfxExit(); return 1;
        }
        boot_result("ASSET PREFLIGHT: PASS", "Sonic pair parsed and checked.\nNo Citro3D attempted.");
        cleanup(); romfsExit(); gfxExit(); return 0;
    }

    boot_checkpoint("FULL: C3D_Init");
    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) { fail_screen("C3D_Init failed"); romfsExit(); gfxExit(); return 1; }
    boot_checkpoint("FULL: RenderTargetCreate");
    g_target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    if (!g_target) { fail_screen("render target allocation failed"); C3D_Fini(); romfsExit(); gfxExit(); return 1; }
    C3D_RenderTargetSetOutput(g_target,GFX_TOP,GFX_LEFT,DISPLAY_TRANSFER_FLAGS);
    g_emerald_coast=(mode==6);
    FILE *stage_only=fopen("sdmc:/3ds/SA3DS/emerald_coast_stage_only.flag","rb");
    g_ec_combined=g_emerald_coast && stage_only==NULL;
    FILE *replay=fopen("sdmc:/3ds/SA3DS/emerald_coast_replay.flag","rb");
    g_ec_replay=g_ec_combined && replay!=NULL;
    g_ec_replay_pause=replay && fgetc(replay)=='P';
    if(replay)fclose(replay);
    if(stage_only)fclose(stage_only);
    if (g_emerald_coast) {
        boot_checkpoint("FULL: loading Emerald Coast visual package");
        if (!load_emerald_coast_package()) {
            fail_screen("Emerald Coast act-0 package missing or invalid");
            cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 1;
        }
        if (g_ec_combined && !load_owner_packages()) {
            fail_screen("Combined probe: Sonic owner pair invalid");
            cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 1;
        }
    } else {
        boot_checkpoint("FULL: loading & validating Sonic");
        if (!load_owner_packages()) {
            fail_screen("owner SA3C/SA3T missing or invalid (RomFS/SD)"); cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 1;
        }
    }
    if (mode==4) {
        boot_checkpoint("Y-only: CPU texture uploads ENTER");
        if (!sa3ds_sonic_gpu_init_traced(&g_sonic_gpu,&g_character,&g_texture_archive,
                                        log_texture_stage,NULL)) {
            fail_screen("Y: CPU texture uploads FAILED");
            cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 1;
        }
        boot_checkpoint("Y-only: CPU texture uploads SUCCESS");
        boot_result("CPU TEXTURES: PASS", "No shaders, mesh draw or animation render.\nNext relaunch and press A.");
        cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 0;
    }
    if (!init_scene()) {
        fail_screen("Citro3D White Zone init failed"); cleanup(); C3D_Fini(); romfsExit(); gfxExit(); return 1;
    }
    g_pose_lab=(mode==5);
    boot_checkpoint(g_pose_lab?"POSE LAB: reset and freeze Stand frame 0":
        (g_emerald_coast?"EMERALD COAST: reset preview":"FULL: reset player/animation"));
    if (g_emerald_coast && !g_ec_combined) {
        sa3ds_wz_tick_reset(&g_tick_clock);
        g_last_frame_ms=osGetTime();
    } else reset_selected_zone();
    if(g_pose_lab) {
        mkdir("sdmc:/3ds/SA3DS",0777);
        FILE *f=fopen("sdmc:/3ds/SA3DS/pass10_pose_metrics.csv","wb");
        if(f){fputs("action,frame,bind,yaw_radians,distance,root,vertices,triangles,min_x,min_y,min_z,max_x,max_y,max_z\n",f);fclose(f);}
    }
    boot_checkpoint(g_pose_lab?"POSE LAB: submit Stand frame 0":
        (g_emerald_coast?"EMERALD COAST: submit first authentic frame":"FULL: submit first game frame"));
    if(g_pose_lab) pose_lab_help(); else if(g_emerald_coast) emerald_coast_help(); else print_help();

    while (aptMainLoop()) {
        hidScanInput();
        u32 down=hidKeysDown(), held=hidKeysHeld(), up=hidKeysUp();
        if (down&KEY_START) break;
        if (g_pose_lab) pose_lab_keys(down);
        int modifier=!g_pose_lab && ((held&KEY_SELECT)!=0);
        if (!g_pose_lab && !g_emerald_coast && (down&KEY_SELECT) && !(held&(KEY_L|KEY_R|KEY_X|KEY_Y|KEY_DLEFT))) g_dev_overlay=!g_dev_overlay;
        if (!g_emerald_coast && modifier && (down&KEY_L)) cycle_zone(-1);
        if (!g_emerald_coast && modifier && (down&KEY_R)) cycle_zone(1);
        if (modifier && (down&KEY_Y)) reset_selected_zone();
        if (modifier && (down&KEY_A)) {
            g_paused=!g_paused;
            sa3ds_wz_tick_reset(&g_tick_clock);
            g_last_frame_ms=osGetTime();
        }
        if (modifier && (down&KEY_X)) trace_toggle();
        if (modifier && (down&KEY_DLEFT)) sa3ds_parity_camera_toggle(&g_camera);
        if (!modifier && (down&KEY_Y)) sa3ds_parity_camera_recenter(&g_camera,g_player.yaw);

        SA1Input in=sa3ds_read_logical_input(down,held,up);
        if (modifier) {
            in.jump_pressed=in.jump_held=in.jump_released=false;
            in.action_pressed=in.action_held=in.action_released=false;
            in.spindash_shortcut_pressed=in.spindash_shortcut_held=in.spindash_shortcut_released=false;
            in.lightspeed_shortcut_pressed=in.lightspeed_shortcut_held=in.lightspeed_shortcut_released=false;
        }
        /* Render cadence is NOT simulation cadence. Keep a fixed 60 Hz
           simulation and deliver button edges once, even if a render frame
           produces zero or several physics steps. */
        uint64_t now_ms=osGetTime();
        uint32_t elapsed_ms=(now_ms>=g_last_frame_ms &&
            now_ms-g_last_frame_ms<=UINT32_MAX) ?
            (uint32_t)(now_ms-g_last_frame_ms) : SA3DS_WZ_MAX_ELAPSED_MS;
        g_last_frame_ms=now_ms;
        unsigned steps=(g_pose_lab || (g_emerald_coast&&!g_ec_combined) || g_paused) ? 0u : sa3ds_wz_tick_begin(&g_tick_clock,elapsed_ms,&in);
        for (unsigned step=0;step<steps;step++) {
            SA1Input tick_input=sa3ds_wz_tick_input(&g_tick_clock,step);
            if(g_ec_replay) {
                if(g_ec_replay_tick==0){reset_selected_zone();trace_toggle();}
                tick_input=sa3ds_ec_replay_input(g_ec_replay_tick);
                /* Fixed camera for equivalent scripted directional inputs. */
                g_camera.yaw=0;
            }
            SA1SteamFlatDebug physics_trace={0};
            if (!modifier && (held&KEY_L)) sa3ds_parity_camera_manual_yaw(&g_camera,-0.035f);
            if (!modifier && (held&KEY_R)) sa3ds_parity_camera_manual_yaw(&g_camera,0.035f);
            SA1Vec3 previous=g_player.pos;
            uint64_t previous_tick=g_player.sim_frame;
            if(g_emerald_coast)g_ec_collision.reference_y=g_player.pos.y;
            sa1_physics_step_steam_sonic_flat(&g_player,&tick_input,g_camera.yaw,
                g_emerald_coast?&g_ec_collision.world:sa3ds_parity_lab_world(),
                &SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&physics_trace);
            if(g_emerald_coast && !(previous_tick && g_player.sim_frame==1))
                sa3ds_ec_collision_resolve_walls(&g_ec_collision,previous,&g_player);
            SA1PlayerState physics_done=g_player;
            g_observation=sa3ds_sonic_animation_observe(&g_anim_sync,&g_player,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
            g_selection=sa3ds_sonic_animation_select(&g_anim_runtime,&g_player,&g_observation);
            if (memcmp(&physics_done,&g_player,sizeof(g_player))!=0) {
                fail_screen("animation mutated physics state");
                trace_stop();
                cleanup(); C3D_Fini(); romfsExit(); gfxExit();
                return 1;
            }
            if (g_trace_file) {
                const SA3DSParityZone *trace_zone=sa3ds_parity_lab_zone_by_id(g_selected_zone);
                if (sa3ds_parity_trace_write_frame(g_trace_file,g_selected_zone,
                        g_emerald_coast?"EMERALD_COAST_ACT0_PROVISIONAL":(trace_zone?trace_zone->name:"UNKNOWN"),&g_player,&tick_input,
                        g_camera.yaw,&physics_trace,&g_selection)!=0) trace_stop();
                else { g_trace_frames++; if ((g_trace_frames&63u)==0) fflush(g_trace_file); }
            }
            sa3ds_parity_camera_tick(&g_camera,g_player.yaw,
                (!modifier && (held&(KEY_L|KEY_R))) ? 1 : 0);
            if(g_ec_replay) {
                if(g_ec_replay_tick%30==0 || tick_input.jump_pressed) {
                    char line[180];snprintf(line,sizeof(line),
                        "tick=%u in=%.2f,%.2f p=%.3f,%.3f,%.3f v=%.3f,%.3f,%.3f g=%d walls=%lu",
                        g_ec_replay_tick,tick_input.move_x,tick_input.move_z,
                        g_player.pos.x,g_player.pos.y,g_player.pos.z,g_player.vel.x,g_player.vel.y,g_player.vel.z,
                        g_player.grounded,(unsigned long)g_ec_collision.wall_hits);boot_log("REPLAY",line);
                }
                if(++g_ec_replay_tick==600){trace_stop();g_ec_replay=0;boot_log("REPLAY","finished; live HID controls resumed");}
                if(g_ec_replay_pause && g_ec_replay_tick==270){
                    g_paused=1;boot_log("REPLAY","paused airborne for inspection; SELECT+A resumes");
                }
            }
        }

        uint64_t t0=osGetTime();
        int probe_log=g_rendered_frames<3u || g_rendered_frames==60u || g_rendered_frames==180u;
        char frame_line[160];
        if(probe_log){snprintf(frame_line,sizeof(frame_line),"frame %u begin stage=%d sonic=%d",g_rendered_frames,g_emerald_coast,!g_emerald_coast||g_ec_combined);boot_log("FRAME",frame_line);}
        if (g_boottrace_first_frame) boot_log("ENTER","First C3D_FrameBegin");
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        if(probe_log)boot_log("FRAME","begin returned");
        C3D_RenderTargetClear(g_target,C3D_CLEAR_ALL,
            g_pose_lab?0x9EBAD2FFu:CLEAR_COLOR,0);
        C3D_FrameDrawOn(g_target);
        if (g_boottrace_first_frame) boot_log("ENTER","First sample_pose (Sonic + lab draw)");
        if (!sample_pose()) {
            C3D_FrameEnd(0); fail_screen("White Zone pose/draw failed"); break;
        }
        if (g_boottrace_first_frame) boot_log("ENTER","First C3D_FrameEnd");
        if(probe_log){snprintf(frame_line,sizeof(frame_line),"draw returned stage=%lu sonic=%lu vertices=%lu",(unsigned long)g_ec_gpu.draw_count,(unsigned long)g_sonic_gpu.frame_draws,(unsigned long)g_sonic_gpu.frame_vertices);boot_log("FRAME",frame_line);}
        C3D_FrameEnd(0);
        if(probe_log)boot_log("FRAME","end returned");
        g_rendered_frames++;
        if (g_boottrace_first_frame) {
            boot_log("OK","First frame rendered/submitted");
            g_boottrace_first_frame=0;
        }
        g_last_submit_ms=osGetTime()-t0;
        if(g_emerald_coast&&probe_log){snprintf(frame_line,sizeof(frame_line),
            "frame=%u tick=%llu pos=%.3f,%.3f,%.3f grounded=%d wall=%lu linear=%lu",
            g_rendered_frames,(unsigned long long)g_player.sim_frame,g_player.pos.x,g_player.pos.y,g_player.pos.z,
            g_player.grounded,(unsigned long)g_ec_collision.wall_hits,(unsigned long)linearSpaceFree());boot_log("PLAY",frame_line);}
        if(g_pose_lab) pose_lab_status(); else if(g_emerald_coast) emerald_coast_status(); else print_status();
        gfxFlushBuffers();
        gfxScreenSwapBuffers(GFX_BOTTOM,false);
    }

    cleanup();
    C3D_Fini(); romfsExit(); gfxExit();
    return 0;
}
