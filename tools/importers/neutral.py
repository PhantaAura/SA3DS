from dataclasses import dataclass, field
from typing import Dict, List, Optional

@dataclass
class NeutralChunk:
    kind: str
    payload: bytes
    provenance: str
    evidence: List[str] = field(default_factory=list)

@dataclass
class NeutralStage:
    source_profile: str
    geometry: Optional[NeutralChunk] = None
    collision: Optional[NeutralChunk] = None
    materials: Optional[NeutralChunk] = None
    textures: Optional[NeutralChunk] = None
    object_placement: Optional[NeutralChunk] = None
    camera: Optional[NeutralChunk] = None
    audio: Optional[NeutralChunk] = None
    metadata: Dict[str, str] = field(default_factory=dict)

    def require_evidence(self):
        for chunk in (self.geometry, self.collision, self.materials, self.textures,
                      self.object_placement, self.camera, self.audio):
            if chunk is not None and not chunk.evidence:
                raise ValueError("neutral chunk lacks evidence: " + chunk.kind)
