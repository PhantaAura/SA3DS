#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../src/game/parity_hud.h"
#include "../src/sa1/physics/movement.h"

int main(void) {
    SA3DSParityCameraState camera;
    sa3ds_parity_camera_init(&camera,0.25f);
    assert(camera.mode == SA3DS_PARITY_CAMERA_FOLLOW);
    sa3ds_parity_camera_tick(&camera,1.25f,0);
    assert(camera.yaw > 0.25f && camera.yaw < 1.25f);
    float followed=camera.yaw;
    sa3ds_parity_camera_toggle(&camera);
    assert(camera.mode == SA3DS_PARITY_CAMERA_REFERENCE);
    sa3ds_parity_camera_tick(&camera,-2.0f,0);
    assert(fabsf(camera.yaw-followed)<1e-6f);
    sa3ds_parity_camera_manual_yaw(&camera,0.1f);
    assert(fabsf(camera.yaw-(followed+0.1f))<1e-6f);
    sa3ds_parity_camera_recenter(&camera,-0.5f);
    assert(fabsf(camera.yaw+0.5f)<1e-6f);
    assert(!strcmp(sa3ds_parity_camera_mode_name(camera.mode),"REFERENCE"));

    SA1PlayerState p;
    memset(&p,0,sizeof(p));
    p.sim_frame=42;
    p.pos=(SA1Vec3){1,2,3};
    p.vel=(SA1Vec3){3,4,4};
    p.yaw=0.75f;
    p.grounded=true;
    p.action_state=SA1_ACTION_GROUNDED;
    p.surface_id=203;
    p.ground_normal=(SA1Vec3){0,1,0};
    p.support_distance=1.0f;
    SA3DSSonicAnimationSelection sel;
    memset(&sel,0,sizeof(sel));
    sel.binding.runtime_id=32;
    sel.binding.action_slot=21;
    sel.sample_frame=7.5f;
    sel.selector_is_provisional=1;
    const SA3DSParityZone *turn=sa3ds_parity_lab_zone_by_id(SA3DS_PARITY_ZONE_TURN);
    SA3DSParityHudSnapshot hud;
    sa3ds_parity_hud_snapshot(&hud,&p,turn,turn,&sel,30,1,99,12,345,1024*1024,camera.mode);
    assert(!strcmp(hud.active_zone,"TURN"));
    assert(hud.sim_frame==42 && hud.runtime_animation_id==32 && hud.action_slot==21);
    assert(hud.trace_recording && hud.trace_frames==99);
    assert(hud.draw_calls==12 && hud.triangles==345 && hud.free_linear_bytes==1024*1024);
    assert(hud.camera_mode==SA3DS_PARITY_CAMERA_REFERENCE);
    assert(hud.selector_is_provisional);
    puts("White Zone parity HUD/camera tests: PASS");
    return 0;
}
