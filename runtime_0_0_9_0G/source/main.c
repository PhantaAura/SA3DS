#include "player_view.h"
#include "wca_replay.h"
#include "emerald_coast_route.h"
#include "emerald_coast_residency.h"
#include "ring_runtime.h"
#include "ring_groups.h"
#include "spring_player.h"
#include "dash_panel.h"
#include "panel_visual.h"
#include "spring_visual.h"
#include "ring_replay.h"
#include "saru_actor.h"
#include "saru_pose.h"
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
#include "emerald_coast_background.h"
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
static uint8_t *g_ec_collision_bytes;
static size_t g_ec_collision_size;
static SA3DSECRoute g_ec_route;
static int g_ec_full_route=0;
static int g_ec_transition_failed=0;
static uint32_t g_ec_act_index=0;
static int g_ec_cycle_probe=0;
static struct {
    uint64_t start_ms,submit_ms,triangles,draws;
    uint32_t frames,ticks,max_submit_ms,min_linear;
    float max_cmd;
} g_ec_perf;
static int g_ec_failed_region_valid=0;
static float g_ec_failed_region_x=0,g_ec_failed_region_z=0;
static SA3DSECVisual g_ec_visual;
static uint8_t *g_background_bytes;
static size_t g_background_size;
static SA3DSECBackground g_background;
static SA3DSECGpu g_background_gpu;
static int g_background_ready;

static SA3DSECGpu g_ec_gpu;
static int g_ec_override_requested=-1;
static uint8_t *g_ring_visual_bytes,*g_ring_placement_bytes;
static size_t g_ring_visual_size,g_ring_placement_size;
static SA3DSECVisual g_ring_visual;
static SA3DSECGpu g_ring_gpu;
static SA3DSRings g_rings;
typedef struct {SA3DSSpringActor actor;SA3DSSpringTrig trig;uint8_t loaded,pending_hit,draw,unsupported;int32_t next,previous;} SA3DSNativeSpring;
static SA3DSMovementObjects g_movement;
static uint8_t *g_movement_bytes,*g_spring_visual_bytes;
static size_t g_movement_size,g_spring_visual_size;
static SA3DSNativeSpring *g_spring_states;
static SA3DSSpringVisual g_spring_visual;
static SA3DSECGpu g_spring_gpu[6];
static SA3DSSpringPendingCommand g_spring_pending;
typedef struct {SA3DSDashPanel actor;SA3DSSpringTrig trig;uint8_t loaded,pending_hit,draw,phase,frame;int32_t next,previous;} SA3DSNativePanel;
static SA3DSNativePanel *g_panel_states;
static SA3DSPanelVisual g_panel_visual;
static SA3DSECGpu g_panel_gpu[3];
static uint8_t *g_panel_visual_bytes;
static size_t g_panel_visual_size;
typedef struct {SA3DSSaruActor actor;uint8_t loaded,display;int32_t next,previous;} SA3DSNativeSaru;
static SA3DSSaruDefinitions g_saru_definitions;
static SA3DSNativeSaru *g_saru_states;
static uint8_t *g_saru_definition_bytes,*g_saru_character_bytes,*g_saru_texture_bytes;
static size_t g_saru_definition_size,g_saru_character_size,g_saru_texture_size;
static SA3CCharacter g_saru_character;
static SA3TArchive g_saru_textures;
static SA3DSSonicGpu g_saru_gpu;
static SA3CTransform g_saru_local[38];
static SA3CMatrix g_saru_world[38];
static uint16_t g_saru_frames[6];
static uint32_t g_saru_rng=1,g_saru_projectiles,g_saru_sounds,g_saru_dust;
static SA3DSCCLQueryCursor g_saru_damage_cursor;
static int g_saru_ready;
static int32_t g_saru_head=-1;
static unsigned g_saru_frame_actors;
static int g_saru_probe;
static unsigned g_saru_probe_ticks,g_saru_probe_draws,g_saru_probe_actions,g_saru_probe_max_actors;
static void reset_saru(void);
static int g_panels_ready;
static int g_panel_probe;
static unsigned g_panel_probe_ticks,g_panel_probe_boosts,g_panel_probe_mask,g_panel_probe_draws,g_panel_probe_sim_mask;
static uint32_t g_panel_probe_row;
static int32_t g_panel_head=-1;
static unsigned g_movement_ccl_queued;
static void reset_panels(void);
static int g_springs_ready;
static int32_t g_spring_head=-1;
static int g_air_attack_probe;
static unsigned g_air_attack_probe_ticks,g_air_attack_probe_attack_ticks,g_air_attack_probe_entry,g_air_attack_probe_exit;
static uint8_t g_air_attack_probe_previous_action;
static int g_spring_probe;
static unsigned previous_spring_animation=0xffffu;
static unsigned g_spring_probe_ticks,g_spring_probe_launches,g_spring_probe_rise,g_spring_probe_fall,g_spring_probe_unsupported;
static void reset_springs(void);

static int g_rings_ready=0;
static int g_ring_probe=0,g_ring_probe_groups=0;
static unsigned g_ring_probe_tick=0;
static unsigned g_ring_frame_draws=0;
static int g_emerald_coast;
static int g_ec_combined;
static int g_ec_replay;
static unsigned g_ec_replay_tick;
static int g_wca_replay=0;
static int g_wca_perf_active=0,g_wca_perf_finished=0;
static uint64_t g_wca_perf_start_ms=0,g_wca_perf_submit_ms=0;
static unsigned g_wca_perf_frames=0,g_wca_perf_max_submit_ms=0;
static uint32_t g_wca_perf_min_linear=UINT32_MAX;
static float g_wca_perf_max_cmd_usage=0;
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
        const SA3DSECStart *start=g_ec_full_route?sa3ds_ec_route_start(&g_ec_route,g_ec_act_index):NULL;
        if(start)sa3ds_ec_residency_start(&g_ec_collision,start,&g_player);
        else sa1_player_reset(&g_player,&g_ec_collision.world);
        g_ec_collision.reference_y=g_player.pos.y;
        if(g_rings_ready)sa3ds_rings_reset_act(&g_rings,g_ec_act_index);
        reset_springs();
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
    if(g_ec_full_route){
        printf("Authentic PC2004 Sonic route.\n");
        printf("Full terrain; route objects partial.\n");
    }else{
        printf("Authentic PC2004 opening subset.\n");
        printf("500-unit opening residency subset.\n");
    }
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
    if(g_ec_full_route)printf("\x1b[17;1HPackage:%5luKB Act:%lu FULL       ",
        (unsigned long)(g_ec_size/1024u),(unsigned long)g_ec_act_index);
    else printf("\x1b[17;1HPackage:%5luKB Radius:%5.0f      ",
        (unsigned long)(g_ec_size/1024u),g_ec_gpu.subset_radius);
    printf("\x1b[18;1HLinear free:%5luKB Frame:%3llums ",
        (unsigned long)(linearSpaceFree()/1024u),(unsigned long long)g_last_submit_ms);
    printf("\x1b[19;1HP:%6.1f %6.1f %6.1f %s    ",g_player.pos.x,g_player.pos.y,g_player.pos.z,g_player.grounded?"GND":"AIR");
    printf("\x1b[20;1HV:%5.2f %5.2f %5.2f S:%ld    ",g_player.vel.x,g_player.vel.y,g_player.vel.z,(long)g_player.surface_id);
    printf("\x1b[21;1HN:%4.2f %4.2f %4.2f Wall:%lu  ",g_player.ground_normal.x,g_player.ground_normal.y,g_player.ground_normal.z,(unsigned long)g_ec_collision.wall_hits);
    printf("\x1b[22;1HCollision:%lu Q:%lu Tests:%lu  ",(unsigned long)g_ec_collision.triangle_count,(unsigned long)g_ec_collision.queries,(unsigned long)g_ec_collision.triangle_tests);
    printf("\x1b[23;1HSonic D:%lu Anim:%u Fr:%.1f    ",(unsigned long)g_sonic_gpu.frame_draws,g_selection.binding.runtime_id,g_selection.sample_frame);
    printf("\x1b[24;1HTick:%llu GPU frames:%u       ",(unsigned long long)g_player.sim_frame,g_rendered_frames);
    if(g_rings_ready)printf("\x1b[25;1HRings:%5d Ring draws:%3u     ",g_rings.rings,g_ring_frame_draws);
    else printf("\x1b[25;1HPartial collision/follow camera ");
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

static void ec_world_bounds(SA3DSECCollision *collision,const SA3DSECVisual *visual){
    float min_x=1e30f,max_x=-1e30f,min_z=1e30f,max_z=-1e30f;
    for(uint32_t i=0;i<visual->instance_count;i++){
        SA3DSECInstance in;if(!sa3ds_ec_instance(visual,i,&in))continue;
        min_x=fminf(min_x,in.center[0]-in.radius);max_x=fmaxf(max_x,in.center[0]+in.radius);
        min_z=fminf(min_z,in.center[2]-in.radius);max_z=fmaxf(max_z,in.center[2]+in.radius);
    }
    collision->world.reset_min_x=min_x;collision->world.reset_max_x=max_x;
    collision->world.reset_min_z=min_z;collision->world.reset_max_z=max_z;
    /* The inherited Y fallback remains provisional. Original finite death
     * meshes are a separate gameplay gate; no retail scalar plane is claimed. */
}

