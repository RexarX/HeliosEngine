"""Read the repository root VERSION file."""

from __future__ import annotations

import re
from pathlib import Path

_VERSION_RE = re.compile(r"^(\d+\.\d+\.\d+)")


def find_repo_root(start: Path | None = None) -> Path:
    here = (start or Path(__file__).resolve()).parent
    for directory in (here, *here.parents):
        if (directory / "VERSION").is_file():
            return directory
    raise FileNotFoundError("repository root VERSION file not found")


def read_project_version(repo_root: Path | None = None) -> str:
    if repo_root is None:
        try:
            root = find_repo_root()
        except FileNotFoundError:
            # Installed copies of this package do not sit next to VERSION.
            return "0.0.0"
    else:
        root = repo_root
    version_file = root / "VERSION"
    if not version_file.is_file():
        return "0.0.0"
    raw = version_file.read_text(encoding="utf-8").strip()
    match = _VERSION_RE.match(raw)
    return match.group(1) if match else "0.0.0"
