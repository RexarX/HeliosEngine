"""Shared helpers for documentation edition selector assets."""

from __future__ import annotations

import html
import json
import os
import re
from pathlib import Path
from typing import Optional, Sequence, Tuple

SEMVER_DIR = re.compile(r"^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$")


def read_project_version(repo_root: Path) -> str:
    cmake_file = repo_root / "CMakeLists.txt"
    text = cmake_file.read_text(encoding="utf-8")
    match = re.search(
        r"project\s*\(\s*HeliosEngine\s+VERSION\s+([\d.]+)",
        text,
        re.IGNORECASE | re.DOTALL,
    )
    return match.group(1) if match else "0.0.0"


def normalize_docs_label(label: str) -> str:
    label = label.strip()
    if label.startswith("refs/heads/"):
        label = label.removeprefix("refs/heads/")
    elif label.startswith("refs/tags/"):
        label = label.removeprefix("refs/tags/")
    if label.startswith("v") and SEMVER_DIR.fullmatch(label[1:]):
        label = label[1:]
    return label


def is_version_label(label: str) -> bool:
    return bool(SEMVER_DIR.fullmatch(normalize_docs_label(label)))


def resolve_current_label() -> str:
    label = os.environ.get("DOCS_LABEL", "").strip()
    return normalize_docs_label(label) if label else "main"


def resolve_current_branch(label: Optional[str] = None) -> str:
    label = (
        normalize_docs_label(label) if label is not None else resolve_current_label()
    )
    if not label or label in ("main", "local") or is_version_label(label):
        return "main"
    if label.startswith("branches/"):
        return label
    return f"branches/{label}"


def resolve_current_version(repo_root: Path, label: Optional[str] = None) -> str:
    label = (
        normalize_docs_label(label) if label is not None else resolve_current_label()
    )
    if is_version_label(label):
        return label
    return read_project_version(repo_root)


def render_selector_html(
    select_id: str,
    title: str,
    options: Sequence[Tuple[str, str]],
    selected: Optional[str] = None,
) -> str:
    if selected is None and options:
        selected = options[0][0]
    lines = [
        f'<select id="{html.escape(select_id, quote=True)}" '
        f'title="{html.escape(title, quote=True)}">'
    ]
    for value, label in options:
        selected_attr = " selected" if value == selected else ""
        escaped_value = html.escape(value, quote=True)
        escaped_label = html.escape(label, quote=False)
        lines.append(
            f'  <option value="{escaped_value}"{selected_attr}>{escaped_label}</option>'
        )
    lines.append("</select>")
    return "\n".join(lines)


def render_branch_selector_html(
    options: Sequence[Tuple[str, str]],
    selected: Optional[str] = None,
) -> str:
    return render_selector_html(
        "branchSelector", "Documentation branch", options, selected
    )


def render_version_selector_html(
    options: Sequence[Tuple[str, str]],
    selected: Optional[str] = None,
) -> str:
    return render_selector_html(
        "versionSelector", "Documentation version", options, selected
    )


def write_selector_js(
    path: Path,
    branch_html: str,
    version_html: str,
    default_version: str,
) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        f"window.__HELIOS_DEFAULT_VERSION__ = {json.dumps(default_version)};\n"
        f"window.__HELIOS_BRANCH_SELECTOR_HTML__ = {json.dumps(branch_html.strip())};\n"
        f"window.__HELIOS_VERSION_SELECTOR_HTML__ = {json.dumps(version_html.strip())};\n",
        encoding="utf-8",
    )
