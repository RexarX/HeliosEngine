"""Scaffold a new module directory."""

from __future__ import annotations

from pathlib import Path

from .manifest import NAME_RE
from .version import read_project_version


def scaffold(
    name: str,
    directory: Path,
    *,
    header_only: bool = False,
    c_api: bool = False,
) -> Path:
    if not NAME_RE.match(name):
        raise SystemExit(f"module name '{name}' must match [a-z][a-z0-9_]*")
    root = directory / name
    if root.exists():
        raise SystemExit(f"{root} already exists")
    (root / "include").mkdir(parents=True)
    if not header_only:
        (root / "src").mkdir()
    if c_api:
        (root / "capi" / "include").mkdir(parents=True)
    (root / "module.toml").write_text(
        "\n".join(
            [
                "[module]",
                f'name = "{name}"',
                f'version = "{read_project_version()}"',
                f'description = "{name} module"',
                'languages = ["cpp"]',
                "default = true",
                "implements = []",
                f"c_api = {'true' if c_api else 'false'}",
                "",
                "[build]",
                'system = "cmake"',
                "",
                "[dependencies]",
                "public = []",
                "private = []",
                "",
                "[dependencies.optional]",
                "public = []",
                "private = []",
                "",
            ]
        ).replace("\n\n\n", "\n\n"),
        encoding="utf-8",
    )
    if header_only:
        cmake = "\n".join(
            [
                f"helios_header_only_library({name})",
                "",
                f"target_sources({name}",
                "    INTERFACE FILE_SET HEADERS BASE_DIRS include FILES",
                f"        include/{name}/{name}.hpp",
                ")",
                "",
                "if(HELIOS_CURRENT_MODULE_BUILD_TESTS)",
                f"    helios_add_module_tests({name} SOURCES tests/main.cpp)",
                "endif()",
                "",
            ]
        )
    else:
        cmake = "\n".join(
            [
                f"add_library({name})",
                "",
                f"target_sources({name}",
                "    PRIVATE src/lib.cpp",
                "    PUBLIC FILE_SET HEADERS BASE_DIRS include FILES",
                f"        include/{name}/{name}.hpp",
                ")",
                "",
                f"helios_apply_conventions({name})",
                "",
                "if(HELIOS_CURRENT_MODULE_BUILD_TESTS)",
                f"    helios_add_module_tests({name} SOURCES tests/main.cpp)",
                "endif()",
                "",
            ]
        )
    (root / "CMakeLists.txt").write_text(cmake, encoding="utf-8")
    return root
