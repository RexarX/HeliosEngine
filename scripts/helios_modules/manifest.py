"""Load and validate module.toml manifests."""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path

import tomllib

NAME_RE = re.compile(r"^[a-z][a-z0-9_]*$")
SEMVER_RE = re.compile(r"^\d+\.\d+\.\d+$")
FEATURE_RE = re.compile(r"^[a-z][a-z0-9_]*$")
LANGUAGES = {"c", "cpp"}

MODULE_KEYS = {
    "name",
    "version",
    "description",
    "languages",
    "default",
    "implements",
    "c_api",
}
BUILD_KEYS = {"system"}
DEP_KEYS = {"public", "private", "optional"}
VIS_KEYS = {"public", "private"}
FEATURE_KEYS = {"default", "abi", "description"}


class ManifestError(Exception):
    def __init__(self, path: Path | None, message: str) -> None:
        self.path = path
        super().__init__(f"{path}: {message}" if path else message)


@dataclass
class Feature:
    name: str
    default: bool = False
    abi: bool = False
    description: str = ""

    @property
    def option(self) -> str:
        return ""  # filled once the module name is known


@dataclass
class Module:
    name: str
    path: Path
    version: str
    description: str
    languages: list[str]
    default: bool
    implements: list[str]
    c_api: bool
    system: str
    target: str
    alias: str | None
    deps_public: list[str]
    deps_private: list[str]
    opt_public: list[str]
    opt_private: list[str]
    features: list[Feature]
    origin: str = "engine"
    fixed: bool = False
    enabled: bool = False
    backend_wins: list[str] = field(default_factory=list)

    def option_name(self) -> str:
        return "HELIOS_BUILD_" + self.name.upper()

    def feature_option(self, feature: Feature) -> str:
        return self.name.upper() + "_" + feature.name.upper()

    def available_def(self) -> str:
        return self.target.upper() + "_AVAILABLE"

    def required(self) -> list[str]:
        return list(dict.fromkeys(self.deps_public + self.deps_private))

    def to_json(self) -> dict:
        return {
            "name": self.name,
            "version": self.version,
            "description": self.description,
            "languages": self.languages,
            "alias": self.alias,
            "target": self.target,
            "path": str(self.path),
            "origin": self.origin,
            "fixed": self.fixed,
            "enabled": self.enabled,
            "c_api": self.c_api,
            "implements": self.implements,
            "backend_wins": self.backend_wins,
            "dependencies": {
                "public": self.deps_public,
                "private": self.deps_private,
            },
            "optional": {
                "public": self.opt_public,
                "private": self.opt_private,
            },
            "features": [
                {
                    "name": feature.name,
                    "option": self.feature_option(feature),
                    "default": feature.default,
                    "abi": feature.abi,
                    "description": feature.description,
                }
                for feature in self.features
            ],
            "available_def": self.available_def(),
        }


def default_alias(name: str) -> str | None:
    if "_" not in name:
        return None
    namespace, rest = name.split("_", 1)
    rest = rest.replace("_", "::")
    return f"{namespace}::{rest}"


def _require_table(data: dict, key: str, path: Path) -> dict:
    value = data.get(key, {})
    if not isinstance(value, dict):
        raise ManifestError(path, f"[{key}] must be a table")
    return value


def _string_list(value: object, path: Path, label: str) -> list[str]:
    if value is None:
        return []
    if not isinstance(value, list) or not all(isinstance(item, str) for item in value):
        raise ManifestError(path, f"{label} must be an array of strings")
    return list(value)


def _unknown(keys: set[str], allowed: set[str], path: Path, label: str) -> None:
    extra = keys - allowed
    if extra:
        raise ManifestError(path, f"unknown {label} keys: {', '.join(sorted(extra))}")


