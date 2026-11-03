# PC2004 shared enemy damage and collision-hit queries

Scope: exact owner executable6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74.
References4CE030,4CDFE0,4C0550,41B9F0,41C6E0/41C750 and787CC0.
Private original code/disassembly remain outside Git. This is a portable source
reimplementation of decisions; native collision production/execution is pending.

## Ordered hit queries

Source enemy damage first asks41CBC0/41C6E0 for a category0 task, then asks
41CBE0/41C750 for category1 only if none was returned. The16-entry hit table
terminates on other-shape byte==-1. Null task/CCL pointers are skipped. Outer
CCL flag0 is not required by these query functions themselves. Resolved category
comes from the current other CCL, not a cached category in the hit record.

41B9F0 shares one cursor across actors and selectors. Changing actor or selector
resets to index0. Repeating the same query begins after the last cached index,
saturated at16. A successful search caches its selected index. A failed scan
leaves the begin index cached, not the final locally scanned index. The port
exposes the reset boundary; native code must call it at the original lifecycle
phase, not silently restart every damage check. No sorting/deduplication occurs.

## Shared damage decisions

Without task status4,4CE030 only tests the current global explosion. Negative
radius disables it.4C0550 stores float coordinate differences;787CC0 stores a
float summed square before square root, then compares magnitude strictly below
radius. Equality is outside. This branch changes no squash, score or hit cursor.

With status4, squash velocity becomes.35 and the helper returns true even when
neither category query succeeds. A category0/1 result sets enemy flag1000 and
requests425C70(10). That score function also maintains the original chain amount,
500 cap and120-tick timer; a native executor must preserve those side effects.

The strong-attribute decision uses CCL+A4's cached other CCL and+A2 record index,
not necessarily the hit returned by the category query. Attribute1000 sets enemy
flag800 and enemy velocityY=.7; it requests no player bounce. Otherwise, cached
other owner task identity selects player0 if equal to the primary task, else1.
A present player with status100, no low ground bits and character0/2/3 gets the
441280 local velocity write(0,2,0), followed by4BCBC0(player,0). That follow-up
is the original optional rumble request, not a homing-action reset. Then enemy
flag400 is cleared. The port emits these requests; it does not yet mutate the
native player, score/chain state or hardware rumble.

Repeated calls retain the shared query cursor and source side effects. Do not
make the helper artificially idempotent once per enemy lifetime.4ACF80's caller
still controls when action12/death is selected and executed.

## Implementation and evidence

src/game/enemy_damage.[ch] uses pointer-free live actor IDs, original-order hits,
current category resolution and a separate cached damage context. Its source
Saru adapter copies real actor status/position and enemy fields through the
shared helper; the world caller must provide genuine prior CCL/cache/player/
explosion state and execute each result in the original phase. No fake native
hit producer or substituted damage rule is supplied.

Focused host checks cover cursor order/reset/failure/sentinel/current-category,
category priority, cached attacker separation, squash/flags/score, normal versus
strong attributes, local bounce gates and strict explosion boundaries. The Saru
actor test also uses the real shared helper at both common-tail damage calls,
retains its cursor/score requests, and reaches the next-tick death request.

Exact-owner verifier checks seven helper body digests and original immediate
operands. This binds the inspected rules to the original profile; it does not
execute retail code or prove x87 retained80 boundary equality. Required host
and staged native build passed. Same381608B/ae538ee9 native artifact; no native
callers yet, so no redundant emulator or unchanged pose/import gate was run.

Remaining: source CCL pair masks/attack fields/hit generation and cached damage
arbitration, actual native player bounce/score/death/effects, projectile behavior,
full task scheduling and real enemy/homing integration. No native combat claim.
