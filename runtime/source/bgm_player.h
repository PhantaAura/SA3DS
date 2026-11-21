#ifndef SA3DS_BGM_PLAYER_H
#define SA3DS_BGM_PLAYER_H
#include <stdint.h>
typedef struct {int ready,playing;uint32_t frames,loops,underruns,sample;} SA3DSBgmStats;
int sa3ds_bgm_init(void);
int sa3ds_bgm_play(const char *path);
void sa3ds_bgm_update(void);
void sa3ds_bgm_stop(void);
void sa3ds_bgm_exit(void);
void sa3ds_bgm_stats(SA3DSBgmStats *stats);
#endif
