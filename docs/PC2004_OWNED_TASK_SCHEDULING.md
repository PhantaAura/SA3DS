# WCA71 — owned flat task scheduling and composed whale trigger execution

`owned_task_list.[ch]` provides one bounded caller-owned root-task pool/list.
Source40B030 prepends new tasks.40B0C0 captures the next link before each
callback, so a new head waits for a later traversal; a later existing task's
changed callback is observed when it is reached. A constructor that installs
its main does not run both callbacks in one visit. Source40B570 reads live
links AFTER destructor, which matters when the destructor prepends a task.
Released pool slots enter a LIFO free chain and retain reusable slot identity.
There is no per-update heap allocation or invented generation filtering.

The module reports bounded exhaustion and invalid operations. It supports
current-task release during traversal and deferred replacement of other task
callbacks; immediate release of an upcoming task, recursive root dispatch,
children, global script-origin context and full native resource cleanup are
outside this flat ordinary profile. This is not a complete game-task allocator.

`whale_trigger_runtime.[ch]` now composes the existing owner trigger reader,
SET eligibility/range, clip/contact, lifetime and summon/kill rules with this
scheduler. Services read live frame context for each main callback. The normal
Sonic EC loader selects player0 origin, or camera when player0 is absent,
applies clip-level type filtering, and loads in source row order. Head insertion
therefore reverses task callback order as in the original. Allocation failure
skips the failed SET row with an observable count and leaves it inactive.
Caller supplies actual SET activation/persistent flags; this layer does NOT
mark every entry active or invent restored-entry payloads.

A new trigger runs its constructor only, installs main, then checks contact on
a later callback. Summon action0 arms; action1 executes the summon service on a
subsequent visit without another sphere test. It stores action2 only AFTER the
service returns. An actor allocated into the same list waits for another list
traversal before running its constructor. Kill requests remain repeated. Clip
requests replace main; actual later free clears association/active bit and
preserves SET load count/flags before a later load can recreate the trigger.
Stage-wide clearing, special load modes, restored work and debug editing still
require their native services. Caller keeps buffers/views alive while tasks exist.

## Evidence and limits

Final required cumulative host and native build PASS. Focused exact-owner
verifier checks10 source spans plus head/next-snapshot/live-link/action-store
instructions. Synthetic scheduler checks exercise constructor delay, future
callback replacement, current-task free, destructor head insertion, LIFO reuse
and exhaustion. Composed trigger checks cover constructor/contact/arm/summon
updates, callback observation before action2, shared-list actor-constructor
latency, repeated kill, pending-unload/reload and allocation-failure count.
Unmarked SET fixtures do not load until explicitly activated. Actor callbacks
in that fixture are mocks, not evidence of a running native whale chase.
No owner asset/pose change; the unchanged pose matrix was not repeated.

Native main does not call this scheduler/trigger runtime yet. Code is compiled
but linker-discarded; image remains WCA67's471008B SHA256
 a0b63f49de98835e97242f821851fe3ef30caf7be1cd8791cc079db5aed6e7b7.
That image still awaits the existing Mac-unlock screen check. Inspected Desktop
preview stays WCA56 and failed lighting is inactive. No retail/hardware claim.

Next native binding: source SET activation/stage clearing and owned world list,
resource loader, real whale constructor/phase/CCL/history/effect/draw services
and exit root. Original CCL initializer41CAF0 receives two shapes at10C8790,
count2/category2, then broad radius150 and SECOND-shape attribute0x10
(CCI array+0x34). The actor disable flag must not suppress all CCL. This is
an additional concrete boundary to import before real contact. Pier/camera,
natural encounter and full CP1–CP9 remain incomplete.
