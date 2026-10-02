"""Graph resolution tests."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from helios_modules.manifest import (
    ManifestError,
    default_alias,
    discover,
    load_manifest,
)
from helios_modules.resolve import ResolveError, load_installed, resolve


def _write(root: Path, name: str, body: str, *, cmake: bool = True) -> None:
    module = root / name
    module.mkdir(parents=True)
    (module / "module.toml").write_text(body, encoding="utf-8")
    if cmake:
        (module / "CMakeLists.txt").write_text(
            f"add_library({name})\n", encoding="utf-8"
        )


class ResolveTests(unittest.TestCase):
    def test_required_cycle(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            _write(
                root,
                "helios_a",
                """
[module]
name = "helios_a"
version = "0.1.0"
description = "a"
languages = ["cpp"]
[build]
system = "cmake"
[dependencies]
public = ["helios_b"]
""",
            )
            _write(
                root,
                "helios_b",
                """
[module]
name = "helios_b"
version = "0.1.0"
description = "b"
languages = ["cpp"]
[build]
system = "cmake"
[dependencies]
public = ["helios_a"]
""",
            )
            modules = discover(root, [])
            with self.assertRaises(ResolveError):
                resolve(modules, {}, engine_root=root)

    def test_optional_cycle_when_both_enabled(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            _write(
                root,
                "helios_a",
                """
[module]
name = "helios_a"
version = "0.1.0"
description = "a"
languages = ["cpp"]
[build]
system = "cmake"
[dependencies.optional]
public = ["helios_b"]
""",
            )
            _write(
                root,
                "helios_b",
                """
[module]
name = "helios_b"
version = "0.1.0"
description = "b"
languages = ["cpp"]
[build]
system = "cmake"
[dependencies.optional]
public = ["helios_a"]
""",
            )
            modules = discover(root, [])
            with self.assertRaises(ResolveError):
                resolve(modules, {}, engine_root=root)

    def test_duplicate_implements(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            for name in ("helios_a", "helios_b"):
                _write(
                    root,
                    name,
                    f"""
[module]
name = "{name}"
version = "0.1.0"
description = "{name}"
languages = ["cpp"]
implements = ["window"]
[build]
system = "cmake"
""",
                )
            with self.assertRaises(ResolveError):
                resolve(discover(root, []), {}, engine_root=root)

    def test_zero_implementers(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            _write(
                root,
                "helios_a",
                """
[module]
name = "helios_a"
version = "0.1.0"
description = "a"
languages = ["cpp"]
default = false
implements = ["window"]
[build]
system = "cmake"
""",
            )
            result = resolve(discover(root, []), {}, engine_root=root)
            self.assertEqual(result.backends, {})
            self.assertEqual(result.enabled, [])

    def test_build_all_tie_break(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            for name in ("helios_c", "helios_a", "helios_b"):
                _write(
                    root,
                    name,
                    f"""
[module]
name = "{name}"
version = "0.1.0"
description = "{name}"
languages = ["cpp"]
default = false
implements = ["window"]
[build]
system = "cmake"
""",
                )
            result = resolve(
                discover(root, []),
                {},
                engine_root=root,
                build_all=True,
            )
            self.assertEqual(result.backends["window"], "helios_a")
            self.assertEqual(result.enabled, ["helios_a"])

    def test_force_overrides_off(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            _write(
                root,
                "helios_a",
                """
[module]
name = "helios_a"
version = "0.1.0"
description = "a"
languages = ["cpp"]
[build]
system = "cmake"
[dependencies]
public = ["helios_b"]
""",
            )
            _write(
                root,
                "helios_b",
                """
[module]
name = "helios_b"
version = "0.1.0"
description = "b"
languages = ["cpp"]
default = false
[build]
system = "cmake"
""",
            )
            result = resolve(
                discover(root, []),
                {"HELIOS_BUILD_HELIOS_B": "OFF"},
                engine_root=root,
            )
            self.assertTrue(any("overriding" in warning for warning in result.warnings))
            self.assertIn("helios_b", result.enabled)

    def test_user_prefix_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            extra = root / "extra"
            engine = root / "src"
            engine.mkdir()
            _write(
                extra,
                "helios_user",
                """
[module]
name = "helios_user"
version = "0.1.0"
description = "nope"
languages = ["cpp"]
[build]
system = "cmake"
""",
            )
            with self.assertRaises(ResolveError):
                resolve(discover(engine, [extra]), {}, engine_root=engine)

    def test_bad_schema(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            path = Path(raw) / "module.toml"
            path.write_text(
                """
