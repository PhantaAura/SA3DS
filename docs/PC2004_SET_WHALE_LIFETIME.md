# WCA70 — SET association and shared whale lifetime

`set_task_lifetime.[ch]` models the pointer-free SET association rules. Source
46BAE0 requires marked/sign-bit entries without active bit0; load filtering,
range and actual allocation are still caller-owned. Successful creation sets
active bit0, increments the byte load counter with wrap, stores task identity
and runtime unload distance(load+100). It does not reset persistent flags.

46C010 and40B6C0 replace main with40B570. They do not free the task inline.
The later scheduled40B570 callback first checks registration via40AFB0 and
invokes any destructor before removing the SET association.40B67F clears entry
+4 and40B685 clears only flag bit0; the load counter and all other flags remain.
The helper preserves those postconditions and exposes deferred free explicitly.
It is not a replacement for the full native allocator/list/child/CCL/resource
cleanup pipeline. Caller must run it at the correct scheduler point and perform
those native operations; its registration argument is a service boundary.

`whale_lifetime.[ch]` models4F88B0's service order: request old-task deletion,
clear shared kill, allocate flags3/list3/constructor5029F0, associate the new
identity, then write its raw spawn position/yaw.40B860 allocates/stores callback
state; it does not execute5029F0 inline. The constructor runs when scheduled.
Shared tip is retained during replacement. Port allocation failure reports an
error after assigning null, rather than inventing a rollback; the original would
dereference a failed allocation. That guard is not a retail failure-parity claim.

4F8910 accepts a kill request only if the shared current task is nonnull,
including one marked for deletion but not destroyed yet.4F8930 only clears the
shared current identity and three tip components if the destroying task still
matches. Thus an older deferred destructor cannot clobber its replacement.
The shared kill word is not cleared by that destructor.5022A0 also always calls
RumbleA(0,0), even if4F8930's identity comparison fails; rumble execution remains
a caller service, not an audio-cleanup assumption.

## Evidence and limits

Required cumulative host and native build PASS. Focused exact-owner verifier
checks11 code spans and relevant calls/flag stores. Tests cover deferred versus
actual free, registration guard, destructor-before-association-clear, preserved
persistent flags, byte counter wrap, later reload, spawn service order, stale
whale destruction, current tip reset, kill gating and explicit allocation error.
No owner payload/pose import changed, so the unchanged pose matrix was not
repeated. Future owner gates now include the static lifetime verifier.

Native consumers are still unconnected; helpers are linker-discarded. Candidate
remains WCA67's471008B SHA256
 a0b63f49de98835e97242f821851fe3ef30caf7be1cd8791cc079db5aed6e7b7.
That image's emulator check still awaits the existing unlock request. Inspected
Desktop preview remains WCA56; failed lighting remains inactive. No retail,
physical Old3DS or movement-freeze acceptance is claimed.

Next: native owned task-list/constructor scheduling and loader binding using
these rules, trigger/contact/history/phase effects, exit-root resolution and
natural spawn/display. Original pier/camera and full chase remain incomplete,
as do full CP1–CP9.
