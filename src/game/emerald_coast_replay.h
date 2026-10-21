#ifndef SA3DS_EMERALD_COAST_REPLAY_H
#define SA3DS_EMERALD_COAST_REPLAY_H
#include "../sa1/physics/input.h"
/* Optional developer input fixture, NOT retail inputs or movement equations.
 * Fed into the existing 60 Hz controller only when the SD replay flag exists.
 * Neutral, partial/full forward, turn, coast, jump, reverse toward hotel. */
static inline SA1Input sa3ds_ec_replay_input(unsigned tick) {
    SA1Input in={0};
    if(tick>=60 && tick<120)in.move_z=0.5f;
    if(tick>=120 && tick<150)in.move_z=1;
    if(tick>=150 && tick<180){in.move_x=0.5f;in.move_z=0.5f;}
    in.jump_pressed=tick==240;
    in.jump_held=tick>=240 && tick<248;
    in.jump_released=tick==248;
    if(tick>=360 && tick<540)in.move_z=-1;
    return in;
}
#endif
