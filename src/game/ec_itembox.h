#ifndef SA3DS_EC_ITEMBOX_H
#define SA3DS_EC_ITEMBOX_H
#include "item_box.h"
#include "owned_task_list.h"
#include "set_object_clip.h"
#include "set_task_lifetime.h"
#include <stddef.h>
/*Emerald Coast O ITEMBOX (list entry13: flags2/3/1, load 160000, main
 *4D6F10). Actions: 0 init (SET flag|0x10; flag bit4 means already broken ->
 *3), 1 appear (grow+=0.11 to 5, not collidable), 2 idle (4D6670 break test
 *on the previous CCL status, then enqueue; icon angle+=3), 3 break (4D6E80:
 *SET flag|4, effect, broken CCL 0x9BF120), 4 broken (enqueue solid CCL).*/
typedef struct {uint32_t act,source_row;uint16_t type;uint8_t kind;int16_t angles[3];float position[3],scale[3];} SA3DSItemBoxPlacement;
typedef struct {const uint8_t *rows,*cci_intact,*cci_broken;uint32_t count,stride;float load_distance_squared;} SA3DSItemBoxAssets;
int sa3ds_itembox_assets_open(SA3DSItemBoxAssets *out,const void *bytes,size_t size);
int sa3ds_itembox_placement(const SA3DSItemBoxAssets *assets,uint32_t index,SA3DSItemBoxPlacement *out);
typedef struct {uint32_t act;unsigned clip_level;SA3DSSetClipInput clip;} SA3DSItemBoxFrame;
/*Previous CCL pass for one box: status bits (1 contact, 4 damaged), the hit
 *partner's player index (-1 none) and this box's cached self shape.*/
typedef struct {uint16_t status;int8_t player,self_shape;uint8_t player_grounded;} SA3DSItemBoxContact;
typedef struct {
 void *context;
 int (*read_frame)(void *context,SA3DSItemBoxFrame *frame);
 int (*contact)(void *context,uint32_t row,SA3DSItemBoxContact *out);
 void (*bounce)(void *context,int player);                     /*441280(p,0,2,0)*/
 void (*award)(void *context,uint32_t row,const SA3DSItemAward *award);
 void (*effect)(void *context,const float position[3]);         /*4D6B60 at y+5*/
 uint16_t (*rand15)(void *context);
} SA3DSItemBoxServices;
typedef struct SA3DSItemBoxRuntime SA3DSItemBoxRuntime;
typedef struct {SA3DSItemBoxRuntime *owner;uint32_t row;SA3DSSetTaskLifetime lifetime;uint8_t action,queued,broken_shapes;float grow,icon_angle;} SA3DSItemBoxEntry;
struct SA3DSItemBoxRuntime {
 const SA3DSItemBoxAssets *assets;SA3DSOwnedTaskList *tasks;SA3DSItemBoxEntry *entries;SA3DSItemBoxServices services;
 uint32_t allocation_failures,initializations,breaks,awards,frees;
};
int sa3ds_itemboxes_init(SA3DSItemBoxRuntime *runtime,const SA3DSItemBoxAssets *assets,SA3DSOwnedTaskList *tasks,
 SA3DSItemBoxEntry *entries,const uint16_t *initial_flags,uint32_t capacity,const SA3DSItemBoxServices *services);
int sa3ds_itemboxes_load(SA3DSItemBoxRuntime *runtime);
#endif
