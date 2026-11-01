#include "wca_replay.h"
#define PI 3.14159265358979323846f

static void jump(SA1Input *in,unsigned tick,unsigned start,unsigned duration) {
    in->jump_pressed=tick==start;
    in->jump_held=tick>=start && tick<start+duration;
    in->jump_released=tick==start+duration;
}
static void charge(SA1Input *in,unsigned tick,unsigned start,unsigned duration) {
    in->action_pressed=tick==start;
    in->action_held=tick>=start && tick<start+duration;
    in->action_released=tick==start+duration;
}
SA3DSWCAReplayStep sa3ds_wca_replay_step(unsigned tick) {
    SA3DSWCAReplayStep out={0};
    if(tick>=SA3DS_WCA_REPLAY_TICKS){out.finished=1;return out;}
    if(tick<SA3DS_WCA_DIRECTION_CASES*SA3DS_WCA_DIRECTION_TICKS) {
        static const int dirs[9][2]={{0,0},{0,1},{1,1},{1,0},{1,-1},
            {0,-1},{-1,-1},{-1,0},{-1,1}};
        out.case_id=tick/SA3DS_WCA_DIRECTION_TICKS;
        out.case_tick=tick%SA3DS_WCA_DIRECTION_TICKS;
        unsigned strength=out.case_id%2, dir=(out.case_id/2)%9, cam=out.case_id/18;
        float scale=strength?1.0f:0.5f;
        /* Circular full deflection: diagonals are unit-length, not sqrt(2).
         * This is deterministic screen input, not measured physical ADC data. */
        if(dirs[dir][0] && dirs[dir][1])scale*=0.7071067811865475244f;
        out.screen_input.move_x=dirs[dir][0]*scale;
        out.screen_input.move_z=dirs[dir][1]*scale;
        out.camera_yaw=cam*PI/4;
        out.name="DIRECTION";
    } else {
        static const char *names[SA3DS_WCA_TRANSITION_CASES]={"STAND_TURN",
            "RUN_TURN","RUN_REVERSE","CAMERA_ROTATE","AIR_STEER",
            "CHARGE_RELEASE","ROLL_UNCURL","JUMP_LAND"};
        unsigned t=tick-SA3DS_WCA_DIRECTION_CASES*SA3DS_WCA_DIRECTION_TICKS;
        unsigned scenario=t/SA3DS_WCA_TRANSITION_TICKS;
        out.case_id=SA3DS_WCA_DIRECTION_CASES+scenario;
        out.case_tick=t%SA3DS_WCA_TRANSITION_TICKS;
        out.name=names[scenario];
        SA1Input *in=&out.screen_input;unsigned local=out.case_tick;
        switch(scenario) {
        case 0:
            if(local>=16 && local<80)in->move_x=1;
            if(local>=80 && local<160)in->move_z=1;
            if(local>=160 && local<260)in->move_z=-1;
            break;
        case 1:
            if(local<100)in->move_z=1;
            else if(local<220)in->move_x=1;
            break;
        case 2:
            if(local<140)in->move_z=1;
            else if(local<300)in->move_z=-1;
            break;
        case 3:
            if(local<270)in->move_z=1;
            out.camera_yaw=(local/90)*PI/2;
            break;
        case 4:
            if(local<70)in->move_z=1;
            else if(local<100)in->move_x=1;
            else if(local<200)in->move_z=1;
            jump(in,local,60,8);
            if(local>=70)out.camera_yaw=PI/2;
            break;
        case 5:
            if(local<30)in->move_z=0.5f;
            charge(in,local,40,40);
            if(local>=80 && local<180)in->move_z=1;
            break;
        case 6:
            charge(in,local,40,20);
            break;
        case 7:
            jump(in,local,40,8);
            break;
        }
    }
    out.reset=out.case_tick==0;
    return out;
}
