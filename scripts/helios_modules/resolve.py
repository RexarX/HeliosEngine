"""Resolve the module graph."""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

from .manifest import ManifestError, Module, scan_private_macros


class ResolveError(Exception):
    pass


@dataclass
class Resolution:
    modules: list[Module]
    enabled: list[str]
    warnings: list[str]
    status: list[str]
    backends: dict[str, str] = field(default_factory=dict)
    forced: list[tuple[str, str]] = field(default_factory=list)


def _parse_bool(value: str) -> bool:
    text = value.strip().lower()
    if text in {"1", "on", "true", "yes"}:
        return True
    if text in {"0", "off", "false", "no"}:
        return False
    raise ResolveError(f"expected a boolean, got '{value}'")


def _settings(raw: dict[str, str]) -> dict[str, bool]:
    return {key: _parse_bool(value) for key, value in raw.items()}


def _index(modules: list[Module]) -> dict[str, Module]:
    found: dict[str, Module] = {}
    for module in modules:
        if module.name in found:
            raise ResolveError(
                f"duplicate module name '{module.name}' "
                f"({found[module.name].path} and {module.path})"
            )
        found[module.name] = module
    return found


def _check_names(modules: list[Module], engine_root: Path | None) -> None:
    aliases: dict[str, str] = {}
    for module in modules:
        in_engine = engine_root is not None and _is_relative_to(
            module.path, engine_root
        )
        if module.origin == "engine" or in_engine:
            if not module.name.startswith("helios_"):
                raise ResolveError(
                    f"{module.name} is under the engine src tree and must be named helios_*"
                )
        else:
            if module.name.startswith("helios_"):
                raise ResolveError(
                    f"{module.name} uses the reserved helios_ prefix outside the engine src tree"
                )
            if module.alias and module.alias.split("::", 1)[0] == "helios":
                raise ResolveError(
                    f"{module.name} cannot use the helios:: alias namespace"
                )
        if module.alias:
            previous = aliases.get(module.alias)
            if previous:
                raise ResolveError(
                    f"alias '{module.alias}' is used by both {previous} and {module.name}"
                )
            aliases[module.alias] = module.name


def _is_relative_to(path: Path, root: Path) -> bool:
    try:
        path.resolve().relative_to(root.resolve())
        return True
    except ValueError:
        return False


def _check_edges(modules: dict[str, Module]) -> None:
    for module in modules.values():
        if module.fixed:
            continue
        for dep_name in module.required() + module.opt_public + module.opt_private:
            dep = modules.get(dep_name)
            if dep is None:
                kind = "optional" if dep_name not in module.required() else "required"
                if kind == "optional":
                    continue
                raise ResolveError(
                    f"{module.name} depends on unknown module '{dep_name}'"
                )
            if module.origin != "engine" and dep.origin == "user":
                pass
            if dep.origin == "user" and module.origin == "engine":
                raise ResolveError(
                    f"engine module {module.name} cannot depend on user module {dep_name}"
                )
            if "c" in module.languages and "cpp" not in module.languages:
                if dep_name in module.deps_public and "c" not in dep.languages:
                    raise ResolveError(
                        f"{module.name} is C-only but publicly depends on {dep_name}, "
                        "which does not list language c"
                    )


def _required_ancestors(modules: dict[str, Module], name: str) -> set[str]:
    seen: set[str] = set()
    stack = list(modules[name].required())
    while stack:
        dep = stack.pop()
        if dep in seen or dep not in modules:
            continue
        seen.add(dep)
        stack.extend(modules[dep].required())
    return seen


def _apply_c_api(modules: dict[str, Module], c_api: bool) -> None:
    if not c_api or "helios_capi" not in modules:
        return
    # helios_capi is a leaf used by other C APIs. Do not point it back at
    # modules it already depends on (helios_platform includes its C headers).
    ancestors = _required_ancestors(modules, "helios_capi")
    for module in modules.values():
        if not module.c_api or module.name == "helios_capi" or module.name in ancestors:
            continue
        if (
            "helios_capi" not in module.deps_public
            and "helios_capi" not in module.deps_private
        ):
            module.deps_public.append("helios_capi")


