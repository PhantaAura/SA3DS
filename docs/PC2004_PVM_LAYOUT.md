# PC2004 format9 texture layout correction

The exact owner executable SHA256
6b0937caa0151287c182fcd8cd02b89ea464bd33a13a081c1220fa791f24cd74
dispatches format0x900 through0389D578[1]/038A63AC[1] to78C570.
The callback copies width*height*pixel-size bytes contiguously. PC2004 format9
is linear row-major, with the audited base image first in mip payloads.
The former Morton interpretation scrambled converted terrain and Sonic images.
The Dreamcast format1/2 decoder is separate and retains its X-high Morton layout.

Both PC importers now identify pc2004-pvm9-linear-v1. Original PVM hashes do not
change, so old converted caches must be regenerated; a matching original archive
hash alone cannot identify an up-to-date converted cache. New Sonic build report
schema is v2. Independent RGB/alpha rectangular grids, base-first mip fixtures,
the exact-executable contiguous-copy witness and exact SONIC.PVM decode passed.
Prior converter-versus-converter comparisons do not establish source layout.

Owner-only regenerated assets remain outside this repository. Native build
b06f7ec7053651437da4dbe21bd0c6d67512883fe92d61f89c5688f3c9f7eeb4
rendered corrected terrain, Sonic and camera-anchored background in the isolated
Azahar preview. Geometry/material chunks were byte-identical to their earlier
packages; SET/CAMERA chunks where present were also preserved. This correction
does not establish complete lighting, transparency, water or retail raster parity.
