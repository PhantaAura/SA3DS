#ifndef SA3DS_PLAYER_DAMAGE_H
#define SA3DS_PLAYER_DAMAGE_H
#include "../sa1/physics/player_state.h"
#include "../sa1/physics/pc2004_motion.h"
/*Pointer-free view of the source task/player work used by496F50/492D70.
 * Caller supplies current source animation request, not an inferred pose.*/
typedef struct {
    uint8_t action,task_byte2,player_index,next_command;
    uint16_t status,timer,flags4,flags6,animation;
    uint32_t flags5c;
    int16_t rings;
    SA1PC2004LocalVector speed;
    SA1Vec3 position,collision_up;
    int32_t angles[3];
} SA3DSPlayerDamageState;
typedef struct {
    int alternate_character,stage_id,game_mode,death_command_allowed;
    void *context;uint16_t (*random15)(void *context);
} SA3DSPlayerDamageWorld;
typedef struct {
    uint8_t accepted,checked,rumble,shield_consumed,release_held;
    uint8_t shader_changed,shader_index,water_effect,release_light_dash;
    uint8_t death_requested,scatter_count,reset_rings;
    uint16_t sound_ids[2];uint8_t sound_channels[2],sound_count; /*0=423D70,1=425710*/
    int32_t scatter_angles[20];
    SA1Vec3 scatter_origin;
} SA3DSPlayerDamageResult;
/*496F50 preamble,492D70 branch results,4506F0 ring-loss requests. Full source
 * action branches are represented as state/effects; caller executes actual
 * audio, scattering, death, water and carried-object systems. No fake executor.*/
int sa3ds_player_damage_tick(SA3DSPlayerDamageState *state,
    const SA3DSPlayerDamageWorld *world,const SA1SteamPlayerParameters *parameters,
    SA3DSPlayerDamageResult *out);
typedef struct {uint8_t resume,begin_jump,idle_selector,ground_selector,write_animation;} SA3DSPlayerHurtResult;
/*Ordinary source action16 after command processing. Carried-object status800
 * is outside this selector profile and returns0 unchanged.*/
int sa3ds_player_hurt_select(SA3DSPlayerDamageState *state,int analog_active,
    int jump_pressed,SA3DSPlayerHurtResult *out);
/*4948C0 suppresses display when task timer bit1 is set.*/
int sa3ds_player_damage_visible(uint16_t timer);
#endif
