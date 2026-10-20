#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/game/parity_lab.h"
#include "../src/game/parity_trace.h"
#include "../src/game/sonic_animation_sync.h"
#include "../src/game/sonic_animation_runtime.h"
#include "../src/sa1/physics/physics_constants.h"
#include "../src/sa1/physics/steam_flat.h"
#include "../src/sa1/physics/steam_player_parameters.h"

typedef enum {
    TEST_ACCEL,
    TEST_COAST,
    TEST_BRAKE,
    TEST_TURN_LOW,
    TEST_TURN_HIGH,
    TEST_JUMP_TAP,
    TEST_JUMP_HOLD,
    TEST_SPINDASH,
    TEST_ROLL_EXIT,
    TEST_UPHILL,
    TEST_DOWNHILL,
    TEST_AIR_CONTROL,
    TEST_NEUTRAL,
    TEST_PARTIAL,
    TEST_QUICK_TURN,
    TEST_CAMERA_TURN,
    TEST_SIDE_THRESHOLD
} TestKind;

typedef struct {
    const char *name;
    TestKind kind;
    SA3DSParityZoneId zone;
} ProtocolRow;

static const ProtocolRow kProtocols[] = {
    {"flat_accel", TEST_ACCEL, SA3DS_PARITY_ZONE_ACCELERATION},
    {"coast", TEST_COAST, SA3DS_PARITY_ZONE_BRAKE_COAST},
    {"brake", TEST_BRAKE, SA3DS_PARITY_ZONE_BRAKE_COAST},
    {"turn_low", TEST_TURN_LOW, SA3DS_PARITY_ZONE_TURN},
    {"turn_high", TEST_TURN_HIGH, SA3DS_PARITY_ZONE_TURN},
    {"jump_tap", TEST_JUMP_TAP, SA3DS_PARITY_ZONE_JUMP},
    {"jump_hold", TEST_JUMP_HOLD, SA3DS_PARITY_ZONE_JUMP},
    {"spindash", TEST_SPINDASH, SA3DS_PARITY_ZONE_ROLL_SPINDASH},
    {"roll_exit", TEST_ROLL_EXIT, SA3DS_PARITY_ZONE_ROLL_SPINDASH},
    {"uphill", TEST_UPHILL, SA3DS_PARITY_ZONE_SLOPE_UP},
    {"downhill", TEST_DOWNHILL, SA3DS_PARITY_ZONE_SLOPE_DOWN},
    {"air_control", TEST_AIR_CONTROL, SA3DS_PARITY_ZONE_AIR_LANDING},
    {"neutral", TEST_NEUTRAL, SA3DS_PARITY_ZONE_CALIBRATION},
    {"partial", TEST_PARTIAL, SA3DS_PARITY_ZONE_ACCELERATION},
    {"quick_turn", TEST_QUICK_TURN, SA3DS_PARITY_ZONE_TURN},
    {"camera_turn", TEST_CAMERA_TURN, SA3DS_PARITY_ZONE_TURN},
    {"side_threshold", TEST_SIDE_THRESHOLD, SA3DS_PARITY_ZONE_TURN},
};

static int parse(const char *name, TestKind *kind, SA3DSParityZoneId *zone) {
    for (unsigned i=0;i<sizeof(kProtocols)/sizeof(kProtocols[0]);++i) {
        if (!strcmp(name,kProtocols[i].name)) {
            *kind=kProtocols[i].kind; *zone=kProtocols[i].zone; return 1;
        }
    }
    return 0;
}

static SA1Input input_for(TestKind kind, unsigned frame) {
    SA1Input in={0};
    switch (kind) {
    case TEST_ACCEL:
    case TEST_UPHILL:
    case TEST_DOWNHILL:
        in.move_z=1.0f; break;
    case TEST_COAST:
    case TEST_ROLL_EXIT:
        break;
    case TEST_BRAKE:
        in.move_z=-1.0f; break;
    case TEST_TURN_LOW:
    case TEST_TURN_HIGH:
        in.move_x=1.0f; break;
    case TEST_JUMP_TAP:
        if (frame==0) in.jump_pressed=true;
        if (frame<2) in.jump_held=true;
        if (frame==2) in.jump_released=true;
        break;
    case TEST_JUMP_HOLD:
        if (frame==0) in.jump_pressed=true;
        if (frame<60) in.jump_held=true;
        if (frame==60) in.jump_released=true;
        break;
    case TEST_SPINDASH:
        if (frame==0) in.action_pressed=true;
        if (frame<20) in.action_held=true;
        if (frame==20) in.action_released=true;
        break;
    case TEST_AIR_CONTROL:
        in.move_x=1.0f; in.move_z=0.25f; break;
    case TEST_PARTIAL:
        in.move_z=(3072.0f + 29441.0f * 0.5f) / 32512.0f; break;
    case TEST_QUICK_TURN:
        in.move_x=1.0f; break;
    case TEST_CAMERA_TURN:
        in.move_z=1.0f; break;
    case TEST_NEUTRAL:
    case TEST_SIDE_THRESHOLD:
        break;
    }
    return in;
}

