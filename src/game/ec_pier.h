#ifndef SA3DS_EC_PIER_H
#define SA3DS_EC_PIER_H
#include "owned_task_list.h"
#include "set_object_clip.h"
#include "set_task_lifetime.h"
#include <stddef.h>
/*Emerald Coast O PIER (registry59: constructor4FF060, main4FED30, display
 *4FE8E0, destructor4FE970). Intact profile: DynamicCOL quad, display, SET
 *lifetime and the whale-tip break; railing CCI records are exposed for the
 *caller CCL pass. The break throws a standing player0; piece flight and sound
 *are requests only.*/
#define SA3DS_PIER_MAX_POINTS 8u
#define SA3DS_PIER_MAX_INDICES 24u
#define SA3DS_PIER_MAX_PIECES 32u
typedef struct {
 uint32_t act,source_row;uint16_t type;uint8_t variant;int16_t angles[3];float position[3],scale[3];
} SA3DSPierPlacement;
typedef struct {
 const uint8_t *rows,*ccl; /*ccl: 2 variants x 2 raw48-byte railing CCI (form3)*/
 uint32_t count,stride;
 float points[SA3DS_PIER_MAX_POINTS][3];uint16_t indices[SA3DS_PIER_MAX_INDICES];
 uint32_t point_count,index_count,surface_flags,broken_ticks;
 float break_distance_squared,clip_distance_squared,load_distance_squared,stand_radius;
 uint32_t piece_count;float piece_offsets[SA3DS_PIER_MAX_PIECES][3]; /*variant0 objects1..n*/
} SA3DSPierAssets;
/*Borrowed owner package view ("PIER" chunk); rows stay in the package bytes.*/
int sa3ds_pier_assets_open(SA3DSPierAssets *out,const void *bytes,size_t size);
int sa3ds_pier_placement(const SA3DSPierAssets *assets,uint32_t index,SA3DSPierPlacement *out);
typedef struct {
 uint32_t act;unsigned clip_level;SA3DSSetClipInput clip;
 float whale_tip[3]; /*shared stage triple03C5E3E8, zero when no AO task*/
 float whale_movement[2]; /*shared X/Z03C5E3D8/03C5E3E0*/
} SA3DSPierFrame;
typedef struct {
 void *context;
 int (*read_frame)(void *context,SA3DSPierFrame *frame);
 int (*dynamic_add)(void *context,uint32_t task,const SA3DSPierPlacement *placement); /*43B2E0*/
 void (*dynamic_remove)(void *context,uint32_t task); /*43B380*/
 void (*broken)(void *context,uint32_t task,const SA3DSPierPlacement *placement); /*pieces/sound*/
 /*441C30(0,task): player0 grounded and standing on this task's DynamicCOL.*/
 int (*standing)(void *context,uint32_t task);
 /*441280(0,v): world velocity for player0.*/
 void (*throw_player)(void *context,const float velocity[3]);
 double (*sine)(void *context,uint16_t bams);
 uint16_t (*random15)(void *context); /*shared CRT rand()*/
} SA3DSPierServices;
/*4FEBC0 constructor / 4FE9B0 update for one child piece (data1/data2 fields).*/
typedef struct {
 float position[3],velocity[3];int32_t rot_x,rot_y,spin_x,spin_y;
 uint16_t timer;uint8_t action,constructed,displayed;
} SA3DSPierPiece;
typedef struct SA3DSPierRuntime SA3DSPierRuntime;
typedef struct {
 SA3DSPierRuntime *owner;uint32_t row;SA3DSSetTaskLifetime lifetime;
 uint8_t action,displayed,dynamic;uint16_t timer;
 float spread[2]; /*4FEF00: data1 Scale.x/z = 2.9*perp + 1.2*away*/
 uint32_t piece_count;SA3DSPierPiece pieces[SA3DS_PIER_MAX_PIECES];
} SA3DSPierEntry;
struct SA3DSPierRuntime {
 const SA3DSPierAssets *assets;SA3DSOwnedTaskList *tasks;SA3DSPierEntry *entries;SA3DSPierServices services;
 uint32_t allocation_failures,constructs,breaks,frees,throws;
};
int sa3ds_piers_init(SA3DSPierRuntime *runtime,const SA3DSPierAssets *assets,SA3DSOwnedTaskList *tasks,
 SA3DSPierEntry *entries,const uint16_t *initial_flags,uint32_t capacity,const SA3DSPierServices *services);
/*SET loading phase (player0 reference, camera if absent), then the shared list.*/
int sa3ds_piers_load(SA3DSPierRuntime *runtime);
#endif