def resolve(
    modules: list[Module],
    settings: dict[str, str],
    *,
    engine_root: Path | None = None,
    build_all: bool = False,
) -> Resolution:
    by_name = _index(modules)
    _check_names(modules, engine_root)
    flags = _settings(settings)
    c_api = flags.get("HELIOS_BUILD_C_API", False)
    _apply_c_api(by_name, c_api)
    _check_edges(by_name)

    explicit: dict[str, bool] = {}
    enabled: dict[str, bool] = {}
    for module in modules:
        if module.fixed:
            enabled[module.name] = module.enabled
            continue
        key = module.option_name()
        if key in flags:
            explicit[module.name] = flags[key]
            enabled[module.name] = flags[key]
        else:
            enabled[module.name] = module.default

    if build_all or flags.get("HELIOS_BUILD_ALL_MODULES", False):
        for module in modules:
            if module.fixed:
                continue
            if explicit.get(module.name) is False:
                continue
            enabled[module.name] = True

    status: list[str] = []
    _backend_override(modules, enabled, explicit, status)
    _build_all_tie_break(
        modules,
        enabled,
        build_all or flags.get("HELIOS_BUILD_ALL_MODULES", False),
        status,
    )

    warnings: list[str] = []
    forced: list[tuple[str, str]] = []
    _force_required(by_name, enabled, explicit, warnings, forced)
    _check_backends(by_name, enabled)
    _check_installed_backends(by_name, enabled)
    order = _topo(by_name, enabled)
    for module in modules:
        module.enabled = enabled[module.name]
        module.backend_wins = [
            tag
            for tag, winner in _winners(by_name, enabled).items()
            if winner == module.name
        ]
    for module in modules:
        if module.fixed:
            continue
        for problem in scan_private_macros(module):
            raise ResolveError(problem)
    return Resolution(
        modules, order, warnings, status, _winners(by_name, enabled), forced
    )


def _backend_override(
    modules: list[Module],
    enabled: dict[str, bool],
    explicit: dict[str, bool],
    status: list[str],
) -> None:
    tags: dict[str, list[Module]] = {}
    for module in modules:
        for tag in module.implements:
            tags.setdefault(tag, []).append(module)
    for tag, group in tags.items():
        users = [
            module
            for module in group
            if module.origin == "user" and enabled[module.name]
        ]
        if not users:
            continue
        for engine in group:
            if engine.origin != "engine":
                continue
            if explicit.get(engine.name) is True and enabled[engine.name]:
                raise ResolveError(
                    f"user module {users[0].name} and engine module {engine.name} both "
                    f"implement '{tag}', and {engine.option_name()} was explicitly ON. "
                    "Disable the engine backend or the user module."
                )
            if (
                engine.default
                and enabled[engine.name]
                and explicit.get(engine.name) is not True
            ):
                enabled[engine.name] = False
                status.append(
                    f"{engine.name} disabled: user module {users[0].name} implements {tag}"
                )


def _build_all_tie_break(
    modules: list[Module],
    enabled: dict[str, bool],
    build_all: bool,
    status: list[str],
) -> None:
    if not build_all:
        return
    tags: dict[str, list[Module]] = {}
    for module in modules:
        if not enabled[module.name]:
            continue
        for tag in module.implements:
            tags.setdefault(tag, []).append(module)
    for tag, group in tags.items():
        if len(group) < 2:
            continue
        winner = sorted(group, key=lambda module: module.name)[0]
        for module in group:
            if module is winner or module.fixed:
                continue
            enabled[module.name] = False
            status.append(
                f"{module.name} disabled: implements {tag}, winner is {winner.name}"
            )


def _force_required(
    modules: dict[str, Module],
    enabled: dict[str, bool],
    explicit: dict[str, bool],
    warnings: list[str],
    forced: list[tuple[str, str]],
) -> None:
    changed = True
    guard = 0
    while changed:
        changed = False
        guard += 1
        if guard > 100:
            raise ResolveError("dependency resolution did not converge")
        for module in list(modules.values()):
            if not enabled.get(module.name):
                continue
            for dep_name in module.required():
                dep = modules.get(dep_name)
                if dep is None:
                    raise ResolveError(
                        f"{module.name} depends on unknown module '{dep_name}'"
                    )
                if enabled.get(dep_name):
                    continue
                if dep.fixed:
                    raise ResolveError(
                        f"{module.name} requires {dep_name}, which is not in this Helios install"
                    )
                if explicit.get(dep_name) is False:
                    warnings.append(
                        f"Helios: enabling {dep_name} because {module.name} requires it "
                        f"(overriding {dep.option_name()}=OFF)"
                    )
                enabled[dep_name] = True
                forced.append((dep_name, module.name))
                changed = True


