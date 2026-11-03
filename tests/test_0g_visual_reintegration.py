#!/usr/bin/env python3
"""0G native candidate must inherit WIP1-6 safeguards without losing 0G HUD/camera/trace.

This is a source integration contract, NOT a native compile or GPU hardware test.
Functional behavior of shared helpers is tested by the existing cumulative C suites.
"""
from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
f=root/'runtime_0_0_9_0F'
g=root/'runtime_0_0_9_0G'
# TextureSyncFix intentionally changes the texture upload/init sections only.
# Retain exact 0F lab submission/cleanup outside the CP1 camera correction.
f_lab=(f/'source/lab_gpu.c').read_text()
g_lab=(g/'source/lab_gpu.c').read_text().replace('#include "player_view.h"\n','')
old_camera=f_lab[f_lab.index('static void camera_relative('):f_lab.index('static void write_vertex(')]
new_camera=g_lab[g_lab.index('static void camera_relative('):g_lab.index('static void write_vertex(')]
assert 'sa3ds_view_point(' in new_camera
assert g_lab.replace(new_camera,old_camera)==f_lab, 'lab submission/cleanup changed outside camera conversion'
f_gpu=(f/'source/sonic_gpu.c').read_text()
g_gpu=(g/'source/sonic_gpu.c').read_text()
# WCA56 permits the exact-owner781CC0/784850 constant diffuse context.
# Normalize only that explicit addition, then retain the entire old draw guard.
mesh=g_gpu[g_gpu.index('static void xform('):g_gpu.index('int sa3ds_sonic_gpu_init_instances_traced(')]
constant='    if(r->constant_diffuse_enabled)return sa3ds_visual_material_rgba(r->constant_diffuse_argb);\n'
assert mesh.count(constant)==1
mesh=mesh.replace(constant,'',1).replace(
    'static uint32_t material_rgba(const SA3DSSonicGpu *r, uint16_t material_index) {\n    const SA3CCharacter *c=r->character;',
    'static uint32_t material_rgba(const SA3CCharacter *c, uint16_t material_index) {',1).replace(
    'material_rgba(r,cmd->material_index)','material_rgba(c,cmd->material_index)',1)
assert f_gpu[f_gpu.index('static void xform('):f_gpu.index('int sa3ds_sonic_gpu_init(')] == mesh, 'mesh draw changed outside owner constant-diffuse context'
assert f_gpu[f_gpu.index('void sa3ds_sonic_gpu_destroy('):f_gpu.index('int sa3ds_sonic_gpu_draw(')] == \
       g_gpu[g_gpu.index('void sa3ds_sonic_gpu_destroy('):g_gpu.index('void sa3ds_sonic_gpu_begin_frame(')], 'original GPU cleanup changed'
# Shared multi-actor append is now explicit; the legacy wrapper must still
# reset once then emit through the same original pose/render callback.
wrapper=g_gpu[g_gpu.index('int sa3ds_sonic_gpu_draw('):]
assert 'sa3ds_sonic_gpu_begin_frame(r)' in wrapper
assert 'return sa3ds_sonic_gpu_draw_append(r,root,world,count,stats)' in wrapper
assert 'sa3c_render_emit(r->character,root_node,world,world_count,draw_command,&ctx,&st)' in g_gpu
assert 'gspWaitForPPF();' not in g_gpu
assert 'C3D_SyncDisplayTransfer(' in g_gpu and 'upload_rgba8_cpu(' in g_gpu
assert 'sa3ds_sonic_gpu_init_traced(' in g_gpu
assert 'sa3ds_sonic_gpu_init(' in g_gpu
f_header=(f/'include/sonic_gpu.h').read_text()
g_header=(g/'include/sonic_gpu.h').read_text()
assert 'int sa3ds_sonic_gpu_init(SA3DSSonicGpu *r,' in f_header
assert 'int sa3ds_sonic_gpu_init(SA3DSSonicGpu *r,' in g_header
assert 'int sa3ds_sonic_gpu_init_traced(' in g_header

s=(g/'source/main.c').read_text()
assert s.count('g_sonic_gpu.constant_diffuse_enabled=1')==1
assert 'if(g_ec_combined){\n            g_sonic_gpu.constant_diffuse_argb=0xffffffffu;g_sonic_gpu.constant_diffuse_enabled=1;' in s

make=(g/'Makefile').read_text()
for token in (
    '#include "sa3c_gpu_preflight.h"',
    '#include "../../src/game/sonic_action_preflight.h"',
    'sa3ds_visual_preflight(&g_character,&g_texture_archive,&g_visual_report)',
    'sa3ds_geometry_preflight(&g_character,&g_geometry_report)',
    'sa3ds_action_preflight(',
    'sa3ds_sonic_animation_binding_at(',
    'sa3ds_gpu_batch_result_name(g_sonic_gpu.last_batch_error)',
    'g_visual_report.fully_transparent_texels',
    'g_action_report.failing_key_index',
    'sa3ds_parity_hud_snapshot(',
    'sa3ds_parity_trace_write_frame(',
    'sa3ds_parity_camera_toggle(',
    'sa3ds_parity_camera_recenter(',
    'sa3ds_parity_lab_reset_player(',
    'sa3ds_parity_lab_world()',
    'sa1_physics_step_steam_sonic_flat(',
    'memcmp(&physics_done,&g_player,sizeof(g_player))',
):
    assert token in s, f'0G lost essential native integration: {token}'
# Ensure load-time guards precede GPU texture initialization.
load=s.index('static int load_owner_packages(')
visual=s.index('sa3ds_visual_preflight(&g_character',load)
geometry=s.index('sa3ds_geometry_preflight(&g_character',load)
action=s.index('sa3ds_action_preflight(',load)
init=s.index('static int init_scene(',load)
assert load < visual < geometry < action < init
assert 'sa3ds_sonic_gpu_init_traced(' in s[init:]
# Check reset refresh occurs inside reset function, not just once after initial load.
reset=re.search(r'static void reset_selected_zone\(void\)\s*\{(.*?)\n\}',s,re.S)
assert reset, 'reset function missing'
assert 'sa3ds_sonic_animation_observe(' in reset.group(1)
assert 'sa3ds_sonic_animation_select(' in reset.group(1)
assert 'sa3ds_parity_camera_recenter(' in reset.group(1)
# Guard against accidentally discarding 0G's developer-only HUD/camera/trace features.
assert 'src/game' in make and '../src/sa3c' in make
assert 'sa3ds_parity_camera_tick(' in s
assert 'if (g_trace_file)' in s
print('0G native visual reintegration: PASS (WIP1-6 guards + 0G parity HUD/camera/trace)')
