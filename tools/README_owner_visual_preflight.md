# Owner-side Sonic visual package preflight (WIP6A)

This is a **read-only** code-only tool. It does not extract or redistribute game files.

Compile on macOS/Linux with a C11 compiler from the repository root:

```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -Isrc/sa3c \
  src/sa3c/sa3c_character.c src/sa3c/sa3t_texture.c \
  src/sa3c/sa3c_gpu_preflight.c tools/check_sonic_visuals.c \
  -lm -o check_sonic_visuals
./check_sonic_visuals /path/to/sonic.sa3c /path/to/sonic.sa3t
```

For ACTIONs, separately run the existing `tools/check_sonic_actions.c` checker.

The visual checker validates original owner-generated package structural compatibility and GPU-read safety: material slots, texture metadata, dimensions, primitive expansion, triangle counts, index ranges and finite source vertex components. It prints alpha statistics. It **does not** verify retail visual appearance, 3DS GPU upload behavior, original action timing or hardware performance.

`sonic.sa3c` and `sonic.sa3t` intentionally contain **different source SHA-256 fields**, because the original sources are CHRMODELS_orig.dll and SONIC.PVM respectively.

Owner-free regression: `bash tests/run_sonic_owner_visual_cli.sh`.
