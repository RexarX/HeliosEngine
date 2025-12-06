"""Conan recipe for Helios Engine

This file defines the Conan package for Helios Engine, including all dependencies
and build configuration options.
"""

import os
from pathlib import Path

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, load

required_conan_version = ">=2.0.0"


class HeliosEngineConan(ConanFile):
    name = "helios-engine"
    description = "Modular game engine focusing on flexibility and performance"
    topics = ("game-engine", "ecs", "async")
    url = "https://github.com/RexarX/HeliosEngine"
    homepage = "https://github.com/RexarX/HeliosEngine"
    license = "MIT"
    package_type = "library"

    settings = "os", "arch", "compiler", "build_type"

    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "core_only": [True, False],
        "with_tests": [True, False],
        "with_examples": [True, False],
        "enable_profiling": [True, False],
        "enable_lto": [True, False],
        "enable_unity_build": [True, False],
    }

    default_options = {
        "shared": False,
        "fPIC": True,
        "core_only": False,
        "with_tests": False,
        "with_examples": False,
        "enable_profiling": False,
        "enable_lto": True,
        "enable_unity_build": False,
        # Boost options
        "boost/*:header_only": False,
        "boost/*:shared": False,
        # spdlog options
        "spdlog/*:header_only": True,
    }

    def export_sources(self):
        """Export source files needed for building"""

        copy(self, "CMakeLists.txt", self.recipe_folder, self.export_sources_folder)
        copy(self, "cmake/*", self.recipe_folder, self.export_sources_folder)
        copy(self, "src/*", self.recipe_folder, self.export_sources_folder)
        copy(self, "third-party/*", self.recipe_folder, self.export_sources_folder)
        if self.options.with_tests:
            copy(self, "tests/*", self.recipe_folder, self.export_sources_folder)
        if self.options.with_examples:
            copy(self, "examples/*", self.recipe_folder, self.export_sources_folder)

    def set_version(self):
        """Read version from CMakeLists.txt"""

        cmake_file = Path(self.recipe_folder) / "CMakeLists.txt"
        if cmake_file.exists():
            content = load(self, str(cmake_file))
            # Extract version from project() command
            import re

            match = re.search(r"project\([^)]*VERSION\s+(\d+\.\d+\.\d+)", content)
            if match:
                self.version = match.group(1)
            else:
                self.version = "0.1.0"
        else:
            self.version = "0.1.0"

    def config_options(self):
        """Remove options that don't apply to current platform"""

        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        """Configure options based on dependencies"""

        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        """Define the build layout - use unified build/ directory"""

        # When using --output-folder build/conan, don't add another build subfolder
        # Just use the current directory (relative to output folder)
        self.folders.build = "."
        self.folders.generators = "."

        # Set source folder
        self.folders.source = "."

    def requirements(self):
        """Define all dependencies"""

        # Core dependencies (always required)
        self.requires("boost/[>=1.82 <2]", transitive_headers=True)
        self.requires("spdlog/[>=1.14 <2]", transitive_headers=True)
        self.requires("stduuid/[>=1.2 <2]")
        self.requires("concurrentqueue/[>=1.0 <2]")

        self.requires("taskflow/[>=3.10 <3.11]", transitive_headers=True)

        # Module dependencies (only if not core_only)
        if not self.options.core_only:
            # Add module-specific dependencies here as needed
            # Example:
            # self.requires("vulkan/[>=1.3 <2]")
            # self.requires("glfw/[>=3.3 <4]")
            pass

        # Test dependencies
        if self.options.with_tests:
            self.requires("doctest/[>=2.4 <3]")

    def build_requirements(self):
        """Define build-time dependencies"""

        pass

    def validate(self):
        """Validate configuration"""

        if self.settings.os == "Windows" and self.settings.compiler == "gcc":
            raise ConanInvalidConfiguration(
                "GCC is not supported on Windows for Helios Engine"
            )

        # Check C++ standard
        if self.settings.compiler.get_safe("cppstd"):
            if int(str(self.settings.compiler.cppstd)) < 23:
                raise ConanInvalidConfiguration(
                    "Helios Engine requires C++23 or higher"
                )

    def generate(self):
        """Generate build system files"""

        # CMake toolchain
        tc = CMakeToolchain(self)

        # Pass options to CMake
        tc.variables["HELIOS_BUILD_CORE_ONLY"] = self.options.core_only
        tc.variables["HELIOS_BUILD_TESTS"] = self.options.with_tests
        tc.variables["HELIOS_BUILD_EXAMPLES"] = self.options.with_examples
        tc.variables["HELIOS_ENABLE_PROFILING"] = self.options.enable_profiling
        tc.variables["HELIOS_ENABLE_LTO"] = self.options.enable_lto
        tc.variables["HELIOS_ENABLE_UNITY_BUILD"] = self.options.enable_unity_build

        # Use Conan for dependencies
        tc.variables["HELIOS_USE_CONAN"] = True

        tc.generate()

        # CMake dependencies
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        """Build the project"""

        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        """Package the built artifacts"""

        cmake = CMake(self)
        cmake.install()

        # Copy license
        copy(
            self,
            "LICENSE*",
            src=self.source_folder,
            dst=os.path.join(self.package_folder, "licenses"),
        )

    def package_info(self):
        """Define package information for consumers"""

        # Use config files for finding the package
        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "HeliosEngine")
        self.cpp_info.set_property("cmake_target_name", "helios::engine")

        # Add build directories for CMake config files
        self.cpp_info.builddirs.append(os.path.join("lib", "cmake", "HeliosEngine"))

        # Define core component (always available)
        self.cpp_info.components["core"].set_property(
            "cmake_target_name", "helios::core"
        )
        self.cpp_info.components["core"].libs = ["helios_core"]
        self.cpp_info.components["core"].requires = [
            "boost::boost",
            "spdlog::spdlog",
            "stduuid::stduuid",
            "concurrentqueue::concurrentqueue",
            "taskflow::taskflow",
        ]

        # Platform-specific requirements
        if self.settings.os == "Linux":
            self.cpp_info.components["core"].system_libs = ["pthread", "dl", "m"]
        elif self.settings.os == "Windows":
            self.cpp_info.components["core"].system_libs = [
                "dbghelp",
                "ole32",
                "shell32",
            ]

    def package_id(self):
        """Customize package ID computation"""

        # Test and examples don't affect binary compatibility
        del self.info.options.with_tests
        del self.info.options.with_examples
        # core_only affects which modules are built
        # Keep it in package ID
