"""Compose an installed SDK manifest and pkg-config files."""

from __future__ import annotations

from pathlib import Path

import tomllib

from .version import read_project_version


class SdkError(Exception):
    pass


_CHECKED_CONFIGS = frozenset({"Debug", "RelWithDebInfo"})


def asserts_enabled(config: str) -> bool:
    """API checks are compiled into Debug and RelWithDebInfo libraries."""
    return config in _CHECKED_CONFIGS


def _pc_name(module: str) -> str:
    return (
        module.replace("helios_", "helios_", 1)
        if module.startswith("helios_")
        else module
    )


def write_sdk(
    *,
    graph: dict,
    artifacts: Path,
    prefix: Path,
    pkgconfig_dir: Path,
    share_dir: Path,
    include_dir: Path | None = None,
    cmake_dir: Path | None = None,
) -> None:
    if not artifacts.is_file():
        raise SdkError(
            f"missing {artifacts.name}. Run cmake --build for this configuration "
            "before cmake --install."
        )
    data = tomllib.loads(artifacts.read_text(encoding="utf-8"))
    libraries = data.get("libraries", {})
    if not isinstance(libraries, dict) or not libraries:
        raise SdkError(f"{artifacts} has no [libraries] table")

    lib_dir = prefix / "lib"
    bin_dir = prefix / "bin"
    for name, spec in libraries.items():
        file_name = spec.get("file_name", "")
        if not file_name:
            raise SdkError(f"library {name} has no file_name in {artifacts.name}")
        candidates = [lib_dir / file_name, bin_dir / file_name]
        if not any(path.is_file() for path in candidates):
            raise SdkError(
                f"library '{file_name}' for {name} was not installed. "
                "Run cmake --build for this configuration before cmake --install."
            )

    enabled = {
        module["name"]: module for module in graph["modules"] if module.get("enabled")
    }
    share_dir.mkdir(parents=True, exist_ok=True)
    pkgconfig_dir.mkdir(parents=True, exist_ok=True)

    sdk = {
        "version": data.get("version", read_project_version()),
        "config": data.get("config", ""),
        "asserts": asserts_enabled(str(data.get("config", ""))),
        "compiler_id": data.get("compiler_id", ""),
        "compiler_version": data.get("compiler_version", ""),
        "cxx_standard": data.get("cxx_standard", "23"),
        "stdlib": data.get("stdlib", ""),
        "modules": [
            enabled[name] for name in graph.get("enabled", []) if name in enabled
        ],
        "libraries": libraries,
    }
    _write_toml(share_dir / "helios_sdk.toml", sdk)
    _write_checks(
        sdk,
        include_dir or (prefix / "include" / "helios"),
        cmake_dir or (prefix / "lib" / "cmake" / "Helios"),
    )
    _write_pc(
        pkgconfig_dir / "helios.pc",
        "helios",
        "Helios engine",
        sdk,
        list(enabled),
        enabled,
    )
    for name, module in enabled.items():
        _write_pc(
            pkgconfig_dir / f"{_pc_name(name)}.pc",
            _pc_name(name),
            module.get("description", name),
            sdk,
            [name],
            enabled,
            requires=module.get("dependencies", {}).get("public", []),
        )


def _check_targets(sdk: dict) -> list[str]:
    targets: list[str] = []
    for module in sdk["modules"]:
        alias = module.get("alias")
        if isinstance(alias, str) and alias and alias not in targets:
            targets.append(alias)
    if "helios" in sdk["libraries"] and "helios::helios" not in targets:
        targets.append("helios::helios")
    return targets


