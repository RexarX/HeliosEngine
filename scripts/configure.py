#!/usr/bin/env python3
"""
HeliosEngine Configuration Script

Cross-platform CMake configuration script for the project.

This script handles:
- Compiler detection and selection
- Build system configuration (Ninja, MSBuild, Make)
- CMake configuration with build options
- Conan dependency integration
- Module selection

This script should be called before building, or will be called automatically
by build.py.
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Tuple


class Colors:
    """ANSI color codes for terminal output"""

    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[1;33m"
    BLUE = ""
    NC = "\033[0m"  # No Color

    @classmethod
    def disable(cls):
        """Disable colors (for Windows terminals that don't support ANSI)"""
        cls.RED = ""
        cls.GREEN = ""
        cls.YELLOW = ""
        cls.BLUE = ""
        cls.NC = ""


# Enable colors on Windows 10+
if sys.platform == "win32":
    try:
        import ctypes

        kernel32 = ctypes.windll.kernel32
        kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
    except Exception:
        Colors.disable()


def print_info(msg: str) -> None:
    """Print info message"""

    print(msg)


def print_success(msg: str) -> None:
    """Print success message in green"""

    print(f"{Colors.GREEN}{msg}{Colors.NC}")


def print_warning(msg: str) -> None:
    """Print warning message in yellow"""

    print(f"{Colors.YELLOW}{msg}{Colors.NC}")


def print_error(msg: str) -> None:
    """Print error message in red"""

    print(f"{Colors.RED}{msg}{Colors.NC}", file=sys.stderr)


def print_header(msg: str) -> None:
    """Print a header message"""

    print("=" * 60)
    print(msg)
    print("=" * 60)


def run_command(
    cmd: List[str], cwd: Optional[Path] = None, env: Optional[dict] = None
) -> Tuple[int, str, str]:
    """
    Run a command and return the exit code, stdout, and stderr

    Args:
        cmd: Command and arguments as a list
        cwd: Working directory for the command
        env: Environment variables for the command

    Returns:
        Tuple of (exit_code, stdout, stderr)
    """

    try:
        result = subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        return result.returncode, result.stdout, result.stderr
    except FileNotFoundError:
        return 1, "", f"Command not found: {cmd[0]}"


def check_command(cmd: str) -> bool:
    """Check if a command is available in PATH"""

    return shutil.which(cmd) is not None


def ask_yes_no(prompt: str, default: bool = True, color: str = Colors.YELLOW) -> bool:
    """
    Ask user a yes/no question

    Args:
        prompt: Question to ask
        default: Default answer if user just presses Enter
        color: ANSI color code for prompt (default: yellow)

    Returns:
        True for yes, False for no
    """

    suffix = "[Y/n]" if default else "[y/N]"
    while True:
        try:
            response = input(f"{color}{prompt} {suffix} {Colors.NC}").strip().lower()
        except (KeyboardInterrupt, EOFError):
            print()
            return False

        if not response:
            return default
        if response in ("y", "yes"):
            return True
        if response in ("n", "no"):
            return False
        print("Please answer yes or no.")


def get_cpu_count() -> int:
    """Get the number of CPU cores"""
    try:
        return os.cpu_count() or 4
    except Exception:
        return 4


def read_cmake_cache(build_dir: Path) -> dict:
    """Read CMakeCache.txt and return a dictionary of cache variables"""
    cache_file = build_dir / "CMakeCache.txt"
    if not cache_file.exists():
        return {}

    cache = {}
    try:
        with open(cache_file, "r") as f:
            for line in f:
                line = line.strip()
                # Skip comments and empty lines
                if not line or line.startswith("#") or line.startswith("//"):
                    continue
                # Parse cache entries: NAME:TYPE=VALUE
                if "=" in line and ":" in line:
                    parts = line.split("=", 1)
                    if len(parts) == 2:
                        key_type = parts[0]
                        value = parts[1]
                        # Extract just the variable name (before the colon)
                        key = key_type.split(":")[0]
                        cache[key] = value
    except Exception as e:
        print_warning(f"Failed to read CMakeCache.txt: {e}")
        return {}

    return cache


class BuildConfig:
    """Configuration for the build"""

    def __init__(self):
        self.script_dir = Path(__file__).parent.resolve()
        self.project_root = self.script_dir.parent

        # Build configuration
        self.build_type: Optional[str] = None
        self.compiler: Optional[str] = None
        self.cxx_compiler: Optional[str] = None
        self.build_system: Optional[str] = None
        self.parallel_jobs = get_cpu_count()
        self.vs_version: Optional[str] = (
            None  # Visual Studio version (2022, 2019, etc.)
        )

        # Module configuration
        self.core_only: Optional[bool] = None
        self.build_tests: Optional[bool] = None
        self.build_examples: Optional[bool] = None

        # Additional options
        self.clean_build = False
        self.conan_toolchain: Optional[Path] = None
        self.use_conan: Optional[bool] = None
        self.interactive = True
        self.cmake_args: str = ""

        # Detected capabilities
        self.is_windows = platform.system() == "Windows"
        self.is_linux = platform.system() == "Linux"
        self.is_macos = platform.system() == "Darwin"

    def get_platform_name(self) -> str:
        """Get lowercase platform name for build directory"""

        if self.is_windows:
            return "windows"
        elif self.is_linux:
            return "linux"
        elif self.is_macos:
            return "macos"
        return "unknown"

    def get_build_dir(self) -> Path:
        """Get the build directory based on build type and platform"""

        build_type_lower = self.build_type.lower() if self.build_type else "release"
        platform_name = self.get_platform_name()
        return self.project_root / "build" / build_type_lower / platform_name

    def get_conan_dir(self) -> Path:
        """Get the Conan installation directory"""

        return self.get_build_dir()


class CompilerDetector:
    """Detect and setup compilers"""

    def __init__(self, config: BuildConfig):
        self.config = config

    def _setup_clang_cl_path(self) -> bool:
        """Ensure clang-cl is in PATH for Windows builds

        Returns:
            True if clang-cl is available or successfully added to PATH
        """
        if not self.config.is_windows:
            return True

        # Check if clang-cl is already in PATH
        if check_command("clang-cl"):
            return True

        # Try to find clang-cl in common locations
        print_info("clang-cl not in PATH, searching common locations...")

        # Check Visual Studio installation paths
        vs_paths = [
            Path("C:/Program Files/Microsoft Visual Studio/2022/Community"),
            Path("C:/Program Files/Microsoft Visual Studio/2022/Professional"),
            Path("C:/Program Files/Microsoft Visual Studio/2022/Enterprise"),
            Path("C:/Program Files (x86)/Microsoft Visual Studio/2019/Community"),
            Path("C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional"),
            Path("C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise"),
        ]

        # Check for clang-cl in VS installations
        for vs_path in vs_paths:
            if not vs_path.exists():
                continue

            # Common locations for clang-cl in VS
            clang_paths = [
                vs_path / "VC" / "Tools" / "Llvm" / "x64" / "bin",
                vs_path / "VC" / "Tools" / "Llvm" / "bin",
            ]

            for clang_path in clang_paths:
                clang_cl = clang_path / "clang-cl.exe"
                if clang_cl.exists():
                    print_success(f"Found clang-cl at: {clang_cl}")
                    # Add to PATH
                    os.environ["PATH"] = (
                        str(clang_path) + os.pathsep + os.environ["PATH"]
                    )

                    # Verify it's now accessible
                    if check_command("clang-cl"):
                        print_success("clang-cl added to PATH successfully")
                        return True
                    else:
                        print_warning("Found clang-cl but failed to add to PATH")

        # Check standalone LLVM installation
        llvm_paths = [
            Path("C:/Program Files/LLVM/bin"),
            Path("C:/Program Files (x86)/LLVM/bin"),
        ]

        for llvm_path in llvm_paths:
            clang_cl = llvm_path / "clang-cl.exe"
            if clang_cl.exists():
                print_success(f"Found clang-cl at: {clang_cl}")
                os.environ["PATH"] = str(llvm_path) + os.pathsep + os.environ["PATH"]

                if check_command("clang-cl"):
                    print_success("clang-cl added to PATH successfully")
                    return True

        print_warning("clang-cl not found in common locations")
        print_info("Please ensure clang-cl is in PATH or use MSVC compiler instead")
        return False

    def detect_msvc(self) -> Tuple[bool, Optional[str]]:
        """Detect MSVC compiler on Windows

        Returns:
            Tuple of (is_available, vs_version) where vs_version is like "2022", "2019", etc.
        """

        if not self.config.is_windows:
            return False, None

        # Try to find Visual Studio using vswhere
        vswhere_path = (
            Path(os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)"))
            / "Microsoft Visual Studio"
            / "Installer"
            / "vswhere.exe"
        )

        if vswhere_path.exists():
            try:
                # Get installation path and version
                result = subprocess.run(
                    [
                        str(vswhere_path),
                        "-latest",
                        "-products",
                        "*",
                        "-requires",
                        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                        "-property",
                        "installationPath",
                    ],
                    capture_output=True,
                    text=True,
                    check=True,
                )
                if result.stdout.strip():
                    vs_path = Path(result.stdout.strip())
                    # Extract version from path (e.g., "2022", "2019")
                    vs_version = None
                    for part in vs_path.parts:
                        if part in ("2022", "2019", "2017"):
                            vs_version = part
                            break

                    vcvars_path = (
                        vs_path / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
                    )
                    if vcvars_path.exists():
                        return True, vs_version
            except subprocess.CalledProcessError:
                pass

        # Fallback: check if cl.exe is in PATH
        if check_command("cl"):
            return True, None

        return False, None

    def setup_msvc_environment(self) -> bool:
        """Setup MSVC environment variables for building with cl.exe or clang-cl"""

        if not self.config.is_windows:
            return True

        # Only needed if using Ninja with MSVC or clang-cl
        if self.config.build_system and "Ninja" not in self.config.build_system:
            return True  # Not using Ninja, no need to setup environment

        if self.config.compiler not in ("msvc", "cl", "clang-cl"):
            return True  # Not using MSVC or clang-cl, no need to setup environment

        # For clang-cl, we need MSVC tools (mt.exe, link.exe, etc.)
        # Check if we already have the MSVC environment set up
        if self.config.compiler == "clang-cl":
            # Check for mt.exe which is needed by clang-cl
            if check_command("mt") or check_command("mt.exe"):
                print_info("MSVC tools already in PATH for clang-cl")
                return True
        elif check_command("cl"):
            # For MSVC, if cl is available, we're good
            print_info("cl.exe already in PATH")
            return True

        # Try to locate vswhere in common locations
        vswhere_path = (
            Path(os.environ.get("ProgramFiles(x86)", "C:\\Program Files (x86)"))
            / "Microsoft Visual Studio"
            / "Installer"
            / "vswhere.exe"
        )
        fallback_vswhere = (
            Path(os.environ.get("ProgramFiles", "C:\\Program Files"))
            / "Microsoft Visual Studio"
            / "Installer"
            / "vswhere.exe"
        )
        if not vswhere_path.exists() and fallback_vswhere.exists():
            vswhere_path = fallback_vswhere

        vs_path = ""
        if vswhere_path.exists():
            try:
                # Prefer an installation with the VC tools
                result = subprocess.run(
                    [
                        str(vswhere_path),
                        "-latest",
                        "-products",
                        "*",
                        "-requires",
                        "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                        "-property",
                        "installationPath",
                    ],
                    capture_output=True,
                    text=True,
                    check=True,
                )
                vs_path = result.stdout.strip()
            except subprocess.CalledProcessError as e:
                print_warning(
                    f"vswhere returned non-zero exit code: {e}; will try common install locations"
                )
                vs_path = ""

        # If vswhere didn't find anything, try some common Visual Studio paths
        if not vs_path:
            common_paths = [
                Path("C:/Program Files/Microsoft Visual Studio/2022/Community"),
                Path("C:/Program Files/Microsoft Visual Studio/2022/Professional"),
                Path("C:/Program Files/Microsoft Visual Studio/2022/Enterprise"),
                Path("C:/Program Files (x86)/Microsoft Visual Studio/2019/Community"),
                Path(
                    "C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional"
                ),
                Path("C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise"),
            ]
            for p in common_paths:
                if p.exists():
                    vs_path = str(p)
                    break

        if not vs_path:
            print_warning(
                "Visual Studio installation not found (vswhere not present or failed, and no common install detected)."
            )
            print_info(
                "Please run this script from a Visual Studio Developer Command Prompt or install the 'Desktop development with C++' workload."
            )
            return False

        # Look for common developer scripts under the installation
        vcvars_candidates = [
            Path(vs_path) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat",
            Path(vs_path) / "Common7" / "Tools" / "VsDevCmd.bat",
            Path(vs_path) / "Common7" / "Tools" / "vcvars64.bat",
        ]
        vcvars = None
        for c in vcvars_candidates:
            if c.exists():
                vcvars = c
                break

        if not vcvars:
            print_error(
                f"VC environment script not found under Visual Studio installation: {vs_path}"
            )
            return False

        print_info(f"Found Visual Studio at: {vs_path}")
        print_info(f"Using developer environment script: {vcvars}")

        # Run the developer script and capture the environment variables.
        # We run via cmd.exe and then run 'set' to list environment variables for capture.
        arch = "amd64"
        # Try VsDevCmd (no args) first if using VsDevCmd.bat
        if vcvars.name.lower().startswith("vsdevcmd"):
            cmd = f'"{vcvars}" && set'
        else:
            cmd = f'"{vcvars}" {arch} && set'

        try:
            proc = subprocess.run(
                cmd, shell=True, capture_output=True, text=True, check=True
            )
            out = proc.stdout
            err = proc.stderr
        except subprocess.CalledProcessError as e:
            # Provide more diagnostic info to the user
            stderr_text = e.stderr.strip() if hasattr(e, "stderr") and e.stderr else ""
            print_error(
                f"Failed to run Visual Studio environment script: {e}. Stderr: {stderr_text}"
            )
            return False
        except Exception as e:
            print_error(f"Error invoking Visual Studio environment script: {e}")
            return False

        # Import the captured environment into the current Python process
        for line in out.splitlines():
            if "=" in line:
                key, _, value = line.partition("=")
                # Overwrite or add environment variables
                os.environ[key] = value

        if err:
            # Non-fatal: show warnings from the script if present
            print_warning(f"Developer script stderr: {err.strip()}")

        # Finally, verify that required tools are now visible
        if self.config.compiler == "clang-cl":
            # For clang-cl, verify mt.exe is available
            if check_command("mt") or check_command("mt.exe"):
                print_success("MSVC environment configured successfully for clang-cl")
                return True
            else:
                print_error("mt.exe not found in PATH after environment setup")
                print_info(
                    "clang-cl requires MSVC tools (mt.exe, link.exe). "
                    "Please ensure Visual Studio is properly installed."
                )
                return False
        else:
            # For MSVC, verify cl.exe
            if check_command("cl"):
                print_success("MSVC environment configured successfully")
                return True
            else:
                print_error("cl.exe not found in PATH after environment setup")
                print_info(
                    "If you are running in PowerShell, ensure the Visual Studio Developer Command Prompt was used or that vcvars is compatible with your shell."
                )
                return False

    def detect_gcc(self) -> bool:
        """Detect GCC compiler"""

        return check_command("gcc") and check_command("g++")

    def detect_clang(self) -> Tuple[bool, Optional[str]]:
        """Detect Clang compiler

        Returns:
            Tuple of (is_available, variant) where variant is 'clang-cl' or 'clang'
        """
        # On Windows, check for both clang-cl (MSVC-compatible) and regular clang
        if self.config.is_windows:
            has_clang_cl = check_command("clang-cl")

            # If not found, try to set it up
            if not has_clang_cl:
                self._setup_clang_cl_path()
                has_clang_cl = check_command("clang-cl")

            has_clang = check_command("clang") and check_command("clang++")

            if has_clang_cl and has_clang:
                # Both available, prefer clang-cl for better Windows compatibility
                return True, "clang-cl"
            elif has_clang_cl:
                return True, "clang-cl"
            elif has_clang:
                return True, "clang"
            return False, None
        else:
            # On Unix-like systems, just check for regular clang
            if check_command("clang") and check_command("clang++"):
                return True, "clang"
            return False, None

    def select_compiler_interactive(self) -> bool:
        """Interactively select compiler"""

        if self.config.is_windows:
            has_msvc, vs_version = self.detect_msvc()
            has_clang, clang_variant = self.detect_clang()

            if not has_msvc and not has_clang:
                print_error("No supported compiler found (MSVC or Clang)")
                return False

            if has_msvc and not has_clang:
                self.config.compiler = "msvc"
                self.config.cxx_compiler = "cl"
                if vs_version:
                    print_info(f"Using MSVC compiler (Visual Studio {vs_version})")
                else:
                    print_info("Using MSVC compiler")
                return True

            if has_clang and not has_msvc:
                if clang_variant == "clang-cl":
                    self.config.compiler = "clang-cl"
                    self.config.cxx_compiler = "clang-cl"
                    print_info("Using Clang-CL compiler (MSVC-compatible Clang)")
                else:
                    self.config.compiler = "clang"
                    self.config.cxx_compiler = "clang++"
                    print_info("Using Clang compiler")
                return True

            # Both available
            print_info("Please select the compiler:")
            if vs_version:
                print(f"1) MSVC (Visual Studio {vs_version}, default)")
            else:
                print("1) MSVC (default)")
            if clang_variant == "clang-cl":
                print("2) Clang-CL (MSVC-compatible Clang)")
            else:
                print("2) Clang")
            choice = input("Enter your choice (1 or 2, default: 1): ").strip()

            if choice == "2":
                if clang_variant == "clang-cl":
                    self.config.compiler = "clang-cl"
                    self.config.cxx_compiler = "clang-cl"
                else:
                    self.config.compiler = "clang"
                    self.config.cxx_compiler = "clang++"
            else:
                self.config.compiler = "msvc"
                self.config.cxx_compiler = "cl"
            return True

        elif self.config.is_linux or self.config.is_macos:
            has_gcc = self.detect_gcc()
            has_clang, _ = self.detect_clang()

            if not has_gcc and not has_clang:
                print_error("No supported compiler found (GCC or Clang)")
                return False

            if has_gcc and not has_clang:
                self.config.compiler = "gcc"
                self.config.cxx_compiler = "g++"
                print_info("Using GCC compiler")
                return True

            if has_clang and not has_gcc:
                self.config.compiler = "clang"
                self.config.cxx_compiler = "clang++"
                print_info("Using Clang compiler")
                return True

            # Both available
            print_info("Please select the compiler:")
            print("1) GCC (default)")
            print("2) Clang")
            choice = input("Enter your choice (1 or 2, default: 1): ").strip()

            if choice == "2":
                self.config.compiler = "clang"
                self.config.cxx_compiler = "clang++"
            else:
                self.config.compiler = "gcc"
                self.config.cxx_compiler = "g++"
            return True

        else:
            print_error("Unsupported platform")
            return False

    def setup_compiler(self) -> bool:
        """Setup compiler based on configuration or interactively"""

        if self.config.compiler:
            # Validate specified compiler
            if self.config.is_windows:
                if self.config.compiler in ("msvc", "cl"):
                    has_msvc, vs_version = self.detect_msvc()
                    if not has_msvc:
                        print_error("MSVC compiler not found")
                        return False
                    self.config.compiler = "msvc"
                    self.config.cxx_compiler = "cl"
                    self.config.vs_version = vs_version
                elif self.config.compiler in ("clang", "clang-cl"):
                    # First ensure clang-cl is in PATH if on Windows
                    if self.config.compiler == "clang-cl":
                        self._setup_clang_cl_path()

                    has_clang, clang_variant = self.detect_clang()
                    if not has_clang:
                        print_error("Clang compiler not found")
                        return False

                    # If user specified clang-cl but only regular clang is available, or vice versa
                    if self.config.compiler == "clang-cl":
                        if clang_variant != "clang-cl" and not check_command(
                            "clang-cl"
                        ):
                            print_error(
                                "clang-cl not found. Install LLVM with clang-cl support."
                            )
                            print_info("You can install it via:")
                            print_info(
                                "  - Visual Studio Installer (C++ Clang tools for Windows)"
                            )
                            print_info(
                                "  - Standalone LLVM from https://releases.llvm.org/"
                            )
                            return False
                        self.config.compiler = "clang-cl"
                        self.config.cxx_compiler = "clang-cl"
                    else:  # regular clang
                        if clang_variant == "clang-cl" and not (
                            check_command("clang") and check_command("clang++")
                        ):
                            print_warning(
                                "Regular clang not found, using clang-cl instead"
                            )
                            self.config.compiler = "clang-cl"
                            self.config.cxx_compiler = "clang-cl"
                        else:
                            self.config.compiler = "clang"
                            self.config.cxx_compiler = "clang++"
                else:
                    print_error(f"Unknown compiler: {self.config.compiler}")
                    return False
            else:
                if self.config.compiler == "gcc":
                    if not self.detect_gcc():
                        print_error("GCC compiler not found")
                        return False
                    self.config.cxx_compiler = "g++"
                elif self.config.compiler == "clang":
                    has_clang, _ = self.detect_clang()
                    if not has_clang:
                        print_error("Clang compiler not found")
                        return False
                    self.config.cxx_compiler = "clang++"
                else:
                    print_error(f"Unknown compiler: {self.config.compiler}")
                    return False
        else:
            # Try to read from existing CMake cache first
            build_dir = self.config.get_build_dir()
            cmake_cache = read_cmake_cache(build_dir)

            if cmake_cache and "CMAKE_CXX_COMPILER" in cmake_cache:
                # Use compiler from cache
                cxx_compiler = cmake_cache["CMAKE_CXX_COMPILER"]
                if "g++" in cxx_compiler or "gcc" in cxx_compiler:
                    self.config.compiler = "gcc"
                    self.config.cxx_compiler = "g++"
                    print_info(f"Using compiler from cache: gcc")
                elif "clang-cl" in cxx_compiler:
                    self.config.compiler = "clang-cl"
                    self.config.cxx_compiler = "clang-cl"
                    print_info(f"Using compiler from cache: clang-cl")
                elif "clang++" in cxx_compiler or "clang" in cxx_compiler:
                    self.config.compiler = "clang"
                    self.config.cxx_compiler = "clang++"
                    print_info(f"Using compiler from cache: clang")
                elif (
                    "cl.exe" in cxx_compiler
                    or "cl" == Path(cxx_compiler).name
                    or "msvc" in cxx_compiler.lower()
                ):
                    self.config.compiler = "msvc"
                    self.config.cxx_compiler = "cl"
                    print_info(f"Using compiler from cache: msvc")
                else:
                    print_warning(
                        f"Unknown compiler in cache: {cxx_compiler}, will auto-detect"
                    )
                    cmake_cache = {}  # Force auto-detection

            # Auto-detect or interactive selection if no cache found
            if not cmake_cache or "CMAKE_CXX_COMPILER" not in cmake_cache:
                if self.config.interactive:
                    if not self.select_compiler_interactive():
                        return False
                else:
                    # Non-interactive: auto-detect compiler
                    # Prefer GCC over Clang on Linux for consistency
                    if self.config.is_windows:
                        has_msvc, vs_version = self.detect_msvc()
                        has_clang, clang_variant = self.detect_clang()

                        if has_msvc:
                            self.config.compiler = "msvc"
                            self.config.cxx_compiler = "cl"
                            self.config.vs_version = vs_version
                            print_info("Auto-detected MSVC compiler")
                        elif has_clang:
                            if clang_variant == "clang-cl":
                                self.config.compiler = "clang-cl"
                                self.config.cxx_compiler = "clang-cl"
                                print_info("Auto-detected Clang-CL compiler")
                            else:
                                self.config.compiler = "clang"
                                self.config.cxx_compiler = "clang++"
                                print_info("Auto-detected Clang compiler")
                        else:
                            print_error("No C++ compiler found")
                            return False
                    else:
                        # Try GCC first, then Clang (prefer stability)
                        if self.detect_gcc():
                            self.config.compiler = "gcc"
                            self.config.cxx_compiler = "g++"
                            print_info("Auto-detected GCC compiler")
                        elif self.detect_clang()[0]:
                            self.config.compiler = "clang"
                            self.config.cxx_compiler = "clang++"
                            print_info("Auto-detected Clang compiler")
                        else:
                            print_error("No C++ compiler found")
                            return False

        print_success(f"Compiler: {self.config.compiler}")
        return True


class BuildSystemSelector:
    """Select and configure build system"""

    def __init__(self, config: BuildConfig):
        self.config = config

    def detect_ninja(self) -> bool:
        """Check if Ninja is available"""

        return check_command("ninja")

    def detect_make(self) -> bool:
        """Check if Make is available"""

        return check_command("make") or check_command("mingw32-make")

    def detect_msbuild(self) -> Tuple[bool, Optional[str]]:
        """Check if MSBuild is available (Windows only)

        Returns:
            Tuple of (is_available, generator_string)
        """

        if not self.config.is_windows:
            return False, None

        # Check if Visual Studio with C++ tools is installed
        has_msvc, vs_version = CompilerDetector(self.config).detect_msvc()
        if not has_msvc:
            return False, None

        # Determine the appropriate Visual Studio generator
        if vs_version == "2022":
            return True, "Visual Studio 17 2022"
        elif vs_version == "2019":
            return True, "Visual Studio 16 2019"
        elif vs_version == "2017":
            return True, "Visual Studio 15 2017"
        else:
            # Fallback: check for msbuild in PATH
            if check_command("msbuild"):
                return True, "Visual Studio 17 2022"  # Default to latest
            return False, None

    def select_build_system_interactive(self) -> bool:
        """Interactively select build system"""

        if self.config.is_windows:
            has_msbuild, msbuild_generator = self.detect_msbuild()
            has_ninja = self.detect_ninja()

            if not has_msbuild and not has_ninja:
                print_error("No supported build system found (MSBuild or Ninja)")
                return False

            if has_ninja and not has_msbuild:
                self.config.build_system = "Ninja"
                print_info("Using Ninja build system")
                return True

            if has_msbuild and not has_ninja:
                self.config.build_system = msbuild_generator
                print_info(f"Using {msbuild_generator}")
                return True

            # Both available
            print_info("Please select the build system:")
            print("1) MSBuild")
            print("2) Ninja (faster, default)")
            choice = input("Enter your choice (1 or 2, default: 2): ").strip()

            if choice == "1":
                self.config.build_system = msbuild_generator
            else:
                self.config.build_system = "Ninja"
            return True

        elif self.config.is_linux or self.config.is_macos:
            has_ninja = self.detect_ninja()
            has_make = self.detect_make()

            if not has_ninja and not has_make:
                print_error("No supported build system found (Ninja or Make)")
                return False

            if has_ninja and not has_make:
                self.config.build_system = "Ninja"
                print_info("Using Ninja build system")
                return True

            if has_make and not has_ninja:
                self.config.build_system = "Make"
                print_info("Using Make build system")
                return True

            # Both available
            print_info("Please select the build system:")
            print("1) Make")
            print("2) Ninja (faster, default)")
            choice = input("Enter your choice (1 or 2, default: 2): ").strip()

            if choice == "2":
                self.config.build_system = "Make"
            else:
                self.config.build_system = "Ninja"
            return True

        else:
            print_error("Unsupported platform")
            return False

    def setup_build_system(self) -> bool:
        """Setup build system based on configuration or interactively"""

        if self.config.build_system:
            # Validate specified build system
            build_sys_lower = self.config.build_system.lower()

            if build_sys_lower == "ninja":
                if not self.detect_ninja():
                    print_error("Ninja not found")
                    return False
                self.config.build_system = "Ninja"
            elif build_sys_lower in ("make", "unix makefiles"):
                if not self.detect_make():
                    print_error("Make not found")
                    return False
                self.config.build_system = "Unix Makefiles"
            elif build_sys_lower in ("msbuild", "visual studio"):
                has_msbuild, msbuild_generator = self.detect_msbuild()
                if not has_msbuild:
                    print_error("MSBuild not found")
                    return False
                self.config.build_system = msbuild_generator
        else:
            # Interactive or auto-detect
            if self.config.interactive:
                if not self.select_build_system_interactive():
                    return False
            else:
                # Non-interactive: auto-detect build system
                if self.config.is_windows:
                    if self.detect_ninja():
                        self.config.build_system = "Ninja"
                        print_info("Auto-detected Ninja build system")
                    else:
                        has_msbuild, msbuild_generator = self.detect_msbuild()
                        if has_msbuild:
                            self.config.build_system = msbuild_generator
                            print_info(f"Auto-detected MSBuild ({msbuild_generator})")
                        else:
                            print_error("No supported build system found")
                            return False
                else:
                    if self.detect_ninja():
                        self.config.build_system = "Ninja"
                        print_info("Auto-detected Ninja build system")
                    elif self.detect_make():
                        self.config.build_system = "Unix Makefiles"
                        print_info("Auto-detected Make")
                    else:
                        print_error("No supported build system found")
                        return False

        print_success(f"Build system: {self.config.build_system}")
        return True


class ConanManager:
    """Manage Conan dependencies"""

    def __init__(self, config: BuildConfig):
        self.config = config

    def find_conan_toolchain(self) -> Optional[Path]:
        """Try to find Conan toolchain file in build directory"""

        # Nested structure: build/<build_type>/<platform>/
        conan_dir = self.config.get_conan_dir()
        toolchain = conan_dir / "conan_toolchain.cmake"

        if toolchain.exists():
            return toolchain

        return None

    def ask_use_conan(self) -> bool:
        """Ask user if they want to use Conan"""

        if self.config.use_conan is not None:
            return self.config.use_conan

        # Check if Conan toolchain already exists (reconfiguring)
        conan_toolchain = self.find_conan_toolchain()
        if conan_toolchain:
            # Conan was used before, keep using it
            return True

        if not self.config.interactive:
            # Non-interactive and no existing Conan setup - don't use Conan
            return False

        print_header("Conan Dependency Management")
        print_info("Conan can automatically manage and build dependencies.")
        print_info("Alternative: use system packages or CPM (automatic download).")
        print_info("")

        return ask_yes_no("Do you want to use Conan for dependencies?", default=True)

    def setup_conan(self) -> bool:
        """Setup Conan dependencies if needed"""

        # Ask if user wants to use Conan
        use_conan = self.ask_use_conan()

        if not use_conan:
            print_info("Skipping Conan, will use system packages or CPM")
            self.config.use_conan = False
            return True

        self.config.use_conan = True

        # Try to find existing Conan toolchain
        conan_toolchain = self.find_conan_toolchain()

        if conan_toolchain:
            print_info(f"Found existing Conan toolchain: {conan_toolchain}")

            if self.config.interactive:
                if ask_yes_no(
                    "Do you want to reinstall Conan dependencies?", default=False
                ):
                    conan_toolchain = None
                else:
                    self.config.conan_toolchain = conan_toolchain
                    return True
            else:
                self.config.conan_toolchain = conan_toolchain
                return True

        # Conan toolchain not found - need to install dependencies
        print_warning("Conan dependencies not installed.")
        print_info("You need to run install_deps.py first to install dependencies.")
        print_info("")

        install_script = self.config.script_dir / "install_deps.py"
        if not install_script.exists():
            print_error("install_deps.py not found")
            print_info(
                "Continuing without Conan - will use system dependencies or CPM fallback"
            )
            self.config.use_conan = False
            return True

        if self.config.interactive:
            if ask_yes_no("Do you want to run install_deps.py now?", default=True):
                print_info("Running dependency installer...")

                cmd = [
                    sys.executable,
                    str(install_script),
                    "--use-conan",
                    "--no-interactive",
                    "--build-type",
                    self.config.build_type,
                ]

                # Pass the compiler to install_deps for profile selection
                if self.config.compiler:
                    cmd.extend(["--compiler", self.config.compiler])

                # Pass the generator to Conan
                if self.config.build_system:
                    cmd.extend(["--generator", self.config.build_system])

                result = subprocess.run(cmd)
                if result.returncode != 0:
                    print_error("Dependency installation failed")
                    print_info(
                        "Continuing without Conan - will use system dependencies or CPM fallback"
                    )
                    self.config.use_conan = False
                    return True

                # Try to find Conan toolchain again
                conan_toolchain = self.find_conan_toolchain()
                if conan_toolchain:
                    self.config.conan_toolchain = conan_toolchain
                    print_success("Conan dependencies installed successfully!")
                    return True
                else:
                    print_warning("Conan toolchain not found after installation")
                    print_info("Continuing with system dependencies or CPM fallback")
                    self.config.use_conan = False
                    return True
            else:
                print_info("Skipping Conan installation")
                print_info(
                    "Run 'python scripts/install_deps.py' manually to install dependencies"
                )
                print_info(
                    "Continuing without Conan - will use system dependencies or CPM fallback"
                )
                self.config.use_conan = False
                return True
        else:
            print_warning(
                f"Please run 'python scripts/install_deps.py --use-conan --build-type {self.config.build_type}' first"
            )
            print_info(
                "Continuing without Conan - will use system dependencies or CPM fallback"
            )
            self.config.use_conan = False
            return True


class CMakeBuilder:
    """Handle CMake configuration and building"""

    def __init__(self, config: BuildConfig):
        self.config = config

    def select_build_type_interactive(self) -> None:
        """Interactively select build type"""

        print_info("Please select the build type:")
        print("1) Debug")
        print("2) RelWithDebInfo")
        print("3) Release (default)")
        choice = input("Enter your choice (1, 2, or 3, default: 3): ").strip()

        if choice == "1":
            self.config.build_type = "Debug"
        elif choice == "2":
            self.config.build_type = "RelWithDebInfo"
        else:
            self.config.build_type = "Release"

    def select_modules_interactive(self) -> None:
        """Interactively select whether to build only core or core + modules"""

        print_info("Build configuration:")
        print("1) Core only")
        print("2) Core + all modules (default)")
        choice = input("Enter your choice (1 or 2, default: 2): ").strip()

        if choice == "1":
            self.config.core_only = True
        else:
            self.config.core_only = False

    def select_tests_and_examples_interactive(self) -> None:
        """Interactively select whether to build tests and examples"""

        if self.config.build_tests is None:
            if ask_yes_no("Do you want to build tests?", False, color=""):
                self.config.build_tests = True
            else:
                self.config.build_tests = False

        if self.config.build_examples is None:
            if ask_yes_no("Do you want to build examples?", False, color=""):
                self.config.build_examples = True
            else:
                self.config.build_examples = False

    def configure(self) -> bool:
        """Configure CMake"""

        print_header("Configuring CMake")

        # Check if using Conan and toolchain is available
        if self.config.use_conan and self.config.conan_toolchain:
            print_info("Using Conan toolchain file")

            # Ensure build dir exists (nested structure)
            build_dir = self.config.get_build_dir()
            build_dir.mkdir(parents=True, exist_ok=True)

            cmake_cmd = [
                "cmake",
                "-S",
                str(self.config.project_root),
                "-B",
                str(build_dir),
                f"-DCMAKE_TOOLCHAIN_FILE={str(self.config.conan_toolchain)}",
                "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
            ]

            # Force Conan priority when explicitly using Conan
            cmake_cmd.append("-DHELIOS_USE_CONAN=ON")
            cmake_cmd.append("-DHELIOS_FORCE_CONAN=ON")

            # Add generator if set (e.g. Ninja)
            if self.config.build_system:
                cmake_cmd.extend(["-G", self.config.build_system])

            # Pass build type and module flags
            if self.config.build_type:
                cmake_cmd.append(f"-DCMAKE_BUILD_TYPE={self.config.build_type}")
            if self.config.core_only is not None:
                cmake_cmd.append(
                    f"-DHELIOS_BUILD_CORE_ONLY={'ON' if self.config.core_only else 'OFF'}"
                )
            if self.config.build_tests is not None:
                cmake_cmd.append(
                    f"-DHELIOS_BUILD_TESTS={'ON' if self.config.build_tests else 'OFF'}"
                )
            if self.config.build_examples is not None:
                cmake_cmd.append(
                    f"-DHELIOS_BUILD_EXAMPLES={'ON' if self.config.build_examples else 'OFF'}"
                )

            # Add additional CMake arguments if provided
            if self.config.cmake_args:
                import shlex

                cmake_cmd.extend(shlex.split(self.config.cmake_args))

            print_info(f"Running: {' '.join(cmake_cmd)}")
            result = subprocess.run(cmake_cmd, cwd=self.config.project_root)

            if result.returncode != 0:
                print_error("CMake configuration failed")
                return False

            print_success("CMake configuration successful")
            return True

        # Fallback to manual configuration
        # Ensure build directory exists (using nested structure)
        build_dir = self.config.get_build_dir()
        build_dir.mkdir(parents=True, exist_ok=True)

        # Check for generator change
        cmake_cache = build_dir / "CMakeCache.txt"
        if cmake_cache.exists():
            try:
                cache_content = cmake_cache.read_text()
                for line in cache_content.splitlines():
                    if line.startswith("CMAKE_GENERATOR:INTERNAL="):
                        old_generator = line.split("=", 1)[1]
                        if old_generator != self.config.build_system:
                            print_warning(
                                f"Generator changed from '{old_generator}' to '{self.config.build_system}'"
                            )
                            if self.config.interactive and ask_yes_no(
                                "Do you want to clean the build directory?", True
                            ):
                                self.config.clean_build = True
                            break
            except Exception:
                pass

        # Clean build if requested
        if self.config.clean_build:
            print_info(f"Cleaning build directory: {build_dir}")
            if build_dir.exists():
                shutil.rmtree(build_dir)
            build_dir.mkdir(parents=True, exist_ok=True)

        # Build CMake command
        cmake_cmd = ["cmake"]

        # Source directory
        cmake_cmd.extend(["-S", str(self.config.project_root)])

        # Build directory
        cmake_cmd.extend(["-B", str(build_dir)])

        # Generator
        if self.config.build_system:
            cmake_cmd.extend(["-G", self.config.build_system])

        # Conan toolchain
        if self.config.use_conan and self.config.conan_toolchain:
            cmake_cmd.append(
                f"-DCMAKE_TOOLCHAIN_FILE={str(self.config.conan_toolchain)}"
            )
            cmake_cmd.append("-DHELIOS_USE_CONAN=ON")
            cmake_cmd.append("-DHELIOS_FORCE_CONAN=ON")
        else:
            # Explicitly disable Conan if not using it
            cmake_cmd.append("-DHELIOS_USE_CONAN=OFF")
            cmake_cmd.append("-DHELIOS_FORCE_CONAN=OFF")

        # Build type (only for single-config generators like Ninja and Make)
        # Multi-config generators like Visual Studio use --config at build time
        is_multi_config = (
            self.config.build_system and "Visual Studio" in self.config.build_system
        )
        if self.config.build_type and not is_multi_config:
            cmake_cmd.append(f"-DCMAKE_BUILD_TYPE={self.config.build_type}")

        # Compiler (don't set for Visual Studio generators, they use the toolset)
        if not is_multi_config:
            if self.config.compiler and self.config.compiler not in ("msvc",):
                # Map compiler names to actual compiler executables
                c_compiler = self.config.compiler
                cxx_compiler = self.config.cxx_compiler

                # Handle special cases
                if self.config.compiler == "msvc":
                    c_compiler = "cl"
                    cxx_compiler = "cl"
                elif self.config.compiler == "clang-cl":
                    c_compiler = "clang-cl"
                    cxx_compiler = "clang-cl"

                cmake_cmd.append(f"-DCMAKE_C_COMPILER={c_compiler}")
                cmake_cmd.append(f"-DCMAKE_CXX_COMPILER={cxx_compiler}")

                # Set archiver tools for Clang to support LTO
                # For clang-cl on Windows, use llvm-lib (MSVC-compatible)
                # For native clang, use llvm-ar and llvm-ranlib
                if self.config.compiler == "clang-cl":
                    # clang-cl uses MSVC-compatible llvm-lib
                    if check_command("llvm-lib"):
                        cmake_cmd.append("-DCMAKE_AR=llvm-lib")
                    # No ranlib needed on Windows
                elif self.config.compiler == "clang":
                    if self.config.is_windows:
                        # Native clang on Windows - let CMake handle it
                        pass
                    else:
                        # Unix-like systems with native clang
                        if check_command("llvm-ar"):
                            cmake_cmd.append("-DCMAKE_AR=llvm-ar")
                        if check_command("llvm-ranlib"):
                            cmake_cmd.append("-DCMAKE_RANLIB=llvm-ranlib")

        # Modules
        if self.config.core_only is not None:
            cmake_cmd.append(
                f"-DHELIOS_BUILD_CORE_ONLY={'ON' if self.config.core_only else 'OFF'}"
            )
        if self.config.build_tests is not None:
            cmake_cmd.append(
                f"-DHELIOS_BUILD_TESTS={'ON' if self.config.build_tests else 'OFF'}"
            )
        if self.config.build_examples is not None:
            cmake_cmd.append(
                f"-DHELIOS_BUILD_EXAMPLES={'ON' if self.config.build_examples else 'OFF'}"
            )

        # Export compile commands
        cmake_cmd.append("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")

        # Add additional CMake arguments if provided
        if self.config.cmake_args:
            import shlex

            cmake_cmd.extend(shlex.split(self.config.cmake_args))

        # Run CMake configure
        env = os.environ.copy()

        # On Windows with Conan, set up environment from conanbuild.bat
        if self.config.is_windows and self.config.use_conan:
            build_dir = self.config.get_build_dir()
            conan_env_script = build_dir / "conanbuild.bat"
            if conan_env_script.exists():
                print_info("Setting up Conan environment for MSVC...")
                try:
                    result_env = subprocess.run(
                        f'"{conan_env_script}" && set',
                        shell=True,
                        capture_output=True,
                        text=True,
                        check=True,
                    )
                    # Parse and update environment variables
                    for line in result_env.stdout.splitlines():
                        if "=" in line:
                            key, _, value = line.partition("=")
                            env[key] = value
                    print_success("Conan environment configured")
                except subprocess.CalledProcessError as e:
                    print_warning(
                        "Failed to set up Conan environment, continuing anyway..."
                    )
                    print_warning(f"Error: {e}")

        print_info(f"Running: {' '.join(cmake_cmd)}")
        result = subprocess.run(cmake_cmd, cwd=self.config.project_root, env=env)

        if result.returncode != 0:
            print_error("CMake configuration failed")
            return False

        print_success("CMake configuration successful")
        return True

    def build(self) -> bool:
        """Build the project"""

        print_header("Building Project")

        # Build using nested structure
        build_dir = self.config.get_build_dir()

        # Build command
        cmake_cmd = [
            "cmake",
            "--build",
            str(build_dir),
            "--parallel",
            str(self.config.parallel_jobs),
        ]

        if self.config.build_type:
            cmake_cmd.extend(["--config", self.config.build_type])

        # Run build
        # Use same environment modification for build
        env = os.environ.copy()
        if self.config.conan_toolchain:
            vcpkg_vars = [
                "VCPKG_ROOT",
                "VCPKG_DEFAULT_TRIPLET",
                "VCPKG_INSTALLATION_ROOT",
            ]
            for var in vcpkg_vars:
                env.pop(var, None)

        print_info(f"Running: {' '.join(cmake_cmd)}")
        result = subprocess.run(cmake_cmd, cwd=self.config.project_root, env=env)

        if result.returncode != 0:
            print_error("Build failed")
            return False

        print_success("Build successful")
        return True


def parse_arguments() -> argparse.Namespace:
    """Parse command line arguments"""

    parser = argparse.ArgumentParser(
        description="Configuration script for Helios Engine",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Interactive configuration (prompts for all options)
  python configure.py

  # Non-interactive with specific options
  python configure.py --type Release --compiler gcc --build-system Ninja --use-conan

  # Debug build without Conan
  python configure.py --type Debug --no-conan

  # Core only build
  python configure.py --core-only --no-tests --no-examples
""",
    )

    parser.add_argument(
        "-t",
        "--type",
        choices=["Debug", "Release", "RelWithDebInfo"],
        help="Build type (default: Debug)",
    )
    parser.add_argument(
        "-c",
        "--compiler",
        help="Compiler to use (gcc, clang, clang-cl, msvc)",
    )
    parser.add_argument(
        "-b",
        "--build-system",
        help="Build system (ninja, make, msbuild)",
    )

    parser.add_argument(
        "--core-only",
        action="store_true",
        help="Build only the core library (no modules)",
    )
    parser.add_argument(
        "--tests",
        action="store_true",
        dest="build_tests",
        help="Build tests",
    )
    parser.add_argument(
        "--no-tests",
        action="store_false",
        dest="build_tests",
        help="Don't build tests",
    )
    parser.add_argument(
        "--examples",
        action="store_true",
        dest="build_examples",
        help="Build examples",
    )
    parser.add_argument(
        "--no-examples",
        action="store_false",
        dest="build_examples",
        help="Don't build examples",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean build directory before building",
    )
    parser.add_argument(
        "--no-interactive",
        action="store_true",
        help="Don't ask for user input, use defaults",
    )
    parser.add_argument(
        "--use-conan",
        action="store_true",
        help="Use Conan for dependency management",
    )
    parser.add_argument(
        "--no-conan",
        action="store_true",
        help="Don't use Conan (use system packages or CPM)",
    )
    parser.add_argument(
        "--cmake-args",
        type=str,
        default="",
        help='Additional CMake arguments (e.g., "--cmake-args "-DHELIOS_ENABLE_PROFILING=ON -DHELIOS_BUILD_TESTS=ON"")',
    )

    parser.set_defaults(build_tests=None, build_examples=None)

    return parser.parse_args()


def main() -> int:
    """Main entry point"""

    args = parse_arguments()

    # Create configuration
    config = BuildConfig()

    # Apply command line arguments
    if args.type:
        config.build_type = args.type
    if args.compiler:
        config.compiler = args.compiler
    if args.build_system:
        config.build_system = args.build_system

    config.clean_build = args.clean
    config.interactive = not args.no_interactive
    config.build_tests = args.build_tests
    config.build_examples = args.build_examples

    # Handle Conan options
    if args.use_conan:
        config.use_conan = True
    elif args.no_conan:
        config.use_conan = False

    # Handle module selection
    if args.core_only:
        config.core_only = True

    # Store additional CMake arguments
    config.cmake_args = args.cmake_args

    print_header("Helios Engine Configuration Script")

    # Check dependencies
    if not check_command("cmake"):
        print_error("CMake is not installed. Please install CMake 3.25 or later.")
        return 1

    if not check_command("git"):
        print_warning("Git is not installed. Some features may not work correctly.")

    # Interactive selections if not specified
    cmake_builder = CMakeBuilder(config)

    # === ORDER: modules, tests/examples, build type, compiler, build system, Conan ===
    if config.interactive:
        # 1. Configuration (core only, etc.)
        if config.core_only is None:
            cmake_builder.select_modules_interactive()
        # 2. Tests/examples
        cmake_builder.select_tests_and_examples_interactive()
        # 3. Build type
        if not config.build_type:
            cmake_builder.select_build_type_interactive()
        # 4. Compiler
        compiler_detector = CompilerDetector(config)
        if not config.compiler:
            compiler_detector.select_compiler_interactive()
        if not compiler_detector.setup_compiler():
            return 1
        # 5. Build system
        build_system_selector = BuildSystemSelector(config)
        if not config.build_system:
            build_system_selector.select_build_system_interactive()
        if not build_system_selector.setup_build_system():
            return 1
        # Setup MSVC environment if needed (for Ninja + MSVC/clang-cl)
        if not compiler_detector.setup_msvc_environment():
            if config.compiler in ("msvc", "clang-cl"):
                print_error("Failed to setup MSVC environment")
                if config.compiler == "clang-cl":
                    print_info("clang-cl requires MSVC environment. Please either:")
                    print_info("  1. Run from 'Developer Command Prompt for VS 2022'")
                    print_info("  2. Use native clang instead: --compiler clang")
                else:
                    print_info(
                        "Please run from a Visual Studio Developer Command Prompt"
                    )
                return 1
    else:
        # Set defaults for non-interactive mode
        if not config.build_type:
            config.build_type = "Release"
        if config.core_only is None:
            config.core_only = False
        if config.build_tests is None:
            config.build_tests = False
        if config.build_examples is None:
            config.build_examples = False
        # Setup compiler
        compiler_detector = CompilerDetector(config)
        if not compiler_detector.setup_compiler():
            return 1
        # Setup build system
        build_system_selector = BuildSystemSelector(config)
        if not build_system_selector.setup_build_system():
            return 1
        # Setup MSVC environment if needed
        if not compiler_detector.setup_msvc_environment():
            if config.compiler in ("msvc", "clang-cl"):
                print_error("Failed to setup MSVC environment")
                if config.compiler == "clang-cl":
                    print_info("clang-cl requires MSVC environment. Please either:")
                    print_info("  1. Run from 'Developer Command Prompt for VS 2022'")
                    print_info("  2. Use native clang instead: --compiler clang")
                else:
                    print_info(
                        "Please run from a Visual Studio Developer Command Prompt"
                    )
                return 1

    # 6. Setup Conan
    conan_manager = ConanManager(config)
    if not conan_manager.setup_conan():
        return 1

    # Configure CMake
    if not cmake_builder.configure():
        return 1

    print_header("Configuration Complete!")
    print_success("CMake configuration successful!")
    print_info("")
    print_info("Next steps:")
    print_info("  Build the project:")
    print_info(f"    python scripts/build.py")
    print_info("  Or use CMake directly:")
    print_info(f"    cmake --build {config.get_build_dir()}")
    print_info("")
    return 0


if __name__ == "__main__":
    sys.exit(main())