static int load_emerald_coast_package(void) {
    uint8_t *route_bytes=NULL;size_t route_size=0;
    if(g_ec_combined&&!g_ec_replay&&sa3ds_read_file(
            "sdmc:/3ds/SA3DS/assets/emerald_coast_route.sa3dspak",&route_bytes,&route_size)){
        g_ec_full_route=sa3ds_ec_route_open(&g_ec_route,route_bytes,route_size);
        if(g_ec_full_route && (g_ec_route.start_count!=2||g_ec_route.edge_count!=1||
                !sa3ds_ec_route_start(&g_ec_route,0)||!sa3ds_ec_route_start(&g_ec_route,1)||
                g_ec_route.edges[0].from_act!=0||g_ec_route.edges[0].to_act!=1))g_ec_full_route=0;
        sa3ds_free_file(route_bytes);
        if(!g_ec_full_route)boot_note("Owner route metadata invalid; retaining opening regression profile");
    }
    g_ec_act_index=0;
    if(g_ec_full_route){
        FILE *cycle=fopen("sdmc:/3ds/SA3DS/emerald_coast_act_cycle.flag","rb");
        g_ec_cycle_probe=cycle!=NULL;if(cycle)fclose(cycle);
        /* Explicit owner geometry-validation hook, not natural progression. */
        FILE *probe=fopen("sdmc:/3ds/SA3DS/emerald_coast_act.flag","rb");
        if(probe){int act=fgetc(probe);fclose(probe);
            if(act=='1'&&sa3ds_ec_route_start(&g_ec_route,1))g_ec_act_index=1;
        }
    }
    const char *path=g_ec_act_index==1?
        "sdmc:/3ds/SA3DS/assets/emerald_coast_act1.sa3dspak":
        "sdmc:/3ds/SA3DS/assets/emerald_coast_act0.sa3dspak";
    boot_checkpoint("Loading Emerald Coast owner act package from SD");
    if(!sa3ds_read_file(path,&g_ec_bytes,&g_ec_size))return 0;
    boot_checkpoint("Validating Emerald Coast package and references");
    SA3DSECVisualResult result=sa3ds_ec_visual_open(&g_ec_visual,g_ec_bytes,g_ec_size);
    if(result!=SA3DS_EC_VISUAL_OK){
        printf("\nEmerald Coast visual: %s\n",sa3ds_ec_visual_result_name(result));return 0;
    }
    boot_note("Emerald Coast owner act package preflight passed");
    if(g_ec_combined){
        boot_checkpoint("Loading and indexing owner Emerald Coast collision");
        if(!sa3ds_read_file("sdmc:/3ds/SA3DS/assets/emerald_coast_collision.sa3dspak",
                &g_ec_collision_bytes,&g_ec_collision_size))return 0;
        const SA3DSECStart *start=g_ec_full_route?sa3ds_ec_route_start(&g_ec_route,g_ec_act_index):NULL;
        int ok=start?sa3ds_ec_collision_open_region(&g_ec_collision,g_ec_collision_bytes,
            g_ec_collision_size,g_ec_act_index,start->task_origin.x,start->task_origin.z):
            sa3ds_ec_collision_open(&g_ec_collision,g_ec_collision_bytes,g_ec_collision_size);
        if(!ok)return 0;
        if(start){
            ec_world_bounds(&g_ec_collision,&g_ec_visual);
            sa3ds_ec_residency_start(&g_ec_collision,start,&g_player);
            boot_note("Source task start selected; full one-act geometry residency");
        }else{
            sa3ds_free_file(g_ec_collision_bytes);g_ec_collision_bytes=NULL;g_ec_collision_size=0;
            if(!sa3ds_ec_collision_spawn(&g_ec_collision,SA3DS_EC_SPAWN_X,SA3DS_EC_SPAWN_Z,0.0f))return 0;
            boot_note("Provisional spawn: imported support at x=0 z=-50; not retail spawn");
        }
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

/* Optional single-texture replacement. All GPU mutation occurs outside an
 * active frame, after pending submissions retire. Missing/invalid means original. */
static void ec_apply_texture_override(void){
    if(g_ec_override_requested<0){
        FILE *enabled=fopen("sdmc:/3ds/SA3DS/mods/emerald_coast.enabled","rb");
        g_ec_override_requested=enabled&&fgetc(enabled)=='1';
        if(enabled)fclose(enabled);
    }
    if(!g_ec_override_requested){
        if(g_ec_gpu.override_enabled){C3D_FrameSync();sa3ds_ec_gpu_clear_texture_override(&g_ec_gpu);}
        boot_log("MOD","Original Emerald Coast textures selected");return;
    }
    C3D_FrameSync();
    sa3ds_ec_gpu_clear_texture_override(&g_ec_gpu);
    FILE *file=fopen("sdmc:/3ds/SA3DS/mods/emerald_coast.sa3mod","rb");
    if(!file){boot_log("MOD","Replacement missing; original textures retained");return;}
    long length=0;
    if(fseek(file,0,SEEK_END)||((length=ftell(file))<=0)||
       (unsigned long)length>SA3DS_TEXTURE_OVERRIDE_MAX_BYTES||fseek(file,0,SEEK_SET)){
        fclose(file);boot_log("MOD","Replacement file size invalid; original textures retained");return;
    }
    uint8_t *bytes=malloc((size_t)length);
    if(!bytes){fclose(file);boot_log("MOD","Replacement allocation failed; original textures retained");return;}
    int complete=fread(bytes,1,(size_t)length,file)==(size_t)length;
    fclose(file);
    SA3DSTextureOverride replacement;
    SA3DSTextureOverrideResult result=complete?sa3ds_texture_override_open(
        &replacement,bytes,(size_t)length,g_ec_act_index,&g_ec_visual.textures):SA3DS_TEXTURE_OVERRIDE_HEADER;
    if(result==SA3DS_TEXTURE_OVERRIDE_OK&&sa3ds_ec_gpu_set_texture_override(
            &g_ec_gpu,&replacement,log_texture_stage,NULL)){
        char note[128];snprintf(note,sizeof(note),"External texture active: act=%lu texture=%lu bytes=%lu",
            (unsigned long)g_ec_act_index,(unsigned long)replacement.texture_id,
            (unsigned long)g_ec_gpu.override_texture_bytes);boot_log("MOD",note);
    }else{
        char note[144];snprintf(note,sizeof(note),"Replacement rejected (%s); original textures retained",
            result==SA3DS_TEXTURE_OVERRIDE_OK?"upload-failed":sa3ds_texture_override_result_name(result));boot_log("MOD",note);
    }
    free(bytes);
}

static void ec_toggle_texture_override(void){
    g_ec_override_requested=!g_ec_override_requested;
    mkdir("sdmc:/3ds/SA3DS/mods",0777);
    const char *path="sdmc:/3ds/SA3DS/mods/emerald_coast.enabled";
    if(g_ec_override_requested){FILE *file=fopen(path,"wb");if(file){fputc('1',file);fclose(file);}}
    else remove(path);
    ec_apply_texture_override();
    /* Optional asset reload pauses this UI operation rather than creating a
     * catch-up burst of gameplay ticks while SD/GPU resources were replaced. */
    sa3ds_wz_tick_reset(&g_tick_clock);
    g_last_frame_ms=osGetTime();
}

static int ec_change_act(uint32_t act){
    uint64_t load_start=osGetTime();
    const SA3DSECStart *start=sa3ds_ec_route_start(&g_ec_route,act);
    if(!start||act>1||!g_ec_collision_bytes)return 0;
    char path[128];snprintf(path,sizeof(path),
        "sdmc:/3ds/SA3DS/assets/emerald_coast_act%lu.sa3dspak",(unsigned long)act);
    uint8_t *bytes=NULL;size_t size=0;SA3DSECVisual visual;
    SA3DSECCollision collision={0};SA3DSECGpu gpu={0};
    if(!sa3ds_read_file(path,&bytes,&size))return 0;
    if(sa3ds_ec_visual_open(&visual,bytes,size)!=SA3DS_EC_VISUAL_OK||
       !sa3ds_ec_collision_open_region(&collision,g_ec_collision_bytes,g_ec_collision_size,
            act,start->task_origin.x,start->task_origin.z))goto fail;
    ec_world_bounds(&collision,&visual);
    C3D_FrameSync(); /* Retire submissions before replacing GPU-owned resources. */
    if(!sa3ds_ec_gpu_init_full(&gpu,&visual,log_texture_stage,NULL))goto fail;
    uint32_t overlap_linear=linearSpaceFree();
    sa3ds_ec_gpu_destroy(&g_ec_gpu);sa3ds_ec_collision_destroy(&g_ec_collision);
    sa3ds_free_file(g_ec_bytes);
    g_ec_bytes=bytes;g_ec_size=size;g_ec_visual=visual;g_ec_gpu=gpu;
    g_ec_gpu.visual=&g_ec_visual;
    g_ec_collision=collision;g_ec_collision.world.context=&g_ec_collision;
    g_ec_act_index=act;g_ec_failed_region_valid=0;
    ec_apply_texture_override();
    memset(&g_ec_perf,0,sizeof(g_ec_perf));
    reset_selected_zone();emerald_coast_help();
    char note[160];snprintf(note,sizeof(note),
        "act=%lu source-start=%.3f,%.3f,%.3f tris=%lu linear=%lu overlapLinear=%lu loadMs=%llu",
        (unsigned long)act,start->task_origin.x,start->task_origin.y,start->task_origin.z,
        (unsigned long)g_ec_gpu.triangle_count,(unsigned long)linearSpaceFree(),
        (unsigned long)overlap_linear,(unsigned long long)(osGetTime()-load_start));boot_log("ACT",note);
    return 1;
fail:
    sa3ds_ec_gpu_destroy(&gpu);sa3ds_ec_collision_destroy(&collision);sa3ds_free_file(bytes);return 0;
}

static void ec_refresh_collision_region(void){
    if(!g_ec_full_route||!sa3ds_ec_residency_needs_recenter(&g_ec_collision,g_player.pos))return;
    float x=floorf(g_player.pos.x/256.0f+0.5f)*256.0f;
    float z=floorf(g_player.pos.z/256.0f+0.5f)*256.0f;
    if(g_ec_failed_region_valid&&x==g_ec_failed_region_x&&z==g_ec_failed_region_z)return;
    if(sa3ds_ec_residency_recenter(&g_ec_collision,g_ec_collision_bytes,g_ec_collision_size,x,z,&g_player)){
        g_ec_failed_region_valid=0;
        char line[128];snprintf(line,sizeof(line),"act=%lu center=%.0f,%.0f triangles=%lu",
            (unsigned long)g_ec_act_index,x,z,(unsigned long)g_ec_collision.triangle_count);boot_log("REGION",line);
    }else{
        g_ec_failed_region_valid=1;g_ec_failed_region_x=x;g_ec_failed_region_z=z;
        boot_log("REGION","No valid replacement support region; retaining previous geometry without invented support");
    }
}

static void load_rings(void){
    if(!g_ec_full_route)return;
    if(!sa3ds_read_file("sdmc:/3ds/SA3DS/assets/ring_visual.sa3dspak",
            &g_ring_visual_bytes,&g_ring_visual_size)||
       !sa3ds_read_file("sdmc:/3ds/SA3DS/assets/ring_placements.sa3dspak",
            &g_ring_placement_bytes,&g_ring_placement_size))goto fail;
    if(g_ring_visual_size>1024*1024||g_ring_placement_size>128*1024||
       sa3ds_ec_visual_open(&g_ring_visual,g_ring_visual_bytes,g_ring_visual_size)!=SA3DS_EC_VISUAL_OK||
       g_ring_visual.instance_count!=1||g_ring_visual.model_count!=1||
       g_ring_visual.index_count>32768||
       !sa3ds_rings_open(&g_rings,g_ring_placement_bytes,g_ring_placement_size)||
       !sa3ds_ec_gpu_init(&g_ring_gpu,&g_ring_visual,500.0f,log_texture_stage,NULL))goto fail;
    g_rings_ready=1;sa3ds_rings_reset_act(&g_rings,g_ec_act_index);
    FILE*probe=fopen("sdmc:/3ds/SA3DS/emerald_coast_ring_replay.flag","rb");
    if(probe){g_ring_probe_groups=fgetc(probe)=='G';fclose(probe);g_ring_probe=1;
        boot_log("VALIDATION","Opening direct-ring input-only replay; no teleports or forced collection");}
    char note[128];snprintf(note,sizeof(note),"ring actors=%lu direct=%lu groups=%lu clip=%u shared triangles=%lu textures=%lu",
        (unsigned long)g_rings.count,(unsigned long)g_rings.direct_count,(unsigned long)g_rings.group_count,g_rings.clip_level,(unsigned long)g_ring_gpu.triangle_count,
        (unsigned long)g_ring_gpu.resident_texture_count);boot_log("RINGS",note);return;
fail:
    sa3ds_ec_gpu_destroy(&g_ring_gpu);sa3ds_rings_destroy(&g_rings);
    sa3ds_free_file(g_ring_visual_bytes);sa3ds_free_file(g_ring_placement_bytes);
    g_ring_visual_bytes=g_ring_placement_bytes=NULL;
    g_ring_visual_size=g_ring_placement_size=0;
    boot_note("Ring assets unavailable/invalid; route objects remain incomplete");
}
static int draw_rings(void){
    g_ring_frame_draws=0;if(!g_rings_ready)return 1;
    for(uint32_t i=0;i<g_rings.count;i++){
        SA3DSRingState*state=&g_rings.states[i];SA3DSRingPlacement p;
        if(!sa3ds_rings_placement(&g_rings,i,&p)||p.act!=g_ec_act_index)continue;
        if(p.group>=0&&g_rings.groups[p.group].skip_children)continue;
        if(!sa3ds_rings_draw_normal(&g_rings,i))continue;
        float center[3]={p.position.x,p.position.y,p.position.z};
        /* Original Ring_Display IsVisible uses a20-unit culling sphere. */
        if(!sa3ds_ec_gpu_sphere_visible(&g_ec_gpu,center,20.0f))continue;
        SA3CMatrix actor;C3D_Mtx model,mv;
        sa3c_matrix_build_object(&actor,(SA3CVec3){p.position.x,p.position.y,p.position.z},
            (SA3CVec3){0,(float)state->yaw*(2.0f*PI/65536.0f),0},(SA3CVec3){1,1,1},0);
        for(unsigned row=0;row<4;row++)model.r[row]=FVec4_New(
            actor.m[row],actor.m[4+row],actor.m[8+row],actor.m[12+row]);
        Mtx_Multiply(&mv,&g_ec_view,&model);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&mv);
        if(!sa3ds_ec_gpu_draw(&g_ring_gpu,0))return 0;
        g_ring_frame_draws++;
    }
    return 1;
}

static void load_background(void) {
    if(!sa3ds_read_file("sdmc:/3ds/SA3DS/assets/emerald_coast_background.sa3dspak",
            &g_background_bytes,&g_background_size))goto fail;
    if(!sa3ds_ec_background_open(&g_background,g_background_bytes,g_background_size)||
       !sa3ds_ec_gpu_init_full(&g_background_gpu,&g_background.visual,
                              log_texture_stage,NULL))goto fail;
    g_background_gpu.background_pass=1;
    g_background_ready=1;
    boot_log("BACKGROUND","Original camera-XZ sky/horizon loaded; source act/clip scales");
    return;
fail:
    sa3ds_ec_gpu_destroy(&g_background_gpu);
    sa3ds_free_file(g_background_bytes);g_background_bytes=NULL;
    boot_log("BACKGROUND","Owner background unavailable; environment remains incomplete");
}

static int draw_background(const C3D_Mtx *view,const float eye[3]) {
    if(!g_background_ready)return 1;
    SA3DSECBackgroundConfig config;SA3CMatrix actor;
    C3D_Mtx projection,model,mv;
    if(!sa3ds_ec_background_select(&g_background,g_ec_act_index,1,&config)||
       !sa3ds_ec_background_transform(&g_background,g_ec_act_index,1,eye,&actor))return 0;
    Mtx_PerspTilt(&projection,C3D_AngleFromDegrees(55.0f),C3D_AspectRatioTop,
                  config.background_near,config.background_far,false);
    for(unsigned row=0;row<4;row++)model.r[row]=FVec4_New(
        actor.m[row],actor.m[4+row],actor.m[8+row],actor.m[12+row]);
    Mtx_Multiply(&mv,view,&model);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_projection,&projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&mv);
    int ok=sa3ds_ec_gpu_draw(&g_background_gpu,0)&&
           sa3ds_ec_gpu_draw(&g_background_gpu,1);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_projection,&g_projection);
    return ok;
}

