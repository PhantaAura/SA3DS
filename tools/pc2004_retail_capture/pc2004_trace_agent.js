'use strict';

/* Exact SADX PC2004 US profile only. This agent reads state and emits one row
 * after Sonic_Main has completed movement and animation dispatch. It never
 * writes game memory or calls game functions. */
const IMAGE_BASE = 0x00400000;
const RVA_INPUT_FILTER = 0x0000f170;
const RVA_SONIC_ACT1 = 0x00096f50;
const RVA_SONIC_MAIN = 0x0009a9b0;
const RVA_PERIPHERAL_PTRS = 0x0370e77c;
const RVA_FILTERED_ANGLE = 0x0370e368;
const RVA_FILTERED_MAGNITUDE = 0x0370e36c;
const RVA_CAMERA_PTR = 0x0372cbb0;

const module = Process.getModuleByName('sonic.exe');
const base = module.base;
function at(rva) { return base.add(rva); }
function s16(p,o) { return p.add(o).readS16(); }
function u16(p,o) { return p.add(o).readU16(); }
function u32(p,o) { return p.add(o).readU32(); }
function f32(p,o) { return p.add(o).readFloat(); }
function bamsRadians(v) {
  let s = v & 0xffff;
  if (s >= 0x8000) s -= 0x10000;
  return s * (Math.PI * 2.0 / 65536.0);
}
function logicalAxis(raw) { return raw >= 0 ? raw / 127.0 : raw / 128.0; }

let latestInput = null;
let frame = 0;
const act1Sequence = new Map();

Interceptor.attach(at(RVA_INPUT_FILTER), {
  onLeave() {
    try {
      const player = 0;
      const peripheral = at(RVA_PERIPHERAL_PTRS + player * 4).readPointer();
      const rawX = s16(peripheral, 0x1c);
      const rawY = s16(peripheral, 0x1e);
      const camera = at(RVA_CAMERA_PTR).readPointer();
      latestInput = {
        raw_x: rawX,
        raw_y: rawY,
        move_x: logicalAxis(rawX),
        move_z: logicalAxis(rawY),
        filtered_input_angle_bams: u16(at(RVA_FILTERED_ANGLE + player * 8), 0),
        filtered_input_magnitude: f32(at(RVA_FILTERED_MAGNITUDE + player * 8), 0),
        camera_yaw: camera.isNull() ? 0.0 : bamsRadians(u32(camera, 0x18))
      };
    } catch (e) {
      send({kind:'capture_error', stage:'input', message:String(e)});
    }
  }
});

Interceptor.attach(at(RVA_SONIC_ACT1), {
  onEnter() {
    const tid = Process.getCurrentThreadId();
    act1Sequence.set(tid, (act1Sequence.get(tid) || 0) + 1);
  }
});

Interceptor.attach(at(RVA_SONIC_MAIN), {
  onEnter(args) {
    this.tid = Process.getCurrentThreadId();
    this.act1Before = act1Sequence.get(this.tid) || 0;
    this.task = args[0];
    try {
      this.entity = this.task.add(0x20).readPointer();
      this.motion = this.task.add(0x24).readPointer();
      this.player = this.motion.isNull() ? ptr(0) : this.motion.readPointer();
    } catch (e) {
      this.entity = ptr(0); this.motion = ptr(0); this.player = ptr(0);
      send({kind:'capture_error', stage:'sonic_main_enter', message:String(e)});
    }
  },
  onLeave() {
    try {
      const e=this.entity, m=this.motion, p=this.player;
      if (e.isNull() || m.isNull() || p.isNull()) return;
      const input=latestInput || {
        raw_x:0,raw_y:0,move_x:0,move_z:0,
        filtered_input_angle_bams:0,filtered_input_magnitude:0,camera_yaw:0
      };
      const status=u16(e,0x04);
      const transitionState=s16(p,0x120);
      const requestedAnimation=s16(p,0x124);
      const transitionSource=s16(p,0x126);
      send({kind:'row', row:{
        trace_version:3,
        tick_phase:'post_sonic_main',
        sonic_act1_ran:(act1Sequence.get(this.tid) || 0) > this.act1Before ? 1 : 0,
        frame:frame++,
        move_x:input.move_x,
        move_z:input.move_z,
        filtered_input_magnitude:input.filtered_input_magnitude,
        filtered_input_angle_bams:input.filtered_input_angle_bams,
        camera_yaw:input.camera_yaw,
        circle_raw_x:input.raw_x,
        circle_raw_y:input.raw_y,
        pos_x:f32(e,0x20), pos_y:f32(e,0x24), pos_z:f32(e,0x28),
        vel_x:f32(m,0x04), vel_y:f32(m,0x08), vel_z:f32(m,0x0c),
        local_forward_speed:f32(p,0x38), local_side_speed:f32(p,0x40),
        heading:bamsRadians(u32(e,0x18)),
        grounded:(status & 3) !== 0 ? 1 : 0,
        action_state:e.readU8(),
        status_bits:status,
        player_status_bits:u32(p,0x5c),
        accel_forward:f32(p,0x2c), accel_normal:f32(p,0x30), accel_side:f32(p,0x34),
        spindash_charge:f32(p,0x00), spindash_ticks:u16(p,0x7e),
        animation_requested_id:requestedAnimation,
        runtime_animation_id:transitionState === 2 ? transitionSource : requestedAnimation,
        animation_frame:f32(p,0x130),
        animation_transition_state:transitionState,
        animation_transition_source_id:transitionSource
      }});
    } catch (e) {
      send({kind:'capture_error', stage:'sonic_main_leave', message:String(e)});
    }
  }
});

send({kind:'ready', module_base:base.toString(), preferred_image_base:'0x00400000',
      module_size:module.size, pid:Process.id, tick_phase:'post_sonic_main'});
