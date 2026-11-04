# WCA61 — original whale constructor and action phases

The portable constructor and action dispatch behind AO callback5022C0 are now
implemented. They are not connected to native world tasks yet. Common player
tracking, the moving collision point, effects/audio/camera execution, shape
animation and drawing remain incomplete; this is not native whale acceptance.

The saved owner decompile omits this function. A bounded private Ghidra pass
reused the existing no-analysis project, disassembled5022C0..5029CF and seeded
all five destinations from the actual jump table at5029D0. Its output and the
original assembly stay outside Git. The portable implementation is an
independent named-state reconstruction, not redistributed owner decompiler code.
Nine exact owner spans now include the main dispatcher and its jump table.

The constructor seeds action1/frame8/step.3, draw offset-100, rise10, curve
base100/curvature-1/time-10, counter18, yaw step80 and roll step72. It records
the original heading before adding yaw0xC7F. Roll truncates the source float
33.333332 times-72 to-2399, not rounded-2400. Its initial horizontal offset uses
the source bend angle,1.8 factor and10-unit multiplier. Sine is an explicit
caller service for the owner's Ninja lookup table; libm sin is not silently
substituted by the phase implementation.

Action1 rises and clears collision-disable on reaching offset0, then enters4.
Actions3/4/5 implement the original frame/curve/yaw/roll cycle and emit the
original splash/rumble/sound requests at their source boundaries. Action5 wraps
at stored frame30, reverses both angular steps, increments the byte cycle and
resets its counter. Only a shared kill request equal to1 at that boundary
selects action2, with source heading-derived dive velocity and vertical-2.5.
Action2 performs its phase displacement and source .4 frame advance, requests
free-camera enable, and marks deletion at draw offset<=-350. This deletion
still permits the original common tail. The separate >1000000 squared-distance
check marks deletion and stops the tail immediately.

Floating stores follow the observed boundaries: action3 compares its unrounded
frame sum; action4/5 compare the stored float; action2 tests its unrounded sum
before deciding whether to subtract30. Phase3/5 explicitly store local phase
time before the parabola, whereas phase4 retains the intermediate through the
parabola before storing it. C double intermediates retain these important
boundaries, but complete x87 extended-precision/retail numerical identity is
NOT proven by the focused checks.

## Evidence and remaining connection

Focused phase checks cover the ten-update emergence, constructor truncation,
7.7+.3 stored-frame versus comparison distinction,17.8+.2 transition,
29.6+.4 wrap, phase5 reset and angular reversal, exact kill-request selection,
<=-350 deletion and strict1000-unit far boundary. Controlled sine values test
service use; they are not replacement gameplay data. Owner trigger package
remains unchanged720B/acb1f158 and uninstalled. No importer/pose data changed;
the focused nine-span owner builder was rerun, without repeating the unchanged
pose matrix.

The original display502210 selects different skeletal/shape motion pairs for
exit action2 versus the ordinary cycle. Shape motion type30 is a vertex/normal
animation boundary not handled by the existing skeletal SA3C path. It must be
imported and sampled faithfully, not replaced by the ambient DOLPHIN mesh or a
rigid rotating whale. Source tip/CCL calculation also interpolates a30-key pitch
track before common movement; this is not the same as the visible body roll.
Next work must implement that common tracking/tip/CCL order, owner shape data
and native executor, then connect WCA60 triggers and camera/pier behavior.

Required host and native build verification is recorded in CURRENT_CHECKPOINT.
Full CP6, lighting raster, retail/full-route and physical Old3DS gates remain open.

WCA69 correction: the far-distance input is Camera_Data1(03B2CBB0), not
player task origin. The field is renamed camera_position. Buffered player
tracking remains a distinct common-tail service; see PC2004_SET_OBJECT_CLIP.md.