static double spring_source_sin(void *context,uint16_t angle){
    return sa3ds_ring_group_sin((const SA3DSRings*)context,angle);
}
static void reset_springs(void){
    g_spring_head=-1;reset_panels();reset_saru();
    if(g_spring_states)memset(g_spring_states,0,g_movement.count*sizeof(*g_spring_states));
    memset(&g_spring_pending,0,sizeof(g_spring_pending));
}
static void load_springs(void){
    if(!g_ec_combined||!g_rings_ready||!g_rings.quarter_lut)goto fail;
    if(!sa3ds_read_file("sdmc:/3ds/SA3DS/assets/ec_movement_objects.sa3dspak",&g_movement_bytes,&g_movement_size)||
       !sa3ds_movement_objects_open(&g_movement,g_movement_bytes,g_movement_size)||
       !sa3ds_read_file("sdmc:/3ds/SA3DS/assets/spring_visual.sa3dspak",&g_spring_visual_bytes,&g_spring_visual_size)||
       !sa3ds_spring_visual_open(&g_spring_visual,g_spring_visual_bytes,g_spring_visual_size))goto fail;
    g_spring_states=calloc(g_movement.count,sizeof(*g_spring_states));
    if(!g_spring_states)goto fail;
    for(unsigned i=0;i<6;i++)
        if(!sa3ds_ec_gpu_init_full(&g_spring_gpu[i],&g_spring_visual.components[i],log_texture_stage,NULL))goto fail;
    g_springs_ready=1;reset_springs();
    boot_log("SPRINGS","Original spring models and SET placements loaded");return;
fail:
    for(unsigned i=0;i<6;i++)sa3ds_ec_gpu_destroy(&g_spring_gpu[i]);
    free(g_spring_states);g_spring_states=NULL;
    sa3ds_free_file(g_movement_bytes);g_movement_bytes=NULL;
    sa3ds_free_file(g_spring_visual_bytes);g_spring_visual_bytes=NULL;
    memset(&g_movement,0,sizeof(g_movement));
    boot_log("SPRINGS","Spring owner resources unavailable; gameplay objects remain incomplete");
}
static void tick_springs(void){
    if(!g_springs_ready)return;
    g_movement_ccl_queued=0;
    SA1Vec3 ccl_task=sa3ds_ec_task_origin(&g_player);
    SA1Vec3 ccl_up=sa1_vec3_normalize(g_player.ground_normal);
    SA3DSSpringPrimaryCollider collider={.task_origin=ccl_task,
        .center=sa1_vec3_add(ccl_task,sa1_vec3_scale(ccl_up,SA1_STEAM_SONIC_PARAMETER_EVIDENCE.center_height)),
        .radius=4,.half_height=4,.foot_radius=.65f,.gravity_axis=3};
    for(uint32_t i=0;i<g_movement.count;i++){
        SA3DSMovementPlacement p;sa3ds_movement_objects_placement(&g_movement,i,&p);
        unsigned kind=p.type&4095u;if(kind!=1&&kind!=2)continue;
        SA3DSNativeSpring *state=&g_spring_states[i];state->draw=0;
        SA1Vec3 task=sa3ds_ec_task_origin(&g_player);
        SA1Vec3 delta=sa1_vec3_sub(task,p.position);
        float distance=sa1_vec3_dot(delta,delta);
        if(p.act!=g_ec_act_index||(state->loaded&&distance>360100.f)){
            if(state->loaded){
                if(state->previous>=0)g_spring_states[state->previous].next=state->next;
                else g_spring_head=state->next;
                if(state->next>=0)g_spring_states[state->next].previous=state->previous;
            }
            memset(state,0,sizeof(*state));continue;
        }
        if(!state->loaded){
            unsigned high=(p.type>>12)&7u;
            int allowed=!high||(high==1?g_rings.clip_level<1:g_rings.clip_level<2);
            if(!allowed||distance>360000.f)continue;
            state->loaded=1;state->previous=-1;state->next=g_spring_head;
            if(g_spring_head>=0)g_spring_states[g_spring_head].previous=(int32_t)i;
            g_spring_head=(int32_t)i;
            for(unsigned axis=0;axis<3;axis++){
                uint16_t angle=(uint16_t)p.angles[axis];
                state->trig.sine[axis]=(float)spring_source_sin(&g_rings,angle);
                state->trig.cosine[axis]=(float)spring_source_sin(&g_rings,(uint16_t)(angle+0x4000));
            }
        }
    }
    /*40B030 inserts new tasks at the head of their priority list. Preserve
     * relative Spring task insertion order across load/unload, not row sort. */
    for(int32_t i=g_spring_head;i>=0;i=g_spring_states[i].next){
        SA3DSMovementPlacement p;sa3ds_movement_objects_placement(&g_movement,(uint32_t)i,&p);
        unsigned kind=p.type&4095u;SA3DSNativeSpring *state=&g_spring_states[i];
        /* Unresolved getter scratch is not a deferred launch request. Keep
         * the object visible, but quarantine it until its normal unload. */
        if(state->unsupported){state->draw=1;continue;}
        SA1Vec3 task=sa3ds_ec_task_origin(&g_player);
        SA1Vec3 forward=g_player.surface_frame_active?g_player.surface_forward:
            (SA1Vec3){sinf(g_player.yaw),0,cosf(g_player.yaw)};
        SA3DSSpringTickInput input={.collision_player=state->pending_hit?0:-1,
            .player_grounded=g_player.grounded,
            .player_local_forward=sa1_vec3_dot(g_player.vel,forward),
            .player_task_origin=task,.sine=spring_source_sin,.sine_context=&g_rings};
        SA3DSSpringTickResult result;
        if(!sa3ds_spring_actor_tick(&state->actor,&p,&state->trig,&input,&result)){
            boot_log("SPRINGS","Unsupported launch context: quarantined until unload; no delayed launch");state->unsupported=1;
            if(g_spring_probe)g_spring_probe_unsupported++;
            state->pending_hit=0;state->draw=1;continue;
        }
        state->draw=result.draw;state->pending_hit=0;
        if(result.launched){
            if(sa3ds_spring_player_apply(&g_player,&p,&state->trig,&result.launch,
                    SA3DS_EC_CONTROLLER_SUPPORT,&g_spring_pending)){
                if(g_spring_probe)g_spring_probe_launches++;
                char note[180];snprintf(note,sizeof(note),"tick=%llu row=%lu kind=%u command=%u task=%.3f,%.3f,%.3f velocity=%.3f,%.3f,%.3f",
                    (unsigned long long)g_player.sim_frame,(unsigned long)p.source_row,kind,result.launch.command.command,
                    result.launch.position.x,result.launch.position.y,result.launch.position.z,
                    g_player.vel.x,g_player.vel.y,g_player.vel.z);boot_log("SPRING_LAUNCH",note);
            }
        }
        if(result.enqueue_collision&&g_movement_ccl_queued<256){
            state->pending_hit=sa3ds_spring_primary_overlap(&p,&collider)==1;g_movement_ccl_queued++;
        }
    }
}
static void reset_panels(void){
    g_panel_head=-1;
    if(g_panel_states)memset(g_panel_states,0,g_movement.count*sizeof(*g_panel_states));
}
static void load_panels(void){
    if(!g_springs_ready)goto fail;
    if(!sa3ds_read_file("sdmc:/3ds/SA3DS/assets/panel_visual.sa3dspak",&g_panel_visual_bytes,&g_panel_visual_size)||
       !sa3ds_panel_visual_open(&g_panel_visual,g_panel_visual_bytes,g_panel_visual_size))goto fail;
    g_panel_states=calloc(g_movement.count,sizeof(*g_panel_states));if(!g_panel_states)goto fail;
    for(unsigned i=0;i<3;i++)
        if(!sa3ds_ec_gpu_init_full(&g_panel_gpu[i],&g_panel_visual.components[i],log_texture_stage,NULL))goto fail;
    g_panels_ready=1;reset_panels();boot_log("PANELS","Original animated panel components and SET placements loaded");return;
fail:
    for(unsigned i=0;i<3;i++)sa3ds_ec_gpu_destroy(&g_panel_gpu[i]);
    free(g_panel_states);g_panel_states=NULL;
    sa3ds_free_file(g_panel_visual_bytes);g_panel_visual_bytes=NULL;
    boot_log("PANELS","Owner panel resources unavailable; gameplay objects remain incomplete");
}
static void tick_panels(const SA1PlayerState *queued_player){
    if(!g_panels_ready)return;
    SA3CMatrix queued_transform;sa3ds_ec_model_transform(queued_player,&queued_transform);
    SA1Vec3 center={queued_transform.m[12],queued_transform.m[13],queued_transform.m[14]};
    for(uint32_t i=0;i<g_movement.count;i++){
        SA3DSMovementPlacement p;sa3ds_movement_objects_placement(&g_movement,i,&p);
        if((p.type&4095u)!=3)continue;
        SA3DSNativePanel *state=&g_panel_states[i];state->draw=0;
        SA1Vec3 delta=sa1_vec3_sub(sa3ds_ec_task_origin(&g_player),p.position);
        float distance=sa1_vec3_dot(delta,delta);
        if(p.act!=g_ec_act_index||(state->loaded&&distance>360100.f)){
            if(state->loaded){
                if(state->previous>=0)g_panel_states[state->previous].next=state->next;
                else g_panel_head=state->next;
                if(state->next>=0)g_panel_states[state->next].previous=state->previous;
            }
            memset(state,0,sizeof(*state));continue;
        }
        if(!state->loaded){
            unsigned high=(p.type>>12)&7u;
            int allowed=!high||(high==1?g_rings.clip_level<1:g_rings.clip_level<2);
            if(!allowed||distance>360000.f)continue;
            state->loaded=1;state->previous=-1;state->next=g_panel_head;
            if(g_panel_head>=0)g_panel_states[g_panel_head].previous=(int32_t)i;
            g_panel_head=(int32_t)i;
            for(unsigned axis=0;axis<3;axis++){
                uint16_t angle=(uint16_t)p.angles[axis];
                state->trig.sine[axis]=(float)spring_source_sin(&g_rings,angle);
                state->trig.cosine[axis]=(float)spring_source_sin(&g_rings,(uint16_t)(angle+0x4000));
            }
        }
    }
    for(int32_t i=g_panel_head;i>=0;i=g_panel_states[i].next){
        SA3DSMovementPlacement p;sa3ds_movement_objects_placement(&g_movement,(uint32_t)i,&p);
        SA3DSNativePanel *state=&g_panel_states[i];
        SA3DSDashPanelInput input={.first_hit_player=state->pending_hit?0:-1,
            .player_ground_status_bit0=g_player.grounded};SA3DSDashPanelResult result;
        if(!sa3ds_dash_panel_tick(&state->actor,&p,&input,&result)){state->pending_hit=0;continue;}
        state->pending_hit=0;
        float location[3]={p.position.x,p.position.y,p.position.z};
        state->draw=result.draw&&sa3ds_ec_gpu_sphere_visible(&g_ec_gpu,location,30.f);
        if(state->draw){
            state->frame=state->phase;state->phase=(state->phase+1)&7u;
            if(g_panel_probe&&p.source_row==g_panel_probe_row)g_panel_probe_sim_mask|=1u<<state->frame;
        }
        if(result.boost&&sa3ds_dash_panel_apply(&g_player,&p,&state->trig,&result.parameters,SA3DS_EC_CONTROLLER_SUPPORT)){
            char note[160];snprintf(note,sizeof(note),"tick=%llu row=%lu speed=%.3f lockout=%ld control=%d velocity=%.3f,%.3f,%.3f",
                (unsigned long long)g_player.sim_frame,(unsigned long)p.source_row,result.parameters.speed,
                (long)result.parameters.lockout,g_player.pc2004_control_timer,g_player.vel.x,g_player.vel.y,g_player.vel.z);
            boot_log("PANEL_BOOST",note);
            if(g_panel_probe&&p.source_row==g_panel_probe_row)g_panel_probe_boosts++;
        }
        if(result.enqueue_collision&&g_movement_ccl_queued<256){
            state->pending_hit=sa3ds_dash_panel_primary_overlap(&p,&state->trig,center,4.f)==1;
            g_movement_ccl_queued++;
        }
    }
}
static int draw_panels(void){
    if(!g_panels_ready)return 1;
    for(uint32_t i=0;i<g_movement.count;i++){
        SA3DSNativePanel *state=&g_panel_states[i];if(!state->draw)continue;
        SA3DSMovementPlacement p;sa3ds_movement_objects_placement(&g_movement,i,&p);
        if(p.act!=g_ec_act_index||(p.type&4095u)!=3)continue;
        SA3DSMovementPlacement orientation=p;orientation.type=2;SA1SurfaceFrame physical;
        SA3CMatrix pose[3];if(!sa3ds_springb_physical_frame(&orientation,&state->trig,&physical)||
            !sa3ds_panel_visual_pose(&g_panel_visual,state->frame,pose))return 0;
        C3D_Mtx actor;
        /* Display7A4360 uses SETY+8000: negate the original SET X/Z columns. */
        actor.r[0]=FVec4_New(-physical.right.x,physical.up.x,-physical.forward.x,p.position.x);
        actor.r[1]=FVec4_New(-physical.right.y,physical.up.y,-physical.forward.y,p.position.y);
        actor.r[2]=FVec4_New(-physical.right.z,physical.up.z,-physical.forward.z,p.position.z);
        actor.r[3]=FVec4_New(0,0,0,1);
        for(unsigned part=0;part<3;part++){
            if(g_panel_visual.nodes[part*16+4]&SA3C_EVAL_HIDE)continue;
            C3D_Mtx local,model,mv;
            for(unsigned row=0;row<4;row++)local.r[row]=FVec4_New(
                pose[part].m[row],pose[part].m[4+row],pose[part].m[8+row],pose[part].m[12+row]);
            Mtx_Multiply(&model,&actor,&local);Mtx_Multiply(&mv,&g_ec_view,&model);
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&mv);
            if(!sa3ds_ec_gpu_draw(&g_panel_gpu[part],0)||!sa3ds_ec_gpu_draw(&g_panel_gpu[part],1))return 0;
            if(g_panel_probe&&p.source_row==g_panel_probe_row){
                g_panel_probe_draws++;g_panel_probe_mask|=1u<<state->frame;
            }
        }
    }
    return 1;
}
static int draw_springs(void){
    if(!g_springs_ready)return 1;
    for(uint32_t i=0;i<g_movement.count;i++){
        SA3DSNativeSpring *state=&g_spring_states[i];if(!state->draw)continue;
        SA3DSMovementPlacement p;sa3ds_movement_objects_placement(&g_movement,i,&p);
        unsigned kind=p.type&4095u;if(p.act!=g_ec_act_index||(kind!=1&&kind!=2))continue;
        SA3DSMovementPlacement orientation=p;orientation.type=2;
        SA1SurfaceFrame physical;
        if(!sa3ds_springb_physical_frame(&orientation,&state->trig,&physical))return 0;
        /* Physical frame uses SETY-4000. Original object display's SETY basis
         * is consequently (physical.right, physical.up, physical.forward). */
        for(unsigned part=0;part<3;part++){
            unsigned component=(kind-1)*3+part;float *offset=g_spring_visual.offsets[component];
            float y=offset[1]+(part==2?state->actor.compression*g_spring_visual.cap_displacement[component]:0);
            SA1Vec3 origin=sa1_vec3_add(p.position,sa1_vec3_add(sa1_vec3_scale(physical.right,offset[0]),
                sa1_vec3_add(sa1_vec3_scale(physical.up,y),sa1_vec3_scale(physical.forward,offset[2]))));
            float stretch=part==1?1.f+state->actor.compression:1.f;if(stretch<=0)stretch=1;
            C3D_Mtx model,mv;
            model.r[0]=FVec4_New(physical.right.x,physical.up.x*stretch,physical.forward.x,origin.x);
            model.r[1]=FVec4_New(physical.right.y,physical.up.y*stretch,physical.forward.y,origin.y);
            model.r[2]=FVec4_New(physical.right.z,physical.up.z*stretch,physical.forward.z,origin.z);
            model.r[3]=FVec4_New(0,0,0,1);
            Mtx_Multiply(&mv,&g_ec_view,&model);
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&mv);
            if(!sa3ds_ec_gpu_draw(&g_spring_gpu[component],0)||!sa3ds_ec_gpu_draw(&g_spring_gpu[component],1))return 0;
        }
    }
    return 1;
}

