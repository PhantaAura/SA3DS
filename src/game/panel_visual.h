#ifndef SA3DS_PANEL_VISUAL_H
#define SA3DS_PANEL_VISUAL_H
#include "emerald_coast_visual.h"
#include "../sa3c/sa3c_pose.h"
#define SA3DS_PANEL_VISUAL_MAX_BYTES (1024u*1024u)
typedef struct {SA3DSECVisual components[3];const uint8_t *nodes,*poses;} SA3DSPanelVisual;
/* Borrowed external owner package, fixed exact-profile three-node/eight-frame
 * ACTION. Geometry is uploaded once per component, not once per frame. */
int sa3ds_panel_visual_open(SA3DSPanelVisual *out,const void *data,size_t size);
int sa3ds_panel_visual_pose(const SA3DSPanelVisual *visual,unsigned frame,SA3CMatrix world[3]);
#endif
