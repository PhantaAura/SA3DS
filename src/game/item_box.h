#ifndef SA3DS_ITEM_BOX_H
#define SA3DS_ITEM_BOX_H
#include <stdint.h>
/*PC2004 O ITEMBOX (4D6F10, object list entry 13). Kind = trunc(SET scale.x):
 *negative -> 8, above 8 -> 0. Handlers 0x9BF194[kind]: 2 AddRings(5),
 *3 AddRings(10), 4 random rings {1,5,10,15,20,40}[|trunc(rand/32768*-6)|]
 *(4D6CD0, table 0x7E6FE4), 6 AddLives(1); every handler plays sound 0xB and,
 *for character 6, also 0x51A. Kinds 0 speed shoes, 1 invincibility,
 *5 barrier, 7 bomb, 8 magnetic barrier are not ported (unported=1).*/
typedef enum {SA3DS_ITEM_SPEED=0,SA3DS_ITEM_INVINCIBLE,SA3DS_ITEM_RINGS5,SA3DS_ITEM_RINGS10,SA3DS_ITEM_RINGS_RANDOM,
 SA3DS_ITEM_BARRIER,SA3DS_ITEM_LIFE,SA3DS_ITEM_BOMB,SA3DS_ITEM_MAGNETIC} SA3DSItemKind;
typedef struct {int32_t rings,lives;uint16_t sounds[2];uint8_t sound_count,unported,icon;} SA3DSItemAward;
int sa3ds_item_box_kind(float scale_x);
/*rand15 is the shared MSVC rand() (0..32767), consumed only by kind4.*/
int sa3ds_item_box_award(int kind,uint8_t character,uint16_t (*rand15)(void *context),void *context,SA3DSItemAward *out);
#endif
