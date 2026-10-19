# Pass11B — independent PC2004 source-motion oracle

Pass11A proved that the imported SA3C pose/render traversal reproduces the previously captured Pass10A hardware bounds and that selected `sonic.exe.c` control-flow markers agree with the hierarchy implementation. It still evaluated both sides of the pose check through SA3DS-derived conversion/runtime code.

Pass11B closes that gap for Sonic ACTION slot 1 (Stand):

1. `tools/audit_pass11b_source_pose.py` parses the exact owner-owned original `CHRMODELS_orig.dll` directly. It intentionally imports none of the project CHRMODELS decoder or SA3C writer.
2. It independently resolves the PE export table, all 149 Sonic ACTION pointers, source object topology, bind transforms, ACTION 1 motion records, and raw key arrays.
3. It reconstructs the pointer-free node mapping and checks every SA3C node's child/sibling links and flags against the original DLL.
4. A separately compiled C diagnostic calls the actual `sa3c_pose_sample_action` implementation.
5. The oracle compares all 223 local transforms at negative/clamped, integer, fractional, middle, final, and beyond-final frames. Positions, BAMS rotations, and scales are checked within a float32 tolerance of `4e-5`.

Run with owner files:

```sh
CHRMODELS_PATH="/path/to/system/CHRMODELS_orig.dll" \
SONIC_PVM_PATH="/path/to/system/SONIC.PVM" \
sh tests/run_pass11b_source_pose.sh
```

The runner generates `sonic.sa3c` only in a temporary directory and deletes it on exit. No original or converted game asset is added to the repository or embedded in the code-only 3DS build.
