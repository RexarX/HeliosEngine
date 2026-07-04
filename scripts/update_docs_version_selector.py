#!/usr/bin/env python3
"""
Generate version_selector.html for multi-edition GitHub Pages documentation.

Enumerates published doc editions on the gh-pages branch (main at site root,
branch previews under branches/, future semver releases at top level) and
writes a <select> element consumed by version_selector_handler.js.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from typing import List, Sequence, Tuple

SEMVER_DIR = re.compile(r"^\d+\.\d+\.\d+")
SKIP_TOP_LEVEL = frozenset({"branches", "search", "docs"})


def git_ls_tree(
    repo_root: Path, ref: str, path: str = "", *, directories_only: bool = False
) -> List[str]:
    cmd = ["git", "ls-tree", "--name-only"]
    if directories_only:
        cmd.append("-d")
    cmd.append(ref)
    if path:
        cmd.append(path if path.endswith("/") else f"{path}/")

    result = subprocess.run(
        cmd, cwd=repo_root, capture_output=True, text=True, check=False
    )
    if result.returncode != 0:
        return []
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def git_has_file(repo_root: Path, ref: str, path: str) -> bool:
    result = subprocess.run(
        ["git", "ls-tree", "--name-only", ref, path],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode == 0 and bool(result.stdout.strip())


def parse_semver(value: str) -> Tuple[int, ...]:
    parts: List[int] = []
    for piece in value.split("."):
        try:
            parts.append(int(piece))
        except ValueError:
            parts.append(0)
    return tuple(parts)


def discover_versions(repo_root: Path, gh_pages_ref: str) -> List[Tuple[str, str]]:
    """Return (path_value, display_label) pairs in selector order."""

    editions: List[Tuple[str, str, Tuple[int, ...]]] = []

    if git_has_file(repo_root, gh_pages_ref, "index.html"):
        editions.append(("main", "main", (0,)))

    for entry in git_ls_tree(repo_root, gh_pages_ref, directories_only=True):
        name = Path(entry).name
        if name in SKIP_TOP_LEVEL or name.startswith("."):
            continue
        if SEMVER_DIR.match(name) and git_has_file(
            repo_root, gh_pages_ref, f"{name}/index.html"
        ):
            editions.append((name, name, (1, *parse_semver(name))))

    for entry in git_ls_tree(
        repo_root, gh_pages_ref, "branches", directories_only=True
    ):
        branch = Path(entry).name
        path_value = f"branches/{branch}"
        if git_has_file(repo_root, gh_pages_ref, f"{path_value}/index.html"):
            editions.append((path_value, branch, (2, 0)))

    editions.sort(key=lambda item: item[2])
    # Semver editions: newest first among release folders.
    semver = [e for e in editions if e[2][0] == 1]
    semver.sort(key=lambda item: item[2][1:], reverse=True)
    ordered = [e for e in editions if e[2][0] == 0]
    ordered.extend(semver)
    ordered.extend(
        sorted((e for e in editions if e[2][0] == 2), key=lambda item: item[1])
    )
    return [(value, label) for value, label, _ in ordered]


def render_selector(options: Sequence[Tuple[str, str]]) -> str:
    lines = ['<select id="versionSelector" title="Documentation edition">']
    for value, label in options:
        lines.append(f'  <option value="{value}">{label}</option>')
    lines.append("</select>")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate version_selector.html for published documentation editions"
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Path to write version_selector.html",
    )
    parser.add_argument(
        "--gh-pages-ref",
        default="origin/gh-pages",
        help="Git ref for the gh-pages branch (default: origin/gh-pages)",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=None,
        help="Repository root (default: parent of scripts/)",
    )
    args = parser.parse_args()

    repo_root = (args.repo_root or Path(__file__).resolve().parent.parent).resolve()
    subprocess.run(
        ["git", "fetch", "origin", "gh-pages"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )

    options = discover_versions(repo_root, args.gh_pages_ref)
    if not options:
        print(
            "Warning: no documentation editions found on gh-pages; defaulting to main",
            file=sys.stderr,
        )
        options = [("main", "main")]

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_selector(options), encoding="utf-8")
    print(f"Wrote {len(options)} edition(s) to {args.output}")
    for value, label in options:
        print(f"  - {label} ({value})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