def load_manifest(path: Path, *, origin: str) -> Module:
    try:
        data = tomllib.loads(path.read_text(encoding="utf-8"))
    except tomllib.TOMLDecodeError as exc:
        raise ManifestError(path, str(exc)) from exc
    if not isinstance(data, dict):
        raise ManifestError(path, "manifest must be a table")
    _unknown(
        set(data), {"module", "build", "dependencies", "features"}, path, "top-level"
    )

    module = _require_table(data, "module", path)
    _unknown(set(module), MODULE_KEYS, path, "[module]")
    name = module.get("name")
    if not isinstance(name, str) or not NAME_RE.match(name):
        raise ManifestError(path, "module.name must match [a-z][a-z0-9_]*")
    version = module.get("version")
    if not isinstance(version, str) or not SEMVER_RE.match(version):
        raise ManifestError(path, "module.version must be MAJOR.MINOR.PATCH")
    description = module.get("description")
    if not isinstance(description, str) or not description.strip():
        raise ManifestError(path, "module.description is required")
    languages = _string_list(module.get("languages"), path, "module.languages")
    if not languages or any(lang not in LANGUAGES for lang in languages):
        raise ManifestError(path, "module.languages must list c and/or cpp")
    default = module.get("default", True)
    if not isinstance(default, bool):
        raise ManifestError(path, "module.default must be a boolean")
    implements = _string_list(module.get("implements", []), path, "module.implements")
    for tag in implements:
        if not NAME_RE.match(tag):
            raise ManifestError(path, f"implements tag '{tag}' is invalid")
    c_api = module.get("c_api", False)
    if not isinstance(c_api, bool):
        raise ManifestError(path, "module.c_api must be a boolean")

    build = _require_table(data, "build", path)
    _unknown(set(build), BUILD_KEYS, path, "[build]")
    system = build.get("system")
    if system != "cmake":
        raise ManifestError(path, 'build.system must be "cmake"')
    target = name
    alias = default_alias(name)

    deps = data.get("dependencies", {})
    if not isinstance(deps, dict):
        raise ManifestError(path, "[dependencies] must be a table")
    _unknown(set(deps), DEP_KEYS, path, "[dependencies]")
    deps_public = _string_list(deps.get("public", []), path, "dependencies.public")
    deps_private = _string_list(deps.get("private", []), path, "dependencies.private")
    optional = deps.get("optional", {})
    if not isinstance(optional, dict):
        raise ManifestError(path, "[dependencies.optional] must be a table")
    _unknown(set(optional), VIS_KEYS, path, "[dependencies.optional]")
    opt_public = _string_list(
        optional.get("public", []), path, "dependencies.optional.public"
    )
    opt_private = _string_list(
        optional.get("private", []), path, "dependencies.optional.private"
    )
    overlap = set(deps_public) & set(deps_private)
    overlap |= set(opt_public) & set(opt_private)
    overlap |= (set(deps_public) | set(deps_private)) & (
        set(opt_public) | set(opt_private)
    )
    if overlap:
        raise ManifestError(
            path, "dependency listed more than once: " + ", ".join(sorted(overlap))
        )

    features_table = data.get("features", {})
    if not isinstance(features_table, dict):
        raise ManifestError(path, "[features] must be a table")
    features: list[Feature] = []
    for feature_name, spec in features_table.items():
        if not FEATURE_RE.match(feature_name):
            raise ManifestError(
                path, f"feature name '{feature_name}' must be lowercase"
            )
        if not isinstance(spec, dict):
            raise ManifestError(path, f"[features.{feature_name}] must be a table")
        _unknown(set(spec), FEATURE_KEYS, path, f"[features.{feature_name}]")
        feature_default = spec.get("default", False)
        abi = spec.get("abi", False)
        feature_description = spec.get("description", "")
        if not isinstance(feature_default, bool) or not isinstance(abi, bool):
            raise ManifestError(path, f"features.{feature_name} flags must be booleans")
        if not isinstance(feature_description, str):
            raise ManifestError(
                path, f"features.{feature_name}.description must be a string"
            )
        features.append(
            Feature(feature_name, feature_default, abi, feature_description)
        )

    root = path.parent
    if system == "cmake" and not (root / "CMakeLists.txt").is_file():
        raise ManifestError(path, "cmake module is missing CMakeLists.txt")
    if c_api and not (root / "capi" / "include").is_dir():
        raise ManifestError(path, "c_api = true but capi/include/ does not exist")

    return Module(
        name=name,
        path=root,
        version=version,
        description=description.strip(),
        languages=languages,
        default=default,
        implements=implements,
        c_api=c_api,
        system=system,
        target=target,
        alias=alias,
        deps_public=deps_public,
        deps_private=deps_private,
        opt_public=opt_public,
        opt_private=opt_private,
        features=features,
        origin=origin,
    )