/* Explicit owner-data development fixture. This never runs in normal play. */
static void begin_spring_probe(void){
    FILE *file=fopen("sdmc:/3ds/SA3DS/emerald_coast_spring_probe.flag","rb");
    if(!file)return;
    unsigned row=0;int parsed=fscanf(file,"%u",&row)==1;fclose(file);
    if(!parsed||!g_springs_ready||!g_ec_full_route){boot_log("SPRING_PROBE","FAIL: full owner route/resources required");g_paused=1;return;}
    SA3DSMovementPlacement target={0};int found=0;
    for(uint32_t i=0;i<g_movement.count;i++){
        SA3DSMovementPlacement p;sa3ds_movement_objects_placement(&g_movement,i,&p);
        if(p.act==g_ec_act_index&&p.source_row==row&&(p.type&4095u)<=2){target=p;found=1;break;}
    }
    if(!found||!sa3ds_ec_residency_recenter(&g_ec_collision,g_ec_collision_bytes,g_ec_collision_size,
        target.position.x,target.position.z,&g_player))goto fail;
    g_ec_collision.reference_y=target.position.y+8;
    SA1Surface floor=sa3ds_ec_collision_surface(&g_ec_collision,target.position.x,target.position.z);
    if(!floor.valid||floor.normal.y<.8f||fabsf(floor.ground_y-target.position.y)>20)goto fail;
    g_player.pos=(SA1Vec3){target.position.x,floor.ground_y+SA3DS_EC_CONTROLLER_SUPPORT,target.position.z};
    g_player.vel=(SA1Vec3){0};g_player.grounded=true;g_player.surface_id=floor.surface_id;
    g_player.ground_normal=floor.normal;g_player.surface_frame_active=false;
    g_player.yaw=0;g_player.surface_forward=(SA1Vec3){0,0,1};g_player.surface_right=(SA1Vec3){1,0,0};
    reset_springs();g_spring_probe=1;g_spring_probe_ticks=0;
    g_spring_probe_launches=g_spring_probe_rise=g_spring_probe_fall=g_spring_probe_unsupported=0;
    g_camera.yaw=0;
    boot_log("SPRING_PROBE","BEGIN: explicit original-placement launch fixture; course progress is not being tested");return;
fail:
    boot_log("SPRING_PROBE","FAIL: original placement lacks supported nearby terrain");g_paused=1;
}
static void begin_panel_probe(void){
    FILE *file=fopen("sdmc:/3ds/SA3DS/emerald_coast_panel_probe.flag","rb");
    if(!file)return;
    unsigned row=0;int parsed=fscanf(file,"%u",&row)==1;fclose(file);
    if(!parsed||!g_panels_ready||!g_ec_full_route){boot_log("PANEL_PROBE","FAIL: full owner route/resources required");g_paused=1;return;}
    SA3DSMovementPlacement target={0};int found=0;
    for(uint32_t i=0;i<g_movement.count;i++){
        SA3DSMovementPlacement p;sa3ds_movement_objects_placement(&g_movement,i,&p);
        if(p.act==g_ec_act_index&&p.source_row==row&&(p.type&4095u)==3){target=p;found=1;break;}
    }
    if(!found||!sa3ds_ec_residency_recenter(&g_ec_collision,g_ec_collision_bytes,g_ec_collision_size,
        target.position.x,target.position.z,&g_player))goto fail;
    g_ec_collision.reference_y=target.position.y+8;
    SA1Surface floor=sa3ds_ec_collision_surface(&g_ec_collision,target.position.x,target.position.z);
    if(!floor.valid||floor.normal.y<.8f||fabsf(floor.ground_y-target.position.y)>20)goto fail;
    g_player.pos=(SA1Vec3){target.position.x,floor.ground_y+SA3DS_EC_CONTROLLER_SUPPORT,target.position.z};
    g_player.vel=(SA1Vec3){0};g_player.grounded=true;g_player.surface_id=floor.surface_id;
    g_player.ground_normal=floor.normal;g_player.surface_frame_active=false;
    g_player.yaw=0;g_player.surface_forward=(SA1Vec3){0,0,1};g_player.surface_right=(SA1Vec3){1,0,0};
    reset_springs();g_panel_probe=1;g_panel_probe_ticks=0;
    g_panel_probe_row=target.source_row;g_panel_probe_boosts=g_panel_probe_mask=g_panel_probe_draws=g_panel_probe_sim_mask=0;
    g_camera.yaw=0;
    boot_log("PANEL_PROBE","BEGIN: explicit original-placement panel fixture; course progress is not being tested");return;
fail:
    boot_log("PANEL_PROBE","FAIL: original placement lacks supported nearby terrain");g_paused=1;
}

