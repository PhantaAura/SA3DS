#ifndef SA1_STEAM_PLAYER_PARAMETERS_H
#define SA1_STEAM_PLAYER_PARAMETERS_H

/*
 * Evidence record matching the 0x84-byte player parameter copied by the exact
 * supported Steam SADX executable. This is NOT the simplified SA3DS movement
 * constant block and is intentionally not wired into movement.c yet.
 */
typedef struct {
    int jump2_timer;
    float pos_error;
    float lim_h_spd;
    float lim_v_spd;
    float max_x_spd;
    float max_psh_spd;
    float jmp_y_spd;
    float nocon_speed;
    float slide_speed;
    float jog_speed;
    float run_speed;
    float rush_speed;
    float crash_speed;
    float dash_speed;
    float jmp_addit;
    float run_accel;
    float air_accel;
    float slow_down;
    float run_break;
    float air_break;
    float air_resist_air;
    float air_resist;
    float air_resist_y;
    float air_resist_z;
    float grd_frict;
    float grd_frict_z;
    float lim_frict;
    float rat_bound;
    float rad;
    float height;
    float weight;
    float eyes_height;
    float center_height;
} SA1SteamPlayerParameters;

/* Exact Steam table entry 0, validated as Characters_Sonic. */
extern const SA1SteamPlayerParameters SA1_STEAM_SONIC_PARAMETER_EVIDENCE;

#endif