def discover(root: Path | None, extra_dirs: list[Path]) -> list[Module]:
    found: list[Module] = []
    if root is not None:
        if not root.is_dir():
            raise ManifestError(None, f"module root does not exist: {root}")
        for child in sorted(root.iterdir()):
            manifest = child / "module.toml"
            if child.is_dir() and manifest.is_file():
                found.append(load_manifest(manifest, origin="engine"))
    for extra in extra_dirs:
        if not extra.exists():
            raise ManifestError(None, f"extra module dir does not exist: {extra}")
        if (extra / "module.toml").is_file():
            found.append(load_manifest(extra / "module.toml", origin="user"))
            continue
        if not extra.is_dir():
            raise ManifestError(None, f"extra module dir is not a directory: {extra}")
        children = [child for child in extra.iterdir() if child.is_dir()]
        if not children and (extra / "CMakeLists.txt").is_file():
            raise ManifestError(
                extra / "CMakeLists.txt",
                "CMakeLists.txt without module.toml",
            )
        saw_manifest = False
        for child in sorted(children):
            manifest = child / "module.toml"
            cmake = child / "CMakeLists.txt"
            if manifest.is_file():
                saw_manifest = True
                found.append(load_manifest(manifest, origin="user"))
            elif cmake.is_file():
                raise ManifestError(
                    cmake,
                    "CMakeLists.txt without module.toml. "
                    "Add module.toml or remove the directory from the module search path.",
                )
        if (
            not saw_manifest
            and (extra / "CMakeLists.txt").is_file()
            and not (extra / "module.toml").is_file()
        ):
            raise ManifestError(
                extra / "CMakeLists.txt", "CMakeLists.txt without module.toml"
            )
    return found


_IDENT = re.compile(r"\b([A-Z][A-Z0-9_]*_AVAILABLE)\b")


def scan_private_macros(module: Module) -> list[str]:
    """Best-effort scan of public headers for private dependency availability macros."""
    private_names = {dep.upper() + "_AVAILABLE" for dep in module.deps_private}
    private_names |= {dep.upper() + "_AVAILABLE" for dep in module.opt_private}
    # Availability macros use the dependency *target* name. Callers pass modules
    # whose target defaults to name, so NAME_AVAILABLE matches. Also accept the
    # declared dependency name, which is the common case.
    if not private_names:
        return []
    errors: list[str] = []
    roots = [module.path / "include"]
    if module.c_api:
        roots.append(module.path / "capi" / "include")
    for root in roots:
        if not root.is_dir():
            continue
        for header in root.rglob("*"):
            if header.suffix.lower() not in {".h", ".hpp", ".hxx", ".hh"}:
                continue
            text = header.read_text(encoding="utf-8", errors="replace")
            stripped = _strip_comments(text)
            for match in _IDENT.finditer(stripped):
                if match.group(1) in private_names or _macro_matches(
                    match.group(1), module
                ):
                    errors.append(
                        f"{header}: mentions private dependency macro {match.group(1)}"
                    )
    return errors


def _macro_matches(token: str, module: Module) -> bool:
    private = set(module.deps_private + module.opt_private)
    for dep in private:
        if token == dep.upper() + "_AVAILABLE":
            return True
    return False


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    return re.sub(r"//.*?$", "", text, flags=re.M)
