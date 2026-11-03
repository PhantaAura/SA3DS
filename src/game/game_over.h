#ifndef SA3DS_GAME_OVER_H
#define SA3DS_GAME_OVER_H
#include <stdint.h>
/*416610 states18/19. Only the ordinary action-stage menu, not Trial menus.*/
typedef struct {int32_t timer;uint8_t initialized,selection,pulse,finished;} SA3DSGameOver;
typedef struct {uint8_t initialize,draw,cleanup,retry,quit,reset_lives,release_textures;uint16_t music;} SA3DSGameOverResult;
int sa3ds_game_over_tick(SA3DSGameOver *menu,uint32_t pressed,SA3DSGameOverResult *out);
typedef struct {uint8_t texture;float x,y,z,scale_x,scale_y;uint32_t argb;} SA3DSGameOverSprite;
/*42BFD0's nine draw calls in source640x480 coordinates. Updates pulse only
 *when drawn. Caller preserves source reciprocal-depth ordering and alpha.*/
int sa3ds_game_over_sprites(SA3DSGameOver *menu,SA3DSGameOverSprite out[9]);
#endif
