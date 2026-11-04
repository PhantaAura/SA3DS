#ifndef SA3DS_WHALE_MOTION_PHASE_H
#define SA3DS_WHALE_MOTION_PHASE_H
#include <stdint.h>
/*5029F0 constructor /5022C0 action dispatch. Common tracking, tip/CCL,
 *effects, audio, camera execution and display remain caller services.*/
typedef struct {
 float position[3],heading[3],dive_velocity[3];
 float frame,frame_step,draw_y,phase_time,curve,curve_base,tracking_speed;
 int32_t yaw,roll,base_yaw,yaw_step,roll_step;
 uint16_t counter;uint8_t action,cycle;
} SA3DSWhaleMotion;
typedef double (*SA3DSWhaleSine)(void *context,uint16_t bams);
typedef struct {void *context;SA3DSWhaleSine sine;} SA3DSWhaleMotionServices;
/*5022C0's far deletion reads Camera_Data1 at03B2CBB0, independently of
 *the buffered player position used later for chase-speed control.*/
typedef struct {float camera_position[3];uint32_t kill_request;} SA3DSWhaleMotionInput;
typedef struct {
 uint8_t destroy,stop_common_tail,clear_collision_disable,allow_free_camera;
 uint8_t surface_splash,cycle_splash,rumble,sound2f;
} SA3DSWhaleMotionResult;
int sa3ds_whale_motion_init(SA3DSWhaleMotion *motion,const float position[3],int32_t yaw,
 const SA3DSWhaleMotionServices *services);
/*Phase only: call once, then original common movement/CCL/display tail.*/
int sa3ds_whale_motion_phase(SA3DSWhaleMotion *motion,const SA3DSWhaleMotionInput *input,
 const SA3DSWhaleMotionServices *services,SA3DSWhaleMotionResult *out);
#endif
