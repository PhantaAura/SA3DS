from abc import ABC, abstractmethod
from pathlib import Path
from .neutral import NeutralStage
from .profiles import SourceProfile

class SourceAdapter(ABC):
    """Adapters turn a validated source profile into neutral data."""
    profile: SourceProfile

    @abstractmethod
    def validate_source(self, root: Path) -> None:
        """Reject sources that do not match this adapter's evidence profile."""

    @abstractmethod
    def import_stage(self, root: Path, stage_id: str) -> NeutralStage:
        """Import only structures whose semantics have supporting evidence."""

class SelectedModAdapter(SourceAdapter):
    """Marker base for explicitly supported data/mod conversions.

    Windows DLL execution is intentionally outside this architecture.
    """
