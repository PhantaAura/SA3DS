# CHECKPOINT SA3DS 0.0.9.1D — CAM Bank Structural Decode

**Input:** Verified 0.0.9.1C source-only archive from the preceding checkpoint. No code-rewrite or physics modifications. All existing renderer and physics source remains intact.

**Verified:** 127 owner source CAM files, 4,955 records, source length formula `64 + count * 64` for all 127, 127/127 matching SHA-256 across two owner system archives, 326 Emerald Coast CAM records across 3 acts. The new pointer-free neutral `SA3DCAM` v1.0 format preserves original 64-byte rows, file identity and original hash. Zero-record source files and opaque non-finite patterns are valid. Source-only assets remain excluded from redistribution.

**New:** CAM decoder, batch importer, optional Emerald Coast `CAMERA` chunk, adversarial tests and source evidence docs. The owner-supplied Universal Modder resource-family findings are cataloged as a separate unverified-reference note.

**Tests:** inherited 0.0.9.1C end-to-end suite PASS; 0.0.9.1D synthetic and exact owner-source CAM tests PASS; owner-side EC package generation PASS for 3 acts containing all six chunks; 127/127 CAM files match across both archives.

**Not proven:** camera mode/trigger/controller semantic mappings; Old 3DS runtime execution of camera data; Old 3DS hardware test; global retail physics 1:1. No new `.3dsx` built in this checkpoint. `PHYSICS_1TO1 = false`.

**Next:** research actual SADX camera function/trigger semantics before native runtime; continue the independent White Zone Sonic hardware gate when power/charger allows. In parallel, 0.0.9.1E whole-game registry can use the now decoded geometry/SET/CAM filename groups.