static void reset_saru(void){
    g_saru_head=-1;g_saru_rng=1;sa3ds_ccl_query_reset(&g_saru_damage_cursor);
    if(g_saru_states)memset(g_saru_states,0,g_saru_definitions.placements.count*sizeof(*g_saru_states));
}
static uint16_t saru_random(void*context){
    (void)context;g_saru_rng=g_saru_rng*214013u+2531011u;return (uint16_t)((g_saru_rng>>16)&32767u);
}
static double saru_sine(void*context,uint16_t angle){(void)context;return spring_source_sin(&g_rings,angle);}
static int32_t saru_atan(void*context,double y,double x){(void)context;return (int32_t)trunc(atan2(y,x)*65536.*.15915497620314795);}
static double saru_floor(void*context,const SA3DSSaruActor*actor){
    (void)context;float height;
    /* Caller verifies coverage before entering the source task. */
    if(!sa3ds_ec_collision_floor_below(&g_ec_collision,actor->position.x,actor->position.y+4.f,actor->position.z,&height))return NAN;
    return height;
}
static int saru_damage(void*context,SA3DSSaruActor*actor){
    (void)context;
    /* Native NPC CCL and global explosion producer are not connected yet.
     * Execute the shared source decision with the actual empty world, rather
     * than inventing attacks or awarding score without a source contact. */
    SA3DSEnemyDamageInput world={.explosion_radius=-1};SA3DSEnemyDamageResult result;
    return sa3ds_saru_damage_check(actor,&world,&g_saru_damage_cursor,&result)&&result.killed;
}
static const SA3DSSaruServices g_saru_services={.sine=saru_sine,.atan_bams=saru_atan,.random15=saru_random,.floor_height=saru_floor,.damage=saru_damage};
static void destroy_saru(void){
    sa3ds_sonic_gpu_destroy(&g_saru_gpu);free(g_saru_states);g_saru_states=NULL;
    sa3ds_free_file(g_saru_definition_bytes);sa3ds_free_file(g_saru_character_bytes);sa3ds_free_file(g_saru_texture_bytes);
    g_saru_definition_bytes=g_saru_character_bytes=g_saru_texture_bytes=NULL;
    memset(&g_saru_definitions,0,sizeof(g_saru_definitions));g_saru_ready=0;g_saru_head=-1;
}
static void load_saru(void){
    if(!g_ec_combined||!g_rings_ready||!g_rings.quarter_lut)goto fail;
    if(!sa3ds_read_file("sdmc:/3ds/SA3DS/assets/saru_definitions.sa3dspak",&g_saru_definition_bytes,&g_saru_definition_size)||
       !sa3ds_saru_definitions_open(&g_saru_definitions,g_saru_definition_bytes,g_saru_definition_size)||
       !g_saru_definitions.player_colliders||!g_saru_definitions.placements.count||
       !sa3ds_read_file("sdmc:/3ds/SA3DS/assets/saru.sa3c",&g_saru_character_bytes,&g_saru_character_size)||
       !sa3ds_read_file("sdmc:/3ds/SA3DS/assets/saru.sa3t",&g_saru_texture_bytes,&g_saru_texture_size)||
       sa3c_character_open(&g_saru_character,g_saru_character_bytes,g_saru_character_size)!=SA3C_OK||
       sa3c_character_validate_references(&g_saru_character)!=SA3C_OK||
       g_saru_character.sections[0].count!=38||g_saru_character.sections[7].count!=6||
       sa3t_open(&g_saru_textures,g_saru_texture_bytes,g_saru_texture_size)!=SA3T_OK)goto fail;
    SA3DSGeometryReport geometry;
    if(sa3ds_geometry_preflight(&g_saru_character,&geometry)!=SA3DS_GEOMETRY_OK)goto fail;
    for(unsigned n=0;n<6;n++)if(!(g_saru_frames[n]=action_frame_count(&g_saru_character,n)))goto fail;
    g_saru_states=calloc(g_saru_definitions.placements.count,sizeof(*g_saru_states));
    if(!g_saru_states||!sa3ds_sonic_gpu_init_instances_traced(&g_saru_gpu,&g_saru_character,&g_saru_textures,
        g_saru_definitions.placements.count,log_texture_stage,NULL))goto fail;
    g_saru_ready=1;reset_saru();
    boot_log("SARU","Owner models/6 motions/SET loaded; native ordinary AI/display; NPC CCL, bombs, effects and source culling remain incomplete");return;
fail:destroy_saru();boot_log("SARU","Owner resources unavailable or invalid; native enemies remain incomplete");
}
static int tick_saru(void){
    if(!g_saru_ready)return 1;
    SA1Vec3 player=sa3ds_ec_task_origin(&g_player);
    for(uint32_t n=0;n<g_saru_definitions.placements.count;n++){
        SA3DSMovementPlacement p;sa3ds_movement_objects_placement(&g_saru_definitions.placements,n,&p);
        SA3DSNativeSaru*s=&g_saru_states[n];SA1Vec3 delta=sa1_vec3_sub(player,p.position);
        float distance=sa1_vec3_dot(delta,delta);
        /* Bounded activation adapter: source46C010 camera relevance and SET
         * task persistence still require reconstruction. Keep the recovered
         * 600/700 source radii, never spring's unrelated unload threshold. */
        if(p.act!=g_ec_act_index||(s->loaded&&distance>g_saru_definitions.unload_squared)){
            if(s->loaded){if(s->previous>=0)g_saru_states[s->previous].next=s->next;else g_saru_head=s->next;
                if(s->next>=0)g_saru_states[s->next].previous=s->previous;}
            memset(s,0,sizeof(*s));continue;
        }
        if(!s->loaded){
            unsigned high=(p.type>>12)&7u;
            if(distance>g_saru_definitions.load_squared||(high&&(high==1?g_rings.clip_level>=1:g_rings.clip_level>=2)))continue;
            if(!sa3ds_saru_actor_init(&s->actor,&p,&g_saru_services))return 0;
            s->loaded=1;s->previous=-1;s->next=g_saru_head;
            if(g_saru_head>=0)g_saru_states[g_saru_head].previous=(int32_t)n;
            g_saru_head=(int32_t)n;
        }
    }
    for(int32_t n=g_saru_head;n>=0;n=g_saru_states[n].next){
        SA3DSNativeSaru*s=&g_saru_states[n];s->display=0;
        /* Do not interpret nonresident terrain as the source missing-floor
         * sentinel. Defer this bounded adapter until its complete orbit fits
         * the resident static query (490 includes source candidate radius10). */
        if((s->actor.status&0x2000)&&hypotf(s->actor.origin.x-g_ec_collision.center_x,s->actor.origin.z-g_ec_collision.center_z)+s->actor.scale.z>490.f)continue;
        float center[3]={s->actor.position.x,s->actor.position.y,s->actor.position.z};
        SA3DSSaruActorInput in={.player_position=player,.action_frames=g_saru_frames,
            .visible=sa3ds_ec_gpu_sphere_visible(&g_ec_gpu,center,g_saru_definitions.visibility_radius)};
        SA3DSSaruActorResult out;
        if(!sa3ds_saru_actor_tick(&s->actor,&in,&g_saru_services,&out))return 0;
        s->display=out.display;
        if(g_saru_probe&&s->actor.action<32)g_saru_probe_actions|=1u<<s->actor.action;
        g_saru_projectiles+=out.projectile;g_saru_sounds+=out.sound_count;g_saru_dust+=out.dust_bursts;
        /* Requests remain observable counters until their source executors are
         * connected. No substitute bomb, score, damage or animal is spawned. */
    }
    return 1;
}
static int draw_saru(void){
    g_saru_frame_actors=0;if(!g_saru_ready)return 1;
    sa3ds_sonic_gpu_begin_frame(&g_saru_gpu);
    for(int32_t n=g_saru_head;n>=0;n=g_saru_states[n].next){
        const SA3DSNativeSaru*s=&g_saru_states[n];const SA3DSSaruActor*a=&s->actor;
        float center[3]={a->position.x,a->position.y,a->position.z};
        if(!s->display||!sa3ds_ec_gpu_sphere_visible(&g_ec_gpu,center,g_saru_definitions.visibility_radius))continue;
        uint16_t root=INVALID_U16;
        if(sa3ds_saru_pose_sample(&g_saru_character,&a->animation,g_saru_local,&root)!=SA3C_POSE_OK||
           sa3c_pose_build_world_animated(&g_saru_character,root,g_saru_local,38,g_saru_world,38)!=SA3C_POSE_OK)return 0;
        /*4AB2B0 read-only normal display: T*Ry(X+Y)*S*Ry(8000).
         * Bob is specific to the logic render helper, not normal display. */
        C3D_Mtx model,mv;Mtx_Identity(&model);Mtx_Translate(&model,a->position.x,a->position.y,a->position.z,true);
        Mtx_RotateY(&model,(uint16_t)((uint32_t)a->angles[0]+(uint32_t)a->angles[1])*(2.f*PI/65536.f),true);
        Mtx_Scale(&model,a->scale.x*g_saru_definitions.display_scale,a->scale.y*g_saru_definitions.display_scale,a->scale.x*g_saru_definitions.display_scale);
        Mtx_RotateY(&model,PI,true);Mtx_Multiply(&mv,&g_ec_view,&model);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&mv);
        if(!sa3ds_sonic_gpu_draw_append(&g_saru_gpu,root,g_saru_world,38,NULL))return 0;
        g_saru_frame_actors++;
    }
    if(g_saru_probe){g_saru_probe_draws+=g_saru_frame_actors;
        if(g_saru_frame_actors>g_saru_probe_max_actors)g_saru_probe_max_actors=g_saru_frame_actors;}
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&g_ec_view);return 1;
}
static void begin_saru_probe(void){
    FILE*f=fopen("sdmc:/3ds/SA3DS/emerald_coast_saru_probe.flag","rb");if(!f)return;
    unsigned row=0;int parsed=fscanf(f,"%u",&row)==1;fclose(f);
    if(!parsed||!g_saru_ready||!g_ec_full_route)goto fail;
    SA3DSMovementPlacement target={0};int found=0;
    for(uint32_t n=0;n<g_saru_definitions.placements.count;n++){
        SA3DSMovementPlacement p;if(!sa3ds_movement_objects_placement(&g_saru_definitions.placements,n,&p))goto fail;
        if(p.act==g_ec_act_index&&p.source_row==row){target=p;found=1;break;}
    }
    float x=target.position.x,z=target.position.z-70.f;
    if(!found||!sa3ds_ec_residency_recenter(&g_ec_collision,g_ec_collision_bytes,g_ec_collision_size,x,z,&g_player))goto fail;
    g_ec_collision.reference_y=target.position.y+8;
    SA1Surface floor=sa3ds_ec_collision_surface(&g_ec_collision,x,z);if(!floor.valid||floor.normal.y<.8f)goto fail;
    g_player.pos=(SA1Vec3){x,floor.ground_y+SA3DS_EC_CONTROLLER_SUPPORT,z};g_player.vel=(SA1Vec3){0};
    g_player.grounded=true;g_player.ground_normal=floor.normal;g_player.surface_id=floor.surface_id;
    g_player.surface_frame_active=false;g_player.yaw=0;g_player.surface_forward=(SA1Vec3){0,0,1};g_player.surface_right=(SA1Vec3){1,0,0};
    reset_springs();g_camera.yaw=0;g_saru_probe=1;
    g_saru_probe_ticks=g_saru_probe_draws=g_saru_probe_actions=g_saru_probe_max_actors=0;
    boot_log("SARU_PROBE","BEGIN: explicit original-placement ordinary AI/display fixture; combat/course parity not tested");return;
fail:boot_log("SARU_PROBE","FAIL: missing original resource/placement/support");g_paused=1;
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
        boot_checkpoint(g_ec_full_route?"CPU-swizzling complete Emerald Coast act":"CPU-swizzling Emerald Coast opening subset");
        if(g_ec_full_route){
            if(!sa3ds_ec_gpu_init_full(&g_ec_gpu,&g_ec_visual,log_texture_stage,NULL))return 0;
        }else if (!sa3ds_ec_gpu_init(&g_ec_gpu,&g_ec_visual,500.0f,log_texture_stage,NULL)) return 0;
        boot_note(g_ec_full_route?"Emerald Coast complete act terrain resident":"Emerald Coast opening subset resident");
        ec_apply_texture_override();
        if (g_ec_combined && !sa3ds_sonic_gpu_init_traced(&g_sonic_gpu,
                &g_character,&g_texture_archive,log_texture_stage,NULL)) return 0;
        load_rings();
        load_background();
        load_springs();
        load_panels();
        load_saru();
        if(g_ec_combined){
            uint16_t root=INVALID_U16;SA3DSPoseMetrics stand;
            if(sa3c_pose_sample_action(&g_character,1,0,0,g_local,g_node_count,&root)!=SA3C_POSE_OK||
               sa3c_pose_build_world_animated(&g_character,root,g_local,g_node_count,g_world,g_node_count)!=SA3C_POSE_OK||
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
        if(g_ec_gpu.full_act){
            float eye[3]={g_player.pos.x-sinf(g_camera.yaw)*40.0f,
                g_player.pos.y+18.0f,g_player.pos.z-cosf(g_camera.yaw)*40.0f};
            float target[3]={g_player.pos.x,g_player.pos.y+3.0f,g_player.pos.z};
            sa3ds_ec_gpu_set_view(&g_ec_gpu,eye,target,C3D_AngleFromDegrees(55.0f),
                C3D_AspectRatioTop,0.1f,5000.0f);
        }
        float background_eye[3]={g_ec_combined?g_player.pos.x-sinf(g_camera.yaw)*40.0f:0.0f,
            g_ec_combined?g_player.pos.y+18.0f:95.0f,
            g_ec_combined?g_player.pos.z-cosf(g_camera.yaw)*40.0f:-260.0f};
        C3D_CullFace(GPU_CULL_NONE);
        if(!draw_background(&stage_mv,background_eye))return 0;
        g_ec_view=stage_mv;
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,g_u_modelview,&stage_mv);
        C3D_CullFace(GPU_CULL_NONE);
        if (!sa3ds_ec_gpu_draw(&g_ec_gpu,0)) return 0;
        if (!draw_rings()||!draw_springs()||!draw_panels()||!draw_saru())return 0;
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
    pr=(g_pose_lab && g_pose_bind) ?
        sa3c_pose_build_world(&g_character,root,g_local,g_node_count,g_world,g_node_count) :
        sa3c_pose_build_world_animated(&g_character,root,g_local,g_node_count,g_world,g_node_count);
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
        Mtx_RotateY(&sonic_mv,sa3ds_view_model_yaw(g_player.yaw,g_camera.yaw),true);
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
    destroy_saru();
    for(unsigned i=0;i<3;i++)sa3ds_ec_gpu_destroy(&g_panel_gpu[i]);
    free(g_panel_states);g_panel_states=NULL;g_panels_ready=0;
    sa3ds_free_file(g_panel_visual_bytes);g_panel_visual_bytes=NULL;
    for(unsigned i=0;i<6;i++)sa3ds_ec_gpu_destroy(&g_spring_gpu[i]);
    free(g_spring_states);g_spring_states=NULL;g_springs_ready=0;
    sa3ds_free_file(g_movement_bytes);g_movement_bytes=NULL;
    sa3ds_free_file(g_spring_visual_bytes);g_spring_visual_bytes=NULL;
    sa3ds_ec_gpu_destroy(&g_ring_gpu);sa3ds_rings_destroy(&g_rings);g_rings_ready=0;
    sa3ds_free_file(g_ring_visual_bytes);sa3ds_free_file(g_ring_placement_bytes);
    g_ring_visual_bytes=g_ring_placement_bytes=NULL;
    sa3ds_ec_gpu_destroy(&g_background_gpu);
    sa3ds_free_file(g_background_bytes);g_background_bytes=NULL;g_background_ready=0;
    sa3ds_ec_gpu_destroy(&g_ec_gpu);
    sa3ds_ec_collision_destroy(&g_ec_collision);
    sa3ds_lab_gpu_destroy(&g_lab_gpu);
    sa3ds_sonic_gpu_destroy(&g_sonic_gpu);
    free(g_world); free(g_transition_local); free(g_local);
    g_world=NULL; g_transition_local=NULL; g_local=NULL;
    sa3ds_free_file(g_sa3t_bytes); sa3ds_free_file(g_sa3c_bytes);
    sa3ds_free_file(g_ec_bytes); g_ec_bytes=NULL; g_ec_size=0;
    sa3ds_free_file(g_ec_collision_bytes);g_ec_collision_bytes=NULL;g_ec_collision_size=0;
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
    int replay_mode=replay?fgetc(replay):EOF;
    g_ec_replay_pause=replay_mode=='P';
    g_wca_replay=g_ec_replay && replay_mode=='M';
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
    if(g_emerald_coast&&g_ec_combined){begin_spring_probe();begin_panel_probe();begin_saru_probe();}
    if(g_emerald_coast&&g_ec_combined){
        FILE *probe=fopen("sdmc:/3ds/SA3DS/emerald_coast_air_attack_probe.flag","rb");
        if(probe){fclose(probe);g_air_attack_probe=1;g_air_attack_probe_previous_action=g_player.pc2004_action_state;
            boot_log("AIR_ATTACK_PROBE","BEGIN: input-only jump/release/second-jump at original start; targetless path only");}
    }
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
        if (g_emerald_coast && modifier && (down&KEY_DUP)) ec_toggle_texture_override();
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
        if(g_ec_full_route&&g_ec_cycle_probe&&g_ec_act_index==0&&g_rendered_frames>=360){
            g_ec_cycle_probe=0;
            boot_log("VALIDATION","Explicit act0->act1 geometry loader probe; not natural stage progression");
            if(!ec_change_act(1)){g_ec_transition_failed=1;fail_screen("Act-cycle validation failed");break;}
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
            if(g_ec_full_route)g_ec_perf.ticks++;
            SA1Input tick_input=sa3ds_wz_tick_input(&g_tick_clock,step);
            SA3DSWCAReplayStep wca={0};
            if(g_ec_replay) {
                if(g_wca_replay) {
                    wca=sa3ds_wca_replay_step(g_ec_replay_tick);
                    if(wca.reset)reset_selected_zone();
                    if(g_ec_replay_tick==0){
                        trace_toggle();g_wca_perf_active=1;
                        g_wca_perf_start_ms=osGetTime();
                    }
                    tick_input=wca.screen_input;
                    g_camera.yaw=wca.camera_yaw;
                } else {
                    if(g_ec_replay_tick==0){reset_selected_zone();trace_toggle();}
                    tick_input=sa3ds_ec_replay_input(g_ec_replay_tick);
                    /* Fixed camera for equivalent scripted directional inputs. */
                    g_camera.yaw=0;
                }
            }
            if(g_ring_probe){
                g_camera.yaw=0;
                tick_input=sa3ds_ring_replay_input_mode(&g_rings,&g_player,0,g_ring_probe_tick,g_ring_probe_groups);
            }
            if(g_spring_probe||g_panel_probe||g_saru_probe){tick_input=(SA1Input){0};g_camera.yaw=0;}
            if(g_air_attack_probe){
                tick_input=(SA1Input){0};
                tick_input.jump_pressed=g_air_attack_probe_ticks==0||g_air_attack_probe_ticks==3;
                tick_input.jump_held=g_air_attack_probe_ticks!=2;
                tick_input.jump_released=g_air_attack_probe_ticks==2;
                if(g_air_attack_probe_ticks>=4&&g_air_attack_probe_ticks<=6)tick_input.move_x=.5f;
            }
            /* Screen input -> RH chase view; physics turn inertia is unchanged. */
            tick_input=sa3ds_view_input(tick_input);
            SA1SteamFlatDebug physics_trace={0};
            if (!g_ec_replay && !g_ring_probe && !modifier && (held&KEY_L)) sa3ds_parity_camera_manual_yaw(&g_camera,-0.035f);
            if (!g_ec_replay && !g_ring_probe && !modifier && (held&KEY_R)) sa3ds_parity_camera_manual_yaw(&g_camera,0.035f);
            if(g_ec_full_route){
                uint32_t next_act;
                if(sa3ds_ec_route_next(&g_ec_route,g_ec_act_index,sa3ds_ec_task_origin(&g_player),&next_act)){
                    if(!ec_change_act(next_act)){g_ec_transition_failed=1;fail_screen("Source act handoff failed: check external owner packages");break;}
                }
                ec_refresh_collision_region();
            }
            if(!sa3ds_spring_player_begin(&g_player,&g_spring_pending,&tick_input)){
                boot_log("SPRINGS","Player command rejected");break;
            }
            SA1Vec3 previous=g_player.pos;
            uint64_t previous_tick=g_player.sim_frame;
            if(g_emerald_coast)g_ec_collision.reference_y=g_player.pos.y;
            sa1_physics_step_steam_sonic_flat(&g_player,&tick_input,g_camera.yaw,
                g_emerald_coast?&g_ec_collision.world:sa3ds_parity_lab_world(),
                &SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&physics_trace);
            sa3ds_spring_player_tail(&g_player);
            if(g_emerald_coast && !(previous_tick && g_player.sim_frame==1))
                sa3ds_ec_collision_resolve_walls(&g_ec_collision,previous,&g_player);
            if(g_ec_full_route && previous_tick && g_player.sim_frame==1){
                g_rings.rings=0;reset_selected_zone();
                if(g_ring_probe){boot_log("RING_PROBE","FAIL: physical death/reset during input-only replay");g_ring_probe=0;g_paused=1;}
                if(g_panel_probe){boot_log("PANEL_PROBE","FAIL: physical death/reset during fixture");g_panel_probe=0;g_paused=1;}
                if(g_air_attack_probe){boot_log("AIR_ATTACK_PROBE","FAIL: physical death/reset during fixture");g_air_attack_probe=0;g_paused=1;}
            }
            if(g_spring_probe&&g_player.pc2004_action_state_valid){
                if(g_player.pc2004_action_state==9||g_player.pc2004_action_state==10)g_spring_probe_rise++;
                if(g_player.pc2004_action_state==12)g_spring_probe_fall++;
            }
            SA1PlayerState physics_done=g_player;
            g_observation=sa3ds_sonic_animation_observe(&g_anim_sync,&g_player,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
            g_selection=sa3ds_sonic_animation_select(&g_anim_runtime,&g_player,&g_observation);
            if(g_spring_probe &&
                (g_selection.requested_runtime_id==16 || g_selection.requested_runtime_id==17 ||
                 g_selection.requested_runtime_id==18) &&
                (!g_anim_runtime.initialized || previous_spring_animation!=g_selection.requested_runtime_id)){
                char note[100];snprintf(note,sizeof(note),"tick=%llu action=%u request=%u",
                    (unsigned long long)g_player.sim_frame,g_player.pc2004_action_state,
                    g_selection.requested_runtime_id);boot_log("SPRING_ANIMATION",note);
            }
            previous_spring_animation=g_selection.requested_runtime_id;
            if(g_air_attack_probe){
                unsigned action=g_player.pc2004_action_state;
                if(action==14)g_air_attack_probe_attack_ticks++;
                if(action!=g_air_attack_probe_previous_action){
                    char note[170];snprintf(note,sizeof(note),"tick=%llu action=%u timer=%d age=%d request=%u ball=%u jumpTicks=%lu",
                        (unsigned long long)g_player.sim_frame,action,g_player.pc2004_air_attack_timer,
                        g_player.pc2004_air_attack_age,g_selection.requested_runtime_id,g_player.ball_form,
                        (unsigned long)g_player.jump_ticks);boot_log("AIR_ATTACK_STATE",note);
                    if(action==14&&g_player.sim_frame==4&&g_player.pc2004_air_attack_timer==2&&
                       g_selection.requested_runtime_id==14&&g_player.ball_form&&g_player.jump_ticks==1)g_air_attack_probe_entry=1;
                    if(action==12&&g_player.sim_frame==9&&g_player.pc2004_air_attack_timer==0&&
                       g_selection.requested_runtime_id==18&&!g_player.ball_form&&g_player.jump_ticks==1)g_air_attack_probe_exit=1;
                }
                g_air_attack_probe_previous_action=action;
                if(++g_air_attack_probe_ticks>=24){
                    char note[170];snprintf(note,sizeof(note),"%s: ticks=%u attackTicks=%u entry=%u exit=%u; input-only targetless fixture",
                        g_air_attack_probe_entry&&g_air_attack_probe_exit&&g_air_attack_probe_attack_ticks==5?"PASS":"FAIL",
                        g_air_attack_probe_ticks,g_air_attack_probe_attack_ticks,g_air_attack_probe_entry,g_air_attack_probe_exit);
                    boot_log("AIR_ATTACK_PROBE",note);g_air_attack_probe=0;g_paused=1;
                }
            }
            if (memcmp(&physics_done,&g_player,sizeof(g_player))!=0) {
                fail_screen("animation mutated physics state");
                trace_stop();
                cleanup(); C3D_Fini(); romfsExit(); gfxExit();
                return 1;
            }
            if (g_trace_file) {
                const SA3DSParityZone *trace_zone=sa3ds_parity_lab_zone_by_id(g_selected_zone);
                char replay_name[64];
                if(g_wca_replay)snprintf(replay_name,sizeof(replay_name),
                    "WCA_%03u_%s",wca.case_id,wca.name);
                if (sa3ds_parity_trace_write_frame(g_trace_file,g_selected_zone,
                        g_wca_replay?replay_name:(g_emerald_coast?"EMERALD_COAST_ACT0_PROVISIONAL":(trace_zone?trace_zone->name:"UNKNOWN")),&g_player,&tick_input,
                        g_camera.yaw,&physics_trace,&g_selection)!=0) trace_stop();
                else { g_trace_frames++; if ((g_trace_frames&63u)==0) fflush(g_trace_file); }
            }
            /* List2 spring objects write commands after Sonic's current
             * movement/animation phase; consume them next player tick. */
            tick_springs();
            tick_panels(&physics_done);
            if(!tick_saru()){fail_screen("Native E SARU task failed");break;}
            if(g_saru_probe&&++g_saru_probe_ticks>=180){
                char note[220];snprintf(note,sizeof(note),"%s ticks=%u draws=%u maxActors=%u actions=%08x projectileRequests=%lu; ordinary AI/display fixture only",
                    g_saru_probe_draws&&g_saru_probe_actions?"PASS":"FAIL",g_saru_probe_ticks,g_saru_probe_draws,g_saru_probe_max_actors,g_saru_probe_actions,(unsigned long)g_saru_projectiles);
                boot_log("SARU_PROBE",note);g_saru_probe=0;g_paused=1;
            }
            if(g_panel_probe&&++g_panel_probe_ticks>=32){
                char note[180];snprintf(note,sizeof(note),"%s: ticks=%u boosts=%u draws=%u simulatedPhases=%02x renderedPhases=%02x; explicit placement fixture only",
                    g_panel_probe_boosts&&g_panel_probe_draws&&g_panel_probe_sim_mask==255?"PASS":"FAIL",
                    g_panel_probe_ticks,g_panel_probe_boosts,g_panel_probe_draws,g_panel_probe_sim_mask,g_panel_probe_mask);
                boot_log("PANEL_PROBE",note);g_panel_probe=0;g_paused=1;
            }
            if(g_spring_probe&&++g_spring_probe_ticks>=180){
                char note[160];snprintf(note,sizeof(note),"%s: ticks=%u launches=%u riseTicks=%u fallTicks=%u unsupported=%u; controlled fixture only",
                    g_spring_probe_launches&&g_spring_probe_rise&&g_spring_probe_fall&&!g_spring_probe_unsupported?"PASS":"FAIL",
                    g_spring_probe_ticks,g_spring_probe_launches,g_spring_probe_rise,g_spring_probe_fall,g_spring_probe_unsupported);
                boot_log("SPRING_PROBE",note);g_spring_probe=0;g_paused=1;
            }
            if(g_rings_ready){
                /* Player CCL was queued in Sonic's phase, before list2
                 * spring commands can change its task transform. */
                SA3CMatrix actor;sa3ds_ec_model_transform(&physics_done,&actor);
                SA1Vec3 center={actor.m[12],actor.m[13],actor.m[14]};
                unsigned collected=sa3ds_rings_tick_presented(&g_rings,sa3ds_ec_task_origin(&g_player),center,
                    SA1_STEAM_SONIC_PARAMETER_EVIDENCE.rad,0,1,g_rings.clip_level,step+1==steps);
                if(collected){char note[96];snprintf(note,sizeof(note),"tick=%llu collected=%u rings=%d",
                    (unsigned long long)g_player.sim_frame,collected,g_rings.rings);boot_log("RINGS",note);}
            }
            if(g_ring_probe){
                if(g_ring_probe_tick%120==0){char note[144];snprintf(note,sizeof(note),
                    "tick=%u p=%.3f,%.3f,%.3f input=%.3f,%.3f rings=%d",g_ring_probe_tick,
                    g_player.pos.x,g_player.pos.y,g_player.pos.z,tick_input.move_x,tick_input.move_z,g_rings.rings);
                    boot_log("RING_PROBE",note);}
                uint32_t events=g_ring_probe_groups?g_rings.group_collected_events:g_rings.collected_events;
                if(++g_ring_probe_tick>=1200||events>=3){
                    char note[144];snprintf(note,sizeof(note),"%s ticks=%u events=%lu; opening only, not stage completion",
                        events>=3?"PASS":"FAIL",g_ring_probe_tick,(unsigned long)events);
                    boot_log("RING_PROBE",note);g_ring_probe=0;g_paused=1;
                }
            }
            sa3ds_parity_camera_tick(&g_camera,g_player.yaw,
                (g_ec_replay || g_ring_probe || (!modifier && (held&(KEY_L|KEY_R)))) ? 1 : 0);
            if(g_ec_replay) {
                if(g_ec_replay_tick%30==0 || tick_input.jump_pressed) {
                    char line[180];snprintf(line,sizeof(line),
                        "tick=%u in=%.2f,%.2f p=%.3f,%.3f,%.3f v=%.3f,%.3f,%.3f g=%d walls=%lu",
                        g_ec_replay_tick,tick_input.move_x,tick_input.move_z,
                        g_player.pos.x,g_player.pos.y,g_player.pos.z,g_player.vel.x,g_player.vel.y,g_player.vel.z,
                        g_player.grounded,(unsigned long)g_ec_collision.wall_hits);boot_log("REPLAY",line);
                }
                if(++g_ec_replay_tick==(g_wca_replay?SA3DS_WCA_REPLAY_TICKS:600u)){
                    if(g_wca_replay)g_wca_perf_finished=1;
                    trace_stop();g_ec_replay=0;g_wca_replay=0;
                    boot_log("REPLAY","finished; live HID controls resumed");
                }
                if(!g_wca_replay && g_ec_replay_pause && g_ec_replay_tick==270){
                    g_paused=1;boot_log("REPLAY","paused airborne for inspection; SELECT+A resumes");
                }
            }
        }

        if(g_ec_transition_failed)break;
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
        if(probe_log){snprintf(frame_line,sizeof(frame_line),"draw returned stage=%lu sonic=%lu vertices=%lu ringDraws=%u ringVertices=%lu modDraws=%lu modBytes=%lu",(unsigned long)g_ec_gpu.draw_count,(unsigned long)g_sonic_gpu.frame_draws,(unsigned long)g_sonic_gpu.frame_vertices,g_ring_frame_draws,(unsigned long)(g_ring_frame_draws*g_ring_gpu.vertex_count),(unsigned long)g_ec_gpu.override_submitted_draws,(unsigned long)g_ec_gpu.override_texture_bytes);boot_log("FRAME",frame_line);}
        C3D_FrameEnd(0);
        if(probe_log)boot_log("FRAME","end returned");
        g_rendered_frames++;
        if (g_boottrace_first_frame) {
            boot_log("OK","First frame rendered/submitted");
            g_boottrace_first_frame=0;
        }
        g_last_submit_ms=osGetTime()-t0;
        if(g_ec_full_route&&g_ec_perf.frames<300){
            if(!g_ec_perf.frames){g_ec_perf.start_ms=t0;g_ec_perf.min_linear=UINT32_MAX;}
            g_ec_perf.frames++;g_ec_perf.submit_ms+=g_last_submit_ms;
            g_ec_perf.triangles+=g_ec_gpu.submitted_triangles;
            g_ec_perf.draws+=g_ec_gpu.submitted_draws;
            if(g_last_submit_ms>g_ec_perf.max_submit_ms)g_ec_perf.max_submit_ms=(uint32_t)g_last_submit_ms;
            uint32_t free_linear=linearSpaceFree();
            if(free_linear<g_ec_perf.min_linear)g_ec_perf.min_linear=free_linear;
            float usage=C3D_GetCmdBufUsage();if(usage>g_ec_perf.max_cmd)g_ec_perf.max_cmd=usage;
            if(g_ec_perf.frames==300){
                char metrics[320];snprintf(metrics,sizeof(metrics),
                    "EC act=%lu frames=%lu ticks=%lu elapsedMs=%llu submitMs=%llu maxSubmitMs=%lu minLinearBytes=%lu maxCmd=%.6f meanTerrainTriangles=%llu meanTerrainDraws=%llu",
                    (unsigned long)g_ec_act_index,(unsigned long)g_ec_perf.frames,
                    (unsigned long)g_ec_perf.ticks,(unsigned long long)(osGetTime()-g_ec_perf.start_ms),
                    (unsigned long long)g_ec_perf.submit_ms,(unsigned long)g_ec_perf.max_submit_ms,
                    (unsigned long)g_ec_perf.min_linear,g_ec_perf.max_cmd,
                    (unsigned long long)(g_ec_perf.triangles/g_ec_perf.frames),
                    (unsigned long long)(g_ec_perf.draws/g_ec_perf.frames));boot_log("PERF",metrics);
            }
        }
        if(g_wca_perf_active){
            g_wca_perf_frames++;g_wca_perf_submit_ms+=g_last_submit_ms;
            if(g_last_submit_ms>g_wca_perf_max_submit_ms)
                g_wca_perf_max_submit_ms=(unsigned)g_last_submit_ms;
            uint32_t free_linear=linearSpaceFree();
            if(free_linear<g_wca_perf_min_linear)g_wca_perf_min_linear=free_linear;
            float cmd_usage=C3D_GetCmdBufUsage();
            if(cmd_usage>g_wca_perf_max_cmd_usage)g_wca_perf_max_cmd_usage=cmd_usage;
            if(g_wca_perf_finished){
                char metrics[256];uint64_t elapsed=osGetTime()-g_wca_perf_start_ms;
                snprintf(metrics,sizeof(metrics),
                    "WCA ticks=%u renderFrames=%u runtimeElapsedMs=%llu submitTotalMs=%llu submitMaxMs=%u minLinearBytes=%lu maxCmdUsage=%.6f",
                    SA3DS_WCA_REPLAY_TICKS,g_wca_perf_frames,(unsigned long long)elapsed,
                    (unsigned long long)g_wca_perf_submit_ms,g_wca_perf_max_submit_ms,
                    (unsigned long)g_wca_perf_min_linear,g_wca_perf_max_cmd_usage);
                boot_log("PERF",metrics);g_wca_perf_active=0;
            }
        }
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
    return g_ec_transition_failed?1:0;
}
