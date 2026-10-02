"""CLI for the Helios module tool."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from .emit_cmake import emit_cmake, emit_json
from .manifest import ManifestError, discover
from .resolve import ResolveError, load_installed, resolve
from .scaffold import scaffold
from .sdk import SdkError, write_sdk


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="helios_modules")
    sub = parser.add_subparsers(dest="command", required=True)

    def add_common(command: argparse.ArgumentParser) -> None:
        command.add_argument("--root", type=Path, help="Engine src directory")
        command.add_argument("--extra-dir", action="append", default=[], type=Path)
        command.add_argument("--installed-graph", type=Path)
        command.add_argument("--set", action="append", default=[], help="KEY=VALUE")

    resolve_cmd = sub.add_parser("resolve")
    add_common(resolve_cmd)
    resolve_cmd.add_argument("--cmake-out", type=Path, required=True)
    resolve_cmd.add_argument("--json-out", type=Path, required=True)
    resolve_cmd.add_argument(
        "--user-only",
        action="store_true",
        help="Emit build rules only for user modules",
    )

    validate_cmd = sub.add_parser("validate")
    add_common(validate_cmd)

    list_cmd = sub.add_parser("list")
    add_common(list_cmd)

    graph_cmd = sub.add_parser("graph")
    add_common(graph_cmd)
    graph_cmd.add_argument("--format", choices=("mermaid", "dot"), default="mermaid")

    new_cmd = sub.add_parser("new")
    new_cmd.add_argument("name")
    new_cmd.add_argument("--dir", type=Path, required=True)
    new_cmd.add_argument("--header-only", action="store_true")
    new_cmd.add_argument("--c-api", action="store_true")

    sdk_cmd = sub.add_parser("sdk")
    sdk_cmd.add_argument("--graph", type=Path, required=True)
    sdk_cmd.add_argument("--artifacts", type=Path, required=True)
    sdk_cmd.add_argument("--prefix", type=Path, required=True)
    sdk_cmd.add_argument("--include-dir", type=Path)
    sdk_cmd.add_argument("--cmake-dir", type=Path)
    return parser


def _settings(pairs: list[str]) -> dict[str, str]:
    found: dict[str, str] = {}
    for pair in pairs:
        if "=" not in pair:
            raise SystemExit(f"--set expects KEY=VALUE, got '{pair}'")
        key, value = pair.split("=", 1)
        found[key] = value
    return found


def _load(args: argparse.Namespace):
    extra = [path.resolve() for path in args.extra_dir]
    root = args.root.resolve() if args.root else None
    modules = []
    if args.installed_graph:
        modules.extend(load_installed(args.installed_graph.resolve()))
    modules.extend(discover(root, extra))
    settings = _settings(args.set)
    build_all = settings.get("HELIOS_BUILD_ALL_MODULES", "").lower() in {
        "1",
        "on",
        "true",
        "yes",
    }
    return resolve(modules, settings, engine_root=root, build_all=build_all)


def main(argv: list[str] | None = None) -> int:
    args = _parser().parse_args(argv)
    try:
        if args.command == "new":
            root = scaffold(
                args.name,
                args.dir.resolve(),
                header_only=args.header_only,
                c_api=args.c_api,
            )
            print(root)
            return 0
        if args.command == "sdk":
            graph = json.loads(args.graph.read_text(encoding="utf-8"))
            write_sdk(
                graph=graph,
                artifacts=args.artifacts,
                prefix=args.prefix,
                pkgconfig_dir=args.prefix / "lib" / "pkgconfig",
                share_dir=args.prefix / "share" / "helios",
                include_dir=args.include_dir,
                cmake_dir=args.cmake_dir,
            )
            return 0
        resolution = _load(args)
    except (ManifestError, ResolveError, SdkError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    if args.command == "resolve":
        args.cmake_out.parent.mkdir(parents=True, exist_ok=True)
        args.json_out.parent.mkdir(parents=True, exist_ok=True)
        args.cmake_out.write_text(
            emit_cmake(resolution, user_only=args.user_only), encoding="utf-8"
        )
        args.json_out.write_text(
            json.dumps(emit_json(resolution), indent=2) + "\n", encoding="utf-8"
        )
        return 0
    if args.command == "validate":
        print(f"validated {len(resolution.modules)} modules")
        return 0
    if args.command == "list":
        for name in resolution.enabled:
            print(name)
        return 0
    if args.command == "graph":
        by_name = {module.name: module for module in resolution.modules}
        if args.format == "dot":
            print("digraph modules {")
            for name in resolution.enabled:
                module = by_name[name]
                for dep in module.required():
                    print(f'  "{name}" -> "{dep}";')
            print("}")
        else:
            print("flowchart LR")
            for name in resolution.enabled:
                module = by_name[name]
                for dep in module.required():
                    print(f"    {name} --> {dep}")
        return 0
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
