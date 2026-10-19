#!/usr/bin/env python3
"""Source-only C string/delimiter guard for 0G native main.c.

Catches the malformed printf merge encountered on real macOS devkitARM
(MacFix2) before user spends time compiling the ARM binary.
This does NOT substitute for a real devkitARM build.
"""
from pathlib import Path
import re

MAIN = Path(__file__).resolve().parents[1] / "runtime_0_0_9_0G/source/main.c"
s = MAIN.read_text(encoding="utf-8")
assert 'printf("\\x1b[27;1HAssets:%s C:%luK T:%luK ",' in s, "assets HUD printf missing or corrupted"
assert 'printf("\\x1b[28;1H60Hz:%llu Drop:%llu %s ",' in s, "tick HUD printf missing or corrupted"
assert ");1HAssets" not in s, "merged printf HUD statements remain corrupted"
assert 'g_paused ? "PAUSED" : "RUN");' in s, "pause status HUD missing"

# Scan C string and character literals for erroneous raw newlines.
# Ignore C/C++ comments, preserving line tracking. Parentheses/braces/brackets
# must remain balanced inside the source, ignoring all literal content.
i, line = 0, 1
state = 'code'
opens = []
while i < len(s):
    a = s[i]
    b = s[i+1] if i+1 < len(s) else ''
    if state == 'code':
        if a == '/' and b == '/': state = 'line_comment'; i += 2; continue
        if a == '/' and b == '*': state = 'block_comment'; i += 2; continue
        if a == '"': state = 'string'; literal_start = line
        elif a == "'": state = 'char'; literal_start = line
        elif a in '({[': opens.append((a, line))
        elif a in ')}]':
            assert opens, f'unmatched {a!r} at line {line}'
            prev, prev_line = opens.pop()
            assert {'(':')','{':'}','[':']'}[prev] == a, f'mismatched brackets at lines {prev_line} and {line}'
    elif state == 'line_comment':
        if a == '\n': state = 'code'
    elif state == 'block_comment':
        if a == '*' and b == '/': state = 'code'; i += 2; continue
    elif state in ('string','char'):
        if a == '\\':
            assert b != '', f'trailing escape at EOF line {line}'
            if b == '\n': line += 1
            i += 2; continue
        if a == '\n': raise AssertionError(f'unterminated {state} started line {literal_start}')
        if a == ('"' if state == 'string' else "'"): state = 'code'
    if a == '\n': line += 1
    i += 1
assert state in ('code','line_comment'), f'unterminated {state} at EOF'
assert not opens, f'unclosed C delimiter {opens[-1]}'
print('0G native main.c lexical/HUD source gate: PASS')
