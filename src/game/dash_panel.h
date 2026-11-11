#ifndef SA3DS_DASH_PANEL_H
#define SA3DS_DASH_PANEL_H
#include "spring_runtime.h"
/* Original task7A4450: last-player lockout is separate from Sonic's signed
 * control timer. Placement lifetime and prior CCL first-hit arbitration are
 * caller-owned. This module does not substitute a distance hit predicate. */
typedef struct {int32_t cooldown;uint8_t initialized,last_player;} SA3DSDashPanel;
typedef struct {float speed;int32_t lockout;int16_t control_timer;} SA3DSDashPanelParameters;
typedef struct {int first_hit_player;int player_ground_status_bit0,skip_update;} SA3DSDashPanelInput;
typedef struct {uint8_t draw,enqueue_collision,boost,player;SA3DSDashPanelParameters parameters;} SA3DSDashPanelResult;
int sa3ds_dash_panel_parameters(const SA3DSMovementPlacement *placement,SA3DSDashPanelParameters *out);
int sa3ds_dash_panel_tick(SA3DSDashPanel *actor,const SA3DSMovementPlacement *placement,
    const SA3DSDashPanelInput *input,SA3DSDashPanelResult *out);
/* Source form7-versus-form6 primary predicate. Panel record has radius10,
 * half-segment4.44 and local-X rotation4000; SET Z/X/Y orients that segment.
 * Player center/radius must come from the queued primary CCL, before boosts. */
int sa3ds_dash_panel_primary_overlap(const SA3DSMovementPlacement *placement,
    const SA3DSSpringTrig *trig,SA1Vec3 player_center,float player_radius);
/* 441490 writes velocity/frame/control timer immediately, retaining position,
 * low-ground status, current action and animation request. Ordinary Sonic only.
 * The following player tick's existing timer filter blocks analog as required. */
int sa3ds_dash_panel_apply(SA1PlayerState *player,const SA3DSMovementPlacement *placement,
    const SA3DSSpringTrig *trig,const SA3DSDashPanelParameters *parameters,float support);
/* O JUMP action2 uses the same player helper with a different local vector. */
int sa3ds_jump_pad_apply(SA1PlayerState *player,const SA3DSMovementPlacement *placement,
    const SA3DSSpringTrig *trig,float support);
#endif