[module]
name = "Bad"
version = "1"
description = "x"
languages = ["cpp"]
[build]
system = "cmake"
""",
                encoding="utf-8",
            )
            with self.assertRaises(ManifestError):
                load_manifest(path, origin="engine")

    def test_cmake_without_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            extra = Path(raw) / "mods" / "orphan"
            extra.mkdir(parents=True)
            (extra / "CMakeLists.txt").write_text("add_library(x)\n", encoding="utf-8")
            with self.assertRaises(ManifestError):
                discover(None, [extra.parent])

    def test_default_alias_nested(self) -> None:
        self.assertEqual(default_alias("helios_sdl3_window"), "helios::sdl3::window")
        self.assertEqual(default_alias("helios_ecs"), "helios::ecs")

    def test_rejects_build_cmake(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            _write(
                root,
                "helios_a",
                """
[module]
name = "helios_a"
version = "0.1.0"
description = "a"
languages = ["cpp"]
[build]
system = "cmake"
[build.cmake]
target = "helios_a"
""",
            )
            with self.assertRaises(ManifestError):
                discover(root, [])

    def test_alias_collision(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            graph = Path(raw) / "graph.json"
            graph.write_text(
                json.dumps(
                    {
                        "modules": [
                            {
                                "name": "helios_a",
                                "version": "0.1.0",
                                "alias": "helios::same",
                            },
                            {
                                "name": "helios_b",
                                "version": "0.1.0",
                                "alias": "helios::same",
                            },
                        ]
                    }
                ),
                encoding="utf-8",
            )
            modules = load_installed(graph)
            with self.assertRaises(ResolveError):
                resolve(modules, {}, engine_root=Path(raw))

    def test_engine_cannot_depend_on_user(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw) / "src"
            extra = Path(raw) / "user"
            _write(
                root,
                "helios_core",
                """
[module]
name = "helios_core"
version = "0.1.0"
description = "core"
languages = ["cpp"]
[build]
system = "cmake"
[dependencies]
public = ["mygame_physics"]
""",
            )
            _write(
                extra,
                "mygame_physics",
                """
[module]
name = "mygame_physics"
version = "0.1.0"
description = "physics"
languages = ["cpp"]
[build]
system = "cmake"
""",
            )
            with self.assertRaises(ResolveError):
                resolve(discover(root, [extra]), {}, engine_root=root)

    def test_backend_override_explicit_on(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw) / "src"
            extra = Path(raw) / "user"
            _write(
                root,
                "helios_glfw",
                """
[module]
name = "helios_glfw"
version = "0.1.0"
description = "glfw"
languages = ["cpp"]
implements = ["window"]
[build]
system = "cmake"
""",
            )
            _write(
                extra,
                "mygame_window",
                """
[module]
name = "mygame_window"
version = "0.1.0"
description = "window"
languages = ["cpp"]
implements = ["window"]
[build]
system = "cmake"
""",
            )
            with self.assertRaises(ResolveError):
                resolve(
                    discover(root, [extra]),
                    {"HELIOS_BUILD_HELIOS_GLFW": "ON"},
                    engine_root=root,
                )

    def test_installed_tag_conflict_and_missing_optional(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            graph = Path(raw) / "modules.json"
            graph.write_text(
                """
{
  "modules": [
    {
      "name": "helios_glfw",
      "version": "0.1.0",
      "enabled": true,
      "implements": ["window"],
      "alias": "helios::glfw",
      "target": "helios_glfw",
      "languages": ["cpp"],
      "dependencies": {"public": [], "private": []},
      "optional": {"public": [], "private": []}
    }
  ],
  "enabled": ["helios_glfw"]
}
""",
                encoding="utf-8",
            )
            extra = Path(raw) / "user"
            _write(
                extra,
                "mygame_window",
                """
[module]
name = "mygame_window"
version = "0.1.0"
description = "window"
languages = ["cpp"]
implements = ["window"]
[build]
system = "cmake"
[dependencies.optional]
public = ["helios_missing"]
""",
            )
            modules = load_installed(graph)
            modules.extend(discover(None, [extra]))
            with self.assertRaises(ResolveError):
                resolve(modules, {}, engine_root=None)
            _write(
                extra,
                "mygame_physics",
                """
[module]
name = "mygame_physics"
version = "0.1.0"
description = "physics"
languages = ["cpp"]
[build]
system = "cmake"
[dependencies.optional]
public = ["helios_missing"]
""",
            )
            modules = load_installed(graph)
            modules.extend(discover(None, [extra / "mygame_physics"]))
            result = resolve(modules, {}, engine_root=None)
            self.assertIn("mygame_physics", result.enabled)
            self.assertNotIn("helios_missing", result.enabled)


if __name__ == "__main__":
    unittest.main()
