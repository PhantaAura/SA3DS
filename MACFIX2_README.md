# SA3DS 0.0.9.1E-WIP9 MacFix2 — native build syntax hotfix

**Parent:** MacFix1 ZIP, SHA-256 `4e0dbe43f180048f79e90f24f585dfa12e63fdc5b1c8541fabfa1edf422f55ab`.

Your Mac reached the REAL 3DS compiler after the host suite passed. It failed at native runtime `main.c` lines 176-180 because HUD status lines for `Assets:` and `60Hz/Drop/PAUSED` were accidentally interleaved. This update fixes only these HUD `printf` statements and adds a lexical source regression test into the cumulative host test runner. It retains the earlier MacFix1 `mkdtemp` portability fix and ALL accumulated White Zone source.

The native source-level syntax bug has been fixed and host tests pass. **No `.3dsx` or real-hardware boot is claimed** without running devkitARM on your Mac.

## macOS

Unzip `SA3DS_MacFix2_AllInOne.zip`, open the extracted folder in Terminal, and run:

```sh
bash SA3DS_WIP9_MacFix2_MacBuild.sh
```

See `RUN12_OLD3DS_TEST.md` after a successful native compile. Do not distribute user-owned Sega data.