def _winners(modules: dict[str, Module], enabled: dict[str, bool]) -> dict[str, str]:
    winners: dict[str, str] = {}
    for module in modules.values():
        if not enabled.get(module.name):
            continue
        for tag in module.implements:
            winners[tag] = module.name
    return winners


def _check_backends(modules: dict[str, Module], enabled: dict[str, bool]) -> None:
    seen: dict[str, str] = {}
    for module in modules.values():
        if not enabled.get(module.name) or module.fixed:
            continue
        for tag in module.implements:
            other = seen.get(tag)
            if other:
                raise ResolveError(
                    f"module '{module.name}' and module '{other}' both implement '{tag}'. "
                    "Enable only one backend."
                )
            seen[tag] = module.name


def _check_installed_backends(
    modules: dict[str, Module], enabled: dict[str, bool]
) -> None:
    installed: dict[str, str] = {}
    for module in modules.values():
        if module.fixed and module.enabled:
            for tag in module.implements:
                installed[tag] = module.name
    for module in modules.values():
        if module.fixed or not enabled.get(module.name):
            continue
        for tag in module.implements:
            if tag in installed:
                raise ResolveError(
                    f"module '{module.name}' implements '{tag}', but installed module "
                    f"'{installed[tag]}' already does. Build Helios without that backend."
                )


def _topo(modules: dict[str, Module], enabled: dict[str, bool]) -> list[str]:
    nodes = [name for name, on in enabled.items() if on and not modules[name].fixed]
    indegree = {name: 0 for name in nodes}
    outgoing: dict[str, list[str]] = {name: [] for name in nodes}

    def edges(name: str) -> list[str]:
        module = modules[name]
        deps = list(module.required())
        for dep in module.opt_public + module.opt_private:
            if enabled.get(dep):
                deps.append(dep)
        return deps

    for name in nodes:
        for dep in edges(name):
            if dep not in indegree:
                continue
            indegree[name] += 1
            outgoing[dep].append(name)
    queue = sorted(name for name, degree in indegree.items() if degree == 0)
    ordered: list[str] = []
    while queue:
        current = queue.pop(0)
        ordered.append(current)
        for neighbor in sorted(outgoing[current]):
            indegree[neighbor] -= 1
            if indegree[neighbor] == 0:
                queue.append(neighbor)
        queue.sort()
    if len(ordered) != len(nodes):
        stuck = [name for name in nodes if name not in ordered]
        raise ResolveError("module dependency cycle among: " + ", ".join(stuck))
    return ordered


def load_installed(path: Path) -> list[Module]:
    import json

    from .manifest import Feature

    data = json.loads(path.read_text(encoding="utf-8"))
    modules: list[Module] = []
    for raw in data.get("modules", []):
        if not raw.get("enabled", True):
            continue
        features = [
            Feature(
                item["name"],
                item.get("default", False),
                item.get("abi", False),
                item.get("description", ""),
            )
            for item in raw.get("features", [])
        ]
        module = Module(
            name=raw["name"],
            path=Path(raw.get("path", ".")),
            version=raw["version"],
            description=raw.get("description", ""),
            languages=raw.get("languages", ["cpp"]),
            default=True,
            implements=raw.get("implements", []),
            c_api=raw.get("c_api", False),
            system="cmake",
            target=raw.get("target", raw["name"]),
            alias=raw.get("alias"),
            deps_public=raw.get("dependencies", {}).get("public", []),
            deps_private=raw.get("dependencies", {}).get("private", []),
            opt_public=raw.get("optional", {}).get("public", []),
            opt_private=raw.get("optional", {}).get("private", []),
            features=features,
            origin="engine",
            fixed=True,
            enabled=True,
            backend_wins=raw.get("backend_wins", raw.get("implements", [])),
        )
        modules.append(module)
    return modules
