#ifndef SA3DS_CHECKPOINT_ASSETS_H
#define SA3DS_CHECKPOINT_ASSETS_H
#include "emerald_coast_visual.h"
#include "movement_objects.h"
#include "ccl_contact.h"
#include "checkpoint_task.h"
#include "sa3c_pose.h"
#define SA3DS_CHECKPOINT_MAX_BYTES (4u*1024u*1024u)
typedef struct {
    SA3DSECVisual components[5]; /*base,inactive R/L,active R/L*/
    SA3DSMovementObjects placements;
    const uint8_t *collider,*player_colliders,*quarter_lut;
    SA3DSCCLRules rules[2];
    /*Legacy package field name; these are inert child offsets, not land meshes.*/
    float node_positions[3][3],land_offsets[2][3],load_squared,unload_squared;
} SA3DSCheckpointAssets;
/*Version2 retains the source CCL_Init category4 rules (v1 used enemy3).
 * Borrowed immutable owner-generated package; bytes must outlive all views.*/
int sa3ds_checkpoint_assets_open(SA3DSCheckpointAssets *out,const void *bytes,size_t size);
/*44EEF0 +44EFE0 callbacks: parent T/Rz/Rx/Ry; each post T/Ry(+/-)/Rx.
 * Ignores the post's default NJS rotation/UNIT bits exactly as its callback.*/
int sa3ds_checkpoint_pose(const SA3DSCheckpointAssets *assets,
    const SA3DSMovementPlacement *placement,const SA3DSCheckpointResult *draw,
    SA3CMatrix world[3]);
#endif
