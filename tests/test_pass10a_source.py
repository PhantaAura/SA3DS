from pathlib import Path
base=Path(__file__).resolve().parents[1]
s=(base/'runtime_0_0_9_0G/source/main.c').read_text()
t=(base/'runtime_0_0_9_0G/source/sonic_gpu.c').read_text()
assert 'g_pose_lab=(mode==5);' in s
assert 'if(g_pose_lab) pose_lab_status(); else print_status();' in s
assert 'if (!g_pose_lab && !sa3ds_lab_gpu_draw' in s
assert 'unsigned steps=(g_pose_lab || g_paused) ? 0u' in s
assert 'if (g_pose_lab)' in s and 'sa3c_pose_sample_action(&g_character,slot,g_pose_frame,0,' in s
assert 'sa3ds_pose_metrics(&g_character,root,g_world,g_node_count' in s
assert 'if(g_pose_lab) pose_lab_help(); else print_help();' in s
assert 'if(down&KEY_A){g_pose_bind=!g_pose_bind;' in s
assert 'gspWaitForPPF();' not in t # no redundant wait call
assert 'sa3ds_sonic_gpu_init_traced' in s
print('Pass10A isolated Pose Lab source guard: PASS')
