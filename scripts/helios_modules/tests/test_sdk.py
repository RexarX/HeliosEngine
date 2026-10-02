"""SDK install checks."""

from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from helios_modules.sdk import SdkError, write_sdk


class SdkTests(unittest.TestCase):
    def test_missing_library_is_an_error(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            artifacts = root / "helios_artifacts_Debug.toml"
            artifacts.write_text(
                """
version = "0.1.0"
config = "Debug"

[libraries.helios_core]
kind = "STATIC_LIBRARY"
file_name = "helios_core.lib"
linker_file = "helios_core.lib"
""",
                encoding="utf-8",
            )
            graph = {
                "modules": [
                    {
                        "name": "helios_core",
                        "enabled": True,
                        "description": "core",
                        "version": "0.1.0",
                        "dependencies": {"public": []},
                    }
                ],
                "enabled": ["helios_core"],
            }
            with self.assertRaises(SdkError) as caught:
                write_sdk(
                    graph=graph,
                    artifacts=artifacts,
                    prefix=root / "prefix",
                    pkgconfig_dir=root / "prefix" / "lib" / "pkgconfig",
                    share_dir=root / "prefix" / "share" / "helios",
                )
            self.assertIn("cmake --build", str(caught.exception))

    def test_writes_pc_requires(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            prefix = root / "prefix"
            (prefix / "lib").mkdir(parents=True)
            (prefix / "lib" / "helios_core.lib").write_bytes(b"")
            (prefix / "lib" / "helios_utils.lib").write_bytes(b"")
            artifacts = root / "helios_artifacts_Debug.toml"
            artifacts.write_text(
                """
version = "0.1.0"
config = "Debug"
compiler_id = "MSVC"
compiler_version = "19.44"
cxx_standard = "23"
stdlib = "msvc-19"

[libraries.helios_core]
kind = "STATIC_LIBRARY"
file_name = "helios_core.lib"
linker_file = "helios_core.lib"

[libraries.helios_utils]
kind = "STATIC_LIBRARY"
file_name = "helios_utils.lib"
linker_file = "helios_utils.lib"
""",
                encoding="utf-8",
            )
            graph = {
                "modules": [
                    {
                        "name": "helios_core",
                        "enabled": True,
                        "description": "core",
                        "version": "0.1.0",
                        "alias": "helios::core",
                        "dependencies": {"public": []},
                    },
                    {
                        "name": "helios_utils",
                        "enabled": True,
                        "description": "utils",
                        "version": "0.1.0",
                        "dependencies": {"public": ["helios_core"]},
                    },
                ],
                "enabled": ["helios_core", "helios_utils"],
            }
            write_sdk(
                graph=graph,
                artifacts=artifacts,
                prefix=prefix,
                pkgconfig_dir=prefix / "lib" / "pkgconfig",
                share_dir=prefix / "share" / "helios",
            )
            utils_pc = (prefix / "lib" / "pkgconfig" / "helios_utils.pc").read_text(
                encoding="utf-8"
            )
            self.assertIn("Requires: helios_core", utils_pc)
            sdk_toml = (prefix / "share" / "helios" / "helios_sdk.toml").read_text(
                encoding="utf-8"
            )
            self.assertIn("asserts = true", sdk_toml)
            self.assertIn(
                "Cflags: -I${prefix}/include -DHELIOS_ENABLE_ASSERTS -DHELIOS_ENABLE_STACKTRACE",
                utils_pc,
            )
            header = (prefix / "include" / "helios" / "sdk_checks.hpp").read_text(
                encoding="utf-8"
            )
            self.assertIn("#define HELIOS_ENABLE_ASSERTS 1", header)
            checks = (
                prefix / "lib" / "cmake" / "Helios" / "HeliosSdkChecks.cmake"
            ).read_text(encoding="utf-8")
            self.assertIn("set(HELIOS_SDK_ASSERTS TRUE)", checks)
            self.assertIn('if(TARGET "helios::core")', checks)
            json.dumps(graph)

    def test_release_sdk_leaves_asserts_off(self) -> None:
        with tempfile.TemporaryDirectory() as raw:
            root = Path(raw)
            prefix = root / "prefix"
            (prefix / "lib").mkdir(parents=True)
            (prefix / "lib" / "libhelios.so").write_bytes(b"")
            artifacts = root / "helios_artifacts_Release.toml"
            artifacts.write_text(
                """
version = "0.1.0"
config = "Release"
compiler_id = "GNU"
compiler_version = "14.2.0"
cxx_standard = "23"
stdlib = "libstdc++-14"

[libraries.helios]
kind = "SHARED_LIBRARY"
file_name = "libhelios.so"
linker_file = "libhelios.so"
""",
                encoding="utf-8",
            )
            graph = {
                "modules": [
                    {
                        "name": "helios_core",
                        "enabled": True,
                        "description": "core",
                        "version": "0.1.0",
                        "alias": "helios::core",
                        "dependencies": {"public": []},
                    }
                ],
                "enabled": ["helios_core"],
            }
            write_sdk(
                graph=graph,
                artifacts=artifacts,
                prefix=prefix,
                pkgconfig_dir=prefix / "lib" / "pkgconfig",
                share_dir=prefix / "share" / "helios",
            )
            sdk_toml = (prefix / "share" / "helios" / "helios_sdk.toml").read_text(
                encoding="utf-8"
            )
            self.assertIn('config = "Release"', sdk_toml)
            self.assertIn("asserts = false", sdk_toml)
            pc = (prefix / "lib" / "pkgconfig" / "helios.pc").read_text(encoding="utf-8")
            self.assertIn("Cflags: -I${prefix}/include\n", pc)
            self.assertNotIn("HELIOS_ENABLE_ASSERTS", pc)
            header = (prefix / "include" / "helios" / "sdk_checks.hpp").read_text(
                encoding="utf-8"
            )
            self.assertNotIn("#define HELIOS_ENABLE_ASSERTS", header)
            checks = (
                prefix / "lib" / "cmake" / "Helios" / "HeliosSdkChecks.cmake"
            ).read_text(encoding="utf-8")
            self.assertIn("set(HELIOS_SDK_ASSERTS FALSE)", checks)
            self.assertNotIn("target_compile_definitions", checks)


if __name__ == "__main__":
    unittest.main()