static void apply_precondition(TestKind kind, SA1PlayerState *p) {
    switch (kind) {
    case TEST_COAST: case TEST_BRAKE: p->vel.z=1.5f; break;
    case TEST_TURN_LOW: p->vel.z=0.35f; break;
    case TEST_TURN_HIGH: p->vel.z=3.0f; break;
    case TEST_ROLL_EXIT:
        p->vel.z=0.18f; p->action_state=SA1_ACTION_ROLL; p->ball_form=true; break;
    case TEST_UPHILL: case TEST_DOWNHILL: p->vel.z=1.0f; break;
    case TEST_AIR_CONTROL:
        p->grounded=false; p->action_state=SA1_ACTION_AIRBORNE;
        p->pos.y+=6.0f; p->vel.y=0.0f; p->ball_form=false; break;
    case TEST_SIDE_THRESHOLD: p->vel.x=1.0f; break;
    default: break;
    }
}

static float camera_yaw_for(TestKind kind) {
    return kind == TEST_CAMERA_TURN ? 1.5707963267948966f : 0.0f;
}

int main(int argc, char **argv) {
    if (argc!=4) {
        fprintf(stderr,"usage: %s TEST FRAMES OUT.csv\n",argv[0]);
        return 2;
    }
    TestKind kind; SA3DSParityZoneId zone;
    if (!parse(argv[1],&kind,&zone)) {
        fprintf(stderr,"unknown test: %s\n",argv[1]);
        return 2;
    }
    unsigned frames=(unsigned)strtoul(argv[2],0,10);
    FILE *out=fopen(argv[3],"wb");
    if (!out) { perror(argv[3]); return 2; }

    SA1PlayerState p;
    if (!sa3ds_parity_lab_reset_player(&p,zone)) { fclose(out); return 2; }
    apply_precondition(kind,&p);
    SA3DSSonicAnimationSync sync={0};
    SA3DSSonicAnimationRuntime runtime={0};
    const SA3DSParityZone *z=sa3ds_parity_lab_zone_by_id(zone);

    if (sa3ds_parity_trace_write_header(out)) { fclose(out); return 2; }
    for (unsigned f=0;f<frames;++f) {
        SA1Input in=input_for(kind,f);
        float camera_yaw=camera_yaw_for(kind);
        SA1SteamFlatDebug physics={0};
        sa1_physics_step_steam_sonic_flat(&p,&in,camera_yaw,
            sa3ds_parity_lab_world(),&SA1_PHYSICS_PLACEHOLDER_CONSTANTS,&physics);
        SA1PlayerState physics_done=p;
        SA3DSSonicAnimationObservation observation=
            sa3ds_sonic_animation_observe(&sync,&p,&SA1_STEAM_SONIC_PARAMETER_EVIDENCE);
        if (memcmp(&physics_done,&p,sizeof(p))!=0) {
            fprintf(stderr,"animation observation mutated physics at frame %u\n",f);
            fclose(out); return 3;
        }
        SA3DSSonicAnimationSelection selection=
            sa3ds_sonic_animation_select(&runtime,&p,&observation);
        if (memcmp(&physics_done,&p,sizeof(p))!=0) {
            fprintf(stderr,"animation selection mutated physics at frame %u\n",f);
            fclose(out); return 3;
        }
        if (sa3ds_parity_trace_write_frame(out,zone,z?z->name:argv[1],&p,&in,
                                           camera_yaw,&physics,&selection)) {
            fclose(out); return 2;
        }
    }
    fclose(out);
    printf("White Zone protocol %s: %u frames -> %s\n",argv[1],frames,argv[3]);
    return 0;
}
