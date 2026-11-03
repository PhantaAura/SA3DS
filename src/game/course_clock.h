#ifndef SA3DS_COURSE_CLOCK_H
#define SA3DS_COURSE_CLOCK_H
#include <stdint.h>
/*Normal Sonic426070 branch. Clock bytes are distinct from4261B0's global
 * source-frame counter. Call after world tasks, once per source world update.*/
typedef struct {uint8_t time[3],enabled;uint32_t updates;} SA3DSCourseClock;
void sa3ds_course_clock_reset(SA3DSCourseClock *clock);
void sa3ds_course_clock_tick(SA3DSCourseClock *clock);
#endif
