#ifndef SA3DS_SE_PLAYER_H
#define SA3DS_SE_PLAYER_H
#include <stdint.h>
/*Needs ndsp already initialized (sa3ds_bgm_init).*/
int sa3ds_se_init(const char *dir);
int sa3ds_se_play(uint32_t id);
void sa3ds_se_exit(void);
void sa3ds_se_stats(uint32_t *played,uint32_t *missing,uint32_t *truncated);
#endif
