#ifndef SA3DS_EC_CCUBE_H
#define SA3DS_EC_CCUBE_H
#include "owned_task_list.h"
#include "set_object_clip.h"
#include "set_task_lifetime.h"
#include <stddef.h>
/*Emerald Coast C CUBE (registry24, main4D47E0): invisible form3 category4
 *box whose half-dimensions are SET scale+10 (4D46C0). Main runs46C360 clip,
 *then action0 initializes CCL and becomes1; action1 enqueues every update.*/
typedef struct {uint32_t act,source_row;uint16_t type;int16_t angles[3];float position[3],scale[3];} SA3DSCubePlacement;
typedef struct {const uint8_t *rows,*cci;uint32_t count,stride;float load_distance_squared;} SA3DSCubeAssets;
int sa3ds_cube_assets_open(SA3DSCubeAssets *out,const void *bytes,size_t size);
int sa3ds_cube_placement(const SA3DSCubeAssets *assets,uint32_t index,SA3DSCubePlacement *out);
/*4D46C0: float scale+10 per axis (raw; may be negative).*/
void sa3ds_cube_dimensions(const SA3DSCubePlacement *placement,float out[3]);
typedef struct {uint32_t act;unsigned clip_level;SA3DSSetClipInput clip;} SA3DSCubeFrame;
typedef struct {void *context;int (*read_frame)(void *context,SA3DSCubeFrame *frame);} SA3DSCubeServices;
typedef struct SA3DSCubeRuntime SA3DSCubeRuntime;
typedef struct {SA3DSCubeRuntime *owner;uint32_t row;SA3DSSetTaskLifetime lifetime;uint8_t action,queued;} SA3DSCubeEntry;
struct SA3DSCubeRuntime {
 const SA3DSCubeAssets *assets;SA3DSOwnedTaskList *tasks;SA3DSCubeEntry *entries;SA3DSCubeServices services;
 uint32_t allocation_failures,initializations,frees;
};
int sa3ds_cubes_init(SA3DSCubeRuntime *runtime,const SA3DSCubeAssets *assets,SA3DSOwnedTaskList *tasks,
 SA3DSCubeEntry *entries,const uint16_t *initial_flags,uint32_t capacity,const SA3DSCubeServices *services);
int sa3ds_cubes_load(SA3DSCubeRuntime *runtime);
#endif
