from dataclasses import dataclass
from typing import Optional

@dataclass(frozen=True)
class SourceProfile:
    identifier: str
    executable_name: str
    executable_size: Optional[int]
    executable_sha256: Optional[str]
    notes: str

STEAM_CLEAN = SourceProfile(
    "sadx-steam-clean-e4330c00", "Sonic Adventure DX.exe", 90922000,
    "e4330c00d7ee3910ecc1acab789ffd2a2d571c506984b99a8de4d3663e697fd6",
    "Supported inventory profile; executable addresses remain UNKNOWN.")

PC_2004 = SourceProfile(
    "sadx-pc-2004-unresolved", "sonic.exe", None, None,
    "Separate profile. No hash or addresses are asserted by this checkpoint.")

PROFILES = {profile.identifier: profile for profile in (STEAM_CLEAN, PC_2004)}
