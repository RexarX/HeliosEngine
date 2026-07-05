#!/usr/bin/env python3
"""
Generate selector data for multi-edition GitHub Pages documentation.

Published docs use three locations:
  - main branch snapshot at the site root
  - release tags at top-level semver directories, e.g. 0.2.0/
  - branch previews under branches/<branch>/
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path
from typing import Iterable, List, Sequence, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parent))

from edition_selector import (
    SEMVER_DIR,
    normalize_docs_label,
    read_project_version,
    render_branch_selector_html,
    render_version_selector_html,
    resolve_current_branch,
    resolve_current_label,
    resolve_current_version,
    write_selector_js,
)

SKIP_TOP_LEVEL = frozenset({"branches", "search", "docs"})


def parse_semver(value: str) -> Tuple[int, ...]:
    version = value.split("+", 1)[0].split("-", 1)[0]
    parts: List[int] = []
    for piece in version.split("."):
        try:
            parts.append(int(piece))
        except ValueError:
            parts.append(0)
    return tuple(parts)


def _is_missing_gh_pages_ref(message: str) -> bool:
    lowered = message.lower()
    return (
        "couldn't find remote ref" in lowered
        or "not found in upstream origin" in lowered
    )


def fetch_gh_pages(repo_root: Path) -> bool:
    result = subprocess.run(
        ["git", "fetch", "origin", "gh-pages"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode == 0:
        return True

    message = result.stderr.strip() or result.stdout.strip()
    if _is_missing_gh_pages_ref(message):
        return False

    raise RuntimeError(f"failed to fetch origin/gh-pages: {message}")


def git_published_paths(repo_root: Path, ref: str) -> List[str]:
    result = subprocess.run(
        ["git", "ls-tree", "-r", "--name-only", ref],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        message = result.stderr.strip() or result.stdout.strip()
        raise RuntimeError(f"failed to inspect {ref}: {message}")
    return [line.strip().replace("\\", "/") for line in result.stdout.splitlines()]


def filesystem_published_paths(pages_root: Path) -> List[str]:
    return [
        path.relative_to(pages_root).as_posix()
        for path in pages_root.rglob("*")
        if path.is_file()
    ]


def branch_label_from_value(value: str) -> str:
    return value.removeprefix("branches/")


def discover_branch_options(
    published_paths: Sequence[str], current_branch: str
) -> List[Tuple[str, str]]:
    options: List[Tuple[str, str]] = []
    seen = set()

    def add(value: str, label: str) -> None:
        if value not in seen:
            options.append((value, label))
            seen.add(value)

    if "index.html" in published_paths or current_branch == "main":
        add("main", "main")

    for path in published_paths:
        if not path.startswith("branches/") or not path.endswith("/index.html"):
            continue
        branch = path[len("branches/") : -len("/index.html")]
        if branch:
            add(f"branches/{branch}", branch)

    if current_branch != "main":
        add(current_branch, branch_label_from_value(current_branch))

    return options


def discover_version_options(
    repo_root: Path, published_paths: Sequence[str], current_version: str
) -> List[Tuple[str, str]]:
    project_version = read_project_version(repo_root)
    versions: List[Tuple[str, str, Tuple[int, ...]]] = []
    seen = set()

    def add(version: str) -> None:
        if version not in seen:
            versions.append((version, version, parse_semver(version)))
            seen.add(version)

    add(project_version)
    add(current_version)

    for path in published_paths:
        if "/" not in path:
            continue
        name, rest = path.split("/", 1)
        if rest != "index.html":
            continue
        if name in SKIP_TOP_LEVEL or name.startswith("."):
            continue
        if SEMVER_DIR.fullmatch(name):
            add(name)

    versions.sort(key=lambda item: item[2], reverse=True)
    return [(value, label) for value, label, _ in versions]


def build_selector_html(
    repo_root: Path, published_paths: Sequence[str], current_label: str
) -> Tuple[str, str, str]:
    current_label = normalize_docs_label(current_label)
    current_branch = resolve_current_branch(current_label)
    current_version = resolve_current_version(repo_root, current_label)
    branch_options = discover_branch_options(published_paths, current_branch)
    version_options = discover_version_options(
        repo_root, published_paths, current_version
    )
    branch_html = render_branch_selector_html(branch_options, current_branch)
    version_html = render_version_selector_html(version_options, current_version)
    return branch_html, version_html, current_version


def write_selector_data(
    repo_root: Path,
    published_paths: Sequence[str],
    current_label: str,
    output: Path,
) -> None:
    branch_html, version_html, default_version = build_selector_html(
        repo_root, published_paths, current_label
    )
    write_selector_js(output, branch_html, version_html, default_version)
    print(f"Wrote selector data to {output}")


def discover_published_editions(pages_root: Path) -> List[Tuple[str, Path]]:
    editions: List[Tuple[str, Path]] = []

    if (pages_root / "index.html").is_file():
        editions.append(("main", pages_root / "edition_selector_data.js"))

    for child in sorted(path for path in pages_root.iterdir() if path.is_dir()):
        if child.name in SKIP_TOP_LEVEL or child.name.startswith("."):
            continue
        if SEMVER_DIR.fullmatch(child.name) and (child / "index.html").is_file():
            editions.append((child.name, child / "edition_selector_data.js"))

    branches_root = pages_root / "branches"
    if branches_root.is_dir():
        for index in sorted(branches_root.rglob("index.html")):
            branch = index.parent.relative_to(branches_root).as_posix()
            if branch:
                editions.append((branch, index.parent / "edition_selector_data.js"))

    return editions


def refresh_all_editions(repo_root: Path, pages_root: Path) -> None:
    published_paths = filesystem_published_paths(pages_root)
    editions = discover_published_editions(pages_root)
    if not editions:
        raise RuntimeError(f"no published documentation editions found in {pages_root}")

    for label, output in editions:
        write_selector_data(repo_root, published_paths, label, output)

    print(f"Refreshed {len(editions)} documentation edition selector(s)")


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Generate edition selector data for published documentation"
    )
    parser.add_argument(
        "--js-output",
        type=Path,
        help="Path to write edition_selector_data.js for the current build",
    )
    parser.add_argument(
        "--refresh-pages-root",
        type=Path,
        help="Checked-out gh-pages root; rewrites selector data for every edition",
    )
    parser.add_argument(
        "--current-label",
        default=None,
        help="Edition label to mark selected (default: DOCS_LABEL or main)",
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
    args = parser.parse_args(list(argv) if argv is not None else None)

    if not args.js_output and not args.refresh_pages_root:
        parser.error("At least one of --js-output or --refresh-pages-root is required")

    repo_root = (args.repo_root or Path(__file__).resolve().parent.parent).resolve()
    current_label = normalize_docs_label(args.current_label or resolve_current_label())

    try:
        if args.js_output:
            if fetch_gh_pages(repo_root):
                published_paths = git_published_paths(repo_root, args.gh_pages_ref)
            else:
                published_paths = []
            write_selector_data(
                repo_root, published_paths, current_label, args.js_output
            )

        if args.refresh_pages_root:
            pages_root = args.refresh_pages_root.resolve()
            refresh_all_editions(repo_root, pages_root)
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