def _write_checks(sdk: dict, include_dir: Path, cmake_dir: Path) -> None:
    include_dir.mkdir(parents=True, exist_ok=True)
    cmake_dir.mkdir(parents=True, exist_ok=True)
    if sdk["asserts"]:
        header = (
            "#pragma once\n"
            "// This Helios install was built with API checks.\n"
            "#ifndef HELIOS_ENABLE_ASSERTS\n"
            "#define HELIOS_ENABLE_ASSERTS 1\n"
            "#endif\n"
            "#ifndef HELIOS_ENABLE_STACKTRACE\n"
            "#define HELIOS_ENABLE_STACKTRACE 1\n"
            "#endif\n"
        )
    else:
        header = (
            "#pragma once\n"
            "// This Helios install was built without API checks.\n"
            "// Define HELIOS_ENABLE_ASSERTS before including Helios headers to\n"
            "// check inline API misuse. Checks inside this library stay off.\n"
        )
    (include_dir / "sdk_checks.hpp").write_text(header, encoding="utf-8")

    lines = [
        "# Generated for this Helios install. Do not edit.",
        f"set(HELIOS_SDK_ASSERTS {'TRUE' if sdk['asserts'] else 'FALSE'})",
    ]
    if sdk["asserts"]:
        for target in _check_targets(sdk):
            lines.append(f'if(TARGET "{target}")')
            lines.append(
                f'  target_compile_definitions("{target}" INTERFACE'
                " HELIOS_ENABLE_ASSERTS HELIOS_ENABLE_STACKTRACE)"
            )
            lines.append("endif()")
    (cmake_dir / "HeliosSdkChecks.cmake").write_text(
        "\n".join(lines) + "\n", encoding="utf-8"
    )


def _write_pc(
    path: Path,
    name: str,
    description: str,
    sdk: dict,
    libs: list[str],
    enabled: dict,
    requires: list[str] | None = None,
) -> None:
    libraries = sdk["libraries"]
    link_names = []
    for lib in libs:
        spec = libraries.get(lib, {})
        file_name = spec.get("file_name", lib)
        stem = file_name
        for suffix in (".lib", ".a", ".dll", ".so", ".dylib"):
            if stem.endswith(suffix):
                stem = stem[: -len(suffix)]
        if stem.startswith("lib"):
            stem = stem[3:]
        link_names.append(f"-l{stem}")
    reqs = []
    for dep in requires or []:
        if dep in enabled:
            reqs.append(_pc_name(dep))
    cflags = "-I${prefix}/include"
    if sdk["asserts"]:
        cflags += " -DHELIOS_ENABLE_ASSERTS -DHELIOS_ENABLE_STACKTRACE"
    lines = [
        f"Name: {name}",
        f"Description: {description}",
        f"Version: {sdk['version']}",
        f"Cflags: {cflags}",
        "Libs: -L${prefix}/lib " + " ".join(link_names),
    ]
    if reqs:
        lines.append("Requires: " + " ".join(reqs))
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def _toml_string(value: str) -> str:
    escaped = value.replace("\\", "\\\\").replace('"', '\\"')
    return f'"{escaped}"'


def _write_toml(path: Path, sdk: dict) -> None:
    lines = [
        f"version = {_toml_string(str(sdk['version']))}",
        f"config = {_toml_string(str(sdk['config']))}",
        f"asserts = {'true' if sdk['asserts'] else 'false'}",
        f"compiler_id = {_toml_string(str(sdk['compiler_id']))}",
        f"compiler_version = {_toml_string(str(sdk['compiler_version']))}",
        f"cxx_standard = {_toml_string(str(sdk['cxx_standard']))}",
        f"stdlib = {_toml_string(str(sdk['stdlib']))}",
        "",
    ]
    for module in sdk["modules"]:
        lines.append("[[modules]]")
        lines.append(f"name = {_toml_string(module['name'])}")
        lines.append(f"version = {_toml_string(module['version'])}")
        lines.append(f"description = {_toml_string(module.get('description', ''))}")
        langs = ", ".join(_toml_string(lang) for lang in module.get("languages", []))
        lines.append(f"languages = [{langs}]")
        if module.get("alias"):
            lines.append(f"alias = {_toml_string(module['alias'])}")
        lines.append(f"c_api = {'true' if module.get('c_api') else 'false'}")
        lines.append("")
    for name, spec in sdk["libraries"].items():
        lines.append(f"[libraries.{name}]")
        for key, value in spec.items():
            if isinstance(value, bool):
                lines.append(f"{key} = {'true' if value else 'false'}")
            else:
                lines.append(f"{key} = {_toml_string(str(value))}")
        lines.append("")
    path.write_text("\n".join(lines), encoding="utf-8")
