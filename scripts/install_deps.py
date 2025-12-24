#!/usr/bin/env python3
"""Helios Engine Dependency Installer

This script handles installation of dependencies for Helios Engine.
It can use Conan or system packages based on user preference.

"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Tuple

# Fix encoding issues on Windows
if platform.system() == "Windows":
    import io
    import sys

    if sys.stdout.encoding != "utf-8":
        sys.stdout = io.TextIOWrapper(
            sys.stdout.buffer, encoding="utf-8", errors="replace"
        )
    if sys.stderr.encoding != "utf-8":
        sys.stderr = io.TextIOWrapper(
            sys.stderr.buffer, encoding="utf-8", errors="replace"
        )


class Colors:
    """ANSI color codes for terminal output"""

    BLUE = ""
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    BOLD = "\033[1m"
    END = "\033[0m"

    @staticmethod
    def disable():
        """Disable colors (for non-terminal output)"""
        Colors.GREEN = ""
        Colors.YELLOW = ""
        Colors.RED = ""
        Colors.BOLD = ""
        Colors.END = ""


# Disable colors on Windows without ANSI support or when piped
if platform.system() == "Windows" and not os.environ.get("ANSICON"):
    Colors.disable()
elif not sys.stdout.isatty():
    Colors.disable()


def print_info(message: str) -> None:
    """Print informational message"""

    print(f"{message}")


def print_success(message: str) -> None:
    """Print success message"""

    print(f"{Colors.GREEN}✓ {message}{Colors.END}")


def print_warning(message: str) -> None:
    """Print warning message"""

    print(f"{Colors.YELLOW}⚠ {message}{Colors.END}")


def print_error(message: str) -> None:
    """Print error message"""

    print(f"{Colors.RED}✗ {message}{Colors.END}", file=sys.stderr)


def print_header(message: str) -> None:
    """Print section header"""

    print(f"\n{Colors.BOLD}{'=' * 60}{Colors.END}")
    print(f"{Colors.BOLD}{message}{Colors.END}")
    print(f"{Colors.BOLD}{'=' * 60}{Colors.END}\n")


def run_command(
    cmd: List[str],
    cwd: Optional[Path] = None,
    check: bool = True,
    live_output: bool = False,
) -> Tuple[int, str, str]:
    """Run a command and return exit code, stdout, stderr"""

    try:
        if live_output:
            # Show output in real-time
            result = subprocess.run(cmd, cwd=cwd, check=check)
            return result.returncode, "", ""
        else:
            result = subprocess.run(
                cmd, cwd=cwd, capture_output=True, text=True, check=check
            )
            return result.returncode, result.stdout, result.stderr
    except subprocess.CalledProcessError as e:
        if live_output:
            return e.returncode, "", ""
        return e.returncode, e.stdout, e.stderr
    except FileNotFoundError:
        return 127, "", f"Command not found: {cmd[0]}"


def check_command(command: str) -> bool:
    """Check if a command is available in PATH"""

    return shutil.which(command) is not None


def ask_yes_no(question: str, default: bool = True, interactive: bool = True) -> bool:
    """Ask a yes/no question"""

    if not interactive:
        return default

    default_str = "Y/n" if default else "y/N"
    while True:
        response = (
            input(f"{Colors.YELLOW}{question} [{default_str}]: {Colors.END}")
            .strip()
            .lower()
        )
        if not response:
            return default
        if response in ("y", "yes"):
            return True
        if response in ("n", "no"):
            return False
        print_error("Please answer 'y' or 'n'")


class DependencyInstaller:
    """Main dependency installer class"""

    def __init__(self, args):
        self.args = args
        self.system = platform.system()
        self.interactive = not args.no_interactive
        self.project_root = Path(__file__).parent.parent.resolve()
        self.package_manager = self._detect_package_manager()
        self._using_mixed_mode = False
        self.detected_compiler = None
        self.detected_compiler_variant = None
        self._msvc_env = None  # Cached MSVC environment

    def get_platform_name(self) -> str:
        """Get lowercase platform name for build directory"""

        if self.system == "Windows":
            return "windows"
        elif self.system == "Linux":
            return "linux"
        elif self.system == "Darwin":
            return "macos"
        return "unknown"

    def get_build_dir(self, build_type: str) -> Path:
        """Get the build directory based on build type and platform"""

        build_type_lower = build_type.lower()
        platform_name = self.get_platform_name()
        return self.project_root / "build" / build_type_lower / platform_name

    def get_conan_dir(self, build_type: str) -> Path:
        """Get the Conan installation directory"""

        return self.get_build_dir(build_type)

    def _detect_package_manager(self) -> Optional[str]:
        """Detect available system package manager"""

        managers = {
            "apt": "apt-get",
            "dnf": "dnf",
            "yum": "yum",
            "pacman": "pacman",
            "zypper": "zypper",
            "brew": "brew",
            "choco": "choco",
            "scoop": "scoop",
        }

        for name, cmd in managers.items():
            if check_command(cmd):
                return name

        return None

    def _setup_msvc_environment(self) -> dict:
        """Setup MSVC environment for building on Windows.

        Returns:
            Environment dictionary with MSVC variables set.
        """

        if self._msvc_env is not None:
            return self._msvc_env

        env = os.environ.copy()

        if self.system != "Windows":
            return env

        # Check if we already have MSVC environment
        if check_command("cl") and "INCLUDE" in os.environ:
            print_info("MSVC environment already configured")
            self._msvc_env = env
            return env

        # Try to locate vswhere
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
            except subprocess.CalledProcessError:
                pass

        # Fallback to common paths
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
            print_warning("Visual Studio installation not found")
            self._msvc_env = env
            return env

        # Find vcvarsall.bat
        vcvars = Path(vs_path) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
        if not vcvars.exists():
            print_warning(f"vcvarsall.bat not found at {vcvars}")
            self._msvc_env = env
            return env

        # Run vcvarsall.bat and capture environment
        try:
            cmd = f'"{vcvars}" amd64 && set'
            proc = subprocess.run(
                cmd, shell=True, capture_output=True, text=True, check=True
            )

            for line in proc.stdout.splitlines():
                if "=" in line:
                    key, _, value = line.partition("=")
                    env[key] = value

            print_success("MSVC environment configured for Conan build")
            self._msvc_env = env
            return env
        except subprocess.CalledProcessError as e:
            print_warning(f"Failed to setup MSVC environment: {e}")
            self._msvc_env = env
            return env

    def _setup_clang_cl_path(self) -> bool:
        """Ensure clang-cl is in PATH for Windows builds

        Returns:
            True if clang-cl is available or successfully added to PATH
        """
        if self.system != "Windows":
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

    def check_python_version(self) -> bool:
        """Check if Python version is sufficient"""

        version = sys.version_info
        if version.major < 3 or (version.major == 3 and version.minor < 8):
            print_error(f"Python 3.8+ required, found {version.major}.{version.minor}")
            return False
        print_success(f"Python {version.major}.{version.minor}.{version.micro}")
        return True

    def check_cmake(self) -> bool:
        """Check if CMake is installed"""

        if not check_command("cmake"):
            print_error("CMake not found. Please install CMake 3.25 or higher")
            return False

        returncode, stdout, _ = run_command(["cmake", "--version"], check=False)
        if returncode == 0:
            version_line = stdout.split("\n")[0]
            print_success(f"CMake: {version_line}")
        return True

    def check_git(self) -> bool:
        """Check if Git is installed"""

        if not check_command("git"):
            print_error("Git not found. Please install Git")
            return False

        returncode, stdout, _ = run_command(["git", "--version"], check=False)
        if returncode == 0:
            print_success(f"Git: {stdout.strip()}")
        return True

    def check_compiler(self) -> bool:
        """Check if a suitable C++ compiler is available"""

        compilers = []

        if check_command("g++"):
            returncode, stdout, _ = run_command(["g++", "--version"], check=False)
            if returncode == 0:
                compilers.append(f"g++: {stdout.split()[2]}")

        if check_command("clang++"):
            returncode, stdout, _ = run_command(["clang++", "--version"], check=False)
            if returncode == 0:
                version = stdout.split("\n")[0].split("version")[1].strip().split()[0]
                compilers.append(f"clang++: {version}")

        if check_command("clang-cl"):
            compilers.append("Clang-CL (clang-cl)")

        if check_command("cl") or check_command("cl.exe"):
            compilers.append("MSVC (cl.exe)")

        if not compilers:
            print_error("No C++ compiler found")
            print_info("Please install one of the following:")
            if self.system == "Linux":
                print_info("  - GCC: sudo apt install g++ (Debian/Ubuntu)")
                print_info("  - GCC: sudo dnf install gcc-c++ (Fedora)")
                print_info("  - GCC: sudo pacman -S gcc (Arch)")
                print_info("  - Clang: sudo apt install clang (Debian/Ubuntu)")
            elif self.system == "Darwin":
                print_info("  - Clang: xcode-select --install")
                print_info("  - GCC: brew install gcc")
            elif self.system == "Windows":
                print_info("  - MSVC: Install Visual Studio 2022")
                print_info("  - Clang: choco install llvm")
            return False

        for compiler in compilers:
            print_success(compiler)

        print_info("Note: C++23 standard will be set during build configuration")
        return True

    def install_conan(self) -> bool:
        """Install or verify Conan installation"""

        if check_command("conan"):
            returncode, stdout, _ = run_command(["conan", "--version"], check=False)
            if returncode == 0:
                print_success(f"Conan: {stdout.strip()}")
                return True

        if not self.args.install_conan:
            if not ask_yes_no(
                "Conan not found. Install it now?",
                default=True,
                interactive=self.interactive,
            ):
                return False

        print_info("Installing Conan via pip...")
        cmd = [sys.executable, "-m", "pip", "install", "--user", "conan"]
        returncode, _, stderr = run_command(cmd, check=False)

        if returncode != 0:
            print_error(f"Failed to install Conan: {stderr}")
            return False

        print_success("Conan installed successfully")
        return True

    def detect_compiler(self) -> Tuple[Optional[str], Optional[str]]:
        """Detect available compiler on the system

        Returns:
            Tuple of (compiler, variant) where:
            - compiler is one of: 'msvc', 'clang', 'gcc'
            - variant is 'clang-cl' for Windows clang-cl, or None
        """
        if self.system == "Windows":
            # Check for MSVC (cl.exe)
            has_msvc = check_command("cl")

            # Check for Clang variants
            has_clang_cl = check_command("clang-cl")
            has_clang = check_command("clang") and check_command("clang++")

            if has_msvc:
                return "msvc", None
            elif has_clang_cl:
                return "clang", "clang-cl"
            elif has_clang:
                return "clang", None

            return None, None
        else:
            # Linux/macOS
            if check_command("gcc") and check_command("g++"):
                return "gcc", None
            elif check_command("clang") and check_command("clang++"):
                return "clang", None

            return None, None

    def select_conan_profile(self) -> str:
        """Select appropriate Conan profile based on compiler and platform

        Returns:
            Profile name or path to use
        """
        # If user specified a profile, use it
        if self.args.profile and self.args.profile != "default":
            profile_path = self.project_root / "conan-profiles" / self.args.profile
            if profile_path.exists():
                print_info(f"Using specified profile: {self.args.profile}")
                return str(profile_path)
            else:
                print_warning(f"Profile not found: {profile_path}")
                print_info("Will use auto-detected profile instead")

        # Use compiler from args if specified (passed from configure.py)
        if hasattr(self.args, "compiler") and self.args.compiler:
            compiler = self.args.compiler
            # Determine variant for clang-cl
            # Respect user's explicit choice - don't auto-switch from clang to clang-cl
            variant = "clang-cl" if compiler == "clang-cl" else None
            self.detected_compiler = compiler
            self.detected_compiler_variant = variant
            print_info(
                f"Using compiler from arguments: {compiler}"
                + (f" ({variant})" if variant else "")
            )
        else:
            # Detect compiler
            compiler, variant = self.detect_compiler()
            self.detected_compiler = compiler
            self.detected_compiler_variant = variant

        if not compiler:
            print_warning("Could not detect compiler, using default profile")
            return "default"

        # Select profile based on platform and compiler
        profiles_dir = self.project_root / "conan-profiles"

        if self.system == "Windows":
            if compiler == "msvc":
                profile = profiles_dir / "msvc-ninja"
                if profile.exists():
                    print_info(f"Using MSVC profile: {profile.name}")
                    return str(profile)
            elif compiler == "clang" or compiler == "clang-cl":
                if variant == "clang-cl" or compiler == "clang-cl":
                    profile = profiles_dir / "clang-cl-windows"
                    if profile.exists():
                        print_info(f"Using Clang-CL profile: {profile.name}")
                        return str(profile)
                else:
                    profile = profiles_dir / "clang-windows"
                    if profile.exists():
                        print_info(f"Using Clang profile: {profile.name}")
                        return str(profile)
        elif self.system == "Linux":
            profile = profiles_dir / "clang-native"
            if profile.exists() and compiler == "clang":
                print_info(f"Using Clang profile: {profile.name}")
                return str(profile)

        # Fallback to default
        print_info(f"Using default Conan profile (compiler: {compiler})")
        return "default"

    def setup_conan_profile(self) -> bool:
        """Set up Conan profile"""

        print_info("Setting up Conan profile...")

        # First, detect the default profile
        cmd = ["conan", "profile", "detect", "--force"]
        returncode, _, stderr = run_command(cmd, check=False)

        if returncode != 0:
            print_error(f"Failed to detect Conan profile: {stderr}")
            return False

        # Select the appropriate profile for this platform/compiler
        self.selected_profile = self.select_conan_profile()

        print_success("Conan profile configured")
        return True

    def _needs_system_tbb(self) -> bool:
        """Check if system TBB is needed for libstdc++ parallel STL.

        On Linux with GCC, libstdc++ uses TBB as the backend for parallel STL
        algorithms. The <algorithm> header pulls in TBB headers which require
        linking against the TBB library.

        Returns:
            True if system TBB package should be installed
        """
        # Only needed on Linux
        if self.system != "Linux":
            return False

        # Check if GCC is the compiler (libstdc++ uses TBB for parallel STL)
        if check_command("g++"):
            return True

        return False

    def _check_system_packages(self) -> dict:
        """Check which dependencies are already available via system package manager

        Returns a dict mapping canonical names to system package names:
        {
            'boost': 'boost',
            'spdlog': 'spdlog',
            'taskflow': 'taskflow'
        }

        Note: TBB is not included here as it's handled separately as a system-only
        dependency for Linux/GCC (libstdc++ parallel STL requirement).
        """

        available = {}

        if not self.package_manager:
            return available

        print_info("Checking for system packages...")

        # Check for key packages based on package manager
        # Note: TBB is not checked here - it's a system-only dependency for Linux/GCC
        if self.package_manager == "pacman":
            # Map canonical names to pacman package names
            # Note: stduuid (util-linux-libs) and taskflow are not available in pacman
            packages_to_check = {
                "boost": "boost",
                "spdlog": "spdlog",
            }
            for canonical_name, pkg in packages_to_check.items():
                returncode, _, _ = run_command(
                    ["pacman", "-Q", pkg], check=False, live_output=False
                )
                if returncode == 0:
                    available[canonical_name] = pkg

        elif self.package_manager == "apt":
            # Note: taskflow in apt is too old (< 3.10), so we don't check for it
            packages_to_check = {
                "boost": "libboost-all-dev",
                "spdlog": "libspdlog-dev",
                "stduuid": "uuid-dev",
            }
            for canonical_name, pkg in packages_to_check.items():
                returncode, _, _ = run_command(
                    ["dpkg", "-s", pkg], check=False, live_output=False
                )
                if returncode == 0:
                    available[canonical_name] = pkg

        elif self.package_manager in ("dnf", "yum"):
            # Note: taskflow availability varies by distro
            packages_to_check = {
                "boost": "boost-devel",
                "spdlog": "spdlog-devel",
                "stduuid": "libuuid-devel",
            }
            for canonical_name, pkg in packages_to_check.items():
                returncode, _, _ = run_command(
                    ["rpm", "-q", pkg], check=False, live_output=False
                )
                if returncode == 0:
                    available[canonical_name] = pkg

        elif self.package_manager == "brew":
            # Note: taskflow 3.10+ is available on homebrew
            packages_to_check = {
                "boost": "boost",
                "spdlog": "spdlog",
                "taskflow": "taskflow",
            }
            for canonical_name, pkg in packages_to_check.items():
                returncode, _, _ = run_command(
                    ["brew", "list", pkg], check=False, live_output=False
                )
                if returncode == 0:
                    available[canonical_name] = pkg

        return available

    def install_conan_dependencies(self) -> bool:
        """Install dependencies using Conan (only missing ones if system packages exist)"""

        print_info("Installing dependencies using Conan...")

        # If using clang-cl, ensure it's in PATH
        if (
            hasattr(self, "detected_compiler_variant")
            and self.detected_compiler_variant == "clang-cl"
        ):
            if not self._setup_clang_cl_path():
                print_warning("clang-cl setup failed, Conan may not build correctly")
        elif hasattr(self.args, "compiler") and self.args.compiler == "clang-cl":
            if not self._setup_clang_cl_path():
                print_warning("clang-cl setup failed, Conan may not build correctly")

        # Check for system packages first
        system_packages = self._check_system_packages()

        # All required dependencies (canonical names matching conanfile.py)
        # Note: TBB is NOT managed via Conan - it's a system package requirement
        # for Linux/GCC because libstdc++ parallel STL uses TBB as backend
        required_deps = [
            "boost",
            "spdlog",
            "stduuid",
            "concurrentqueue",
            "taskflow",
        ]

        # Determine which dependencies are missing
        missing_deps = [dep for dep in required_deps if dep not in system_packages]

        if system_packages:
            print_info("Found system packages:")
            for canonical_name, pkg_name in system_packages.items():
                print_info(f"  ✓ {canonical_name} ({pkg_name})")

        if missing_deps:
            print_info("")
            print_info("Missing dependencies (will be installed via Conan):")
            for dep in missing_deps:
                print_info(f"  • {dep}")
            print_info("")
        else:
            print_info("")
            print_success("All dependencies found in system packages!")
            print_info("")
            print_info("System packages will be used automatically during build.")
            print_info("No need to use Conan - just build normally:")
            print_info("  make build")
            print_info("  # or: cmake -B build && cmake --build build")
            print_info("")
            print_info(
                "Note: System packages are preferred by default for faster builds."
            )
            print_info("")
            return True

        # Ask user about mixed mode
        use_mixed_mode = True
        if system_packages and self.interactive:
            print_info("You have some dependencies from system packages.")
            use_mixed_mode = ask_yes_no(
                "Use system packages where available and Conan for missing ones?",
                default=True,
                interactive=True,
            )
            if not use_mixed_mode:
                print_info(
                    "Installing all dependencies via Conan (ignoring system packages)"
                )
                missing_deps = required_deps

        conanfile = self.project_root / "conanfile.py"
        if not conanfile.exists():
            print_error(f"conanfile.py not found at {conanfile}")
            return False

        # Build options
        cmd = ["conan", "install", str(self.project_root)]
        cmd.extend(["-s", f"build_type={self.args.build_type}"])

        # Set C++23 standard required by Helios Engine
        cmd.extend(["-s", "compiler.cppstd=23"])

        # Set CMake generator - auto-detect if not specified
        generator = self.args.generator
        if not generator:
            # Auto-detect generator based on platform
            if self.system == "Windows":
                # On Windows, prefer Ninja if available, otherwise use Visual Studio
                if check_command("ninja"):
                    generator = "Ninja"
                else:
                    generator = "Visual Studio 17 2022"
                print_info(f"Auto-detected generator: {generator}")
            elif check_command("ninja"):
                generator = "Ninja"
            else:
                generator = "Unix Makefiles"

        if generator:
            cmd.extend(["-c", f"tools.cmake.cmaketoolchain:generator={generator}"])
            if self.system == "Windows" and generator == "Ninja":
                print_info(
                    "Note: Using Ninja on Windows requires proper MSVC environment."
                )
                print_info(
                    "      The build will use Conan's environment setup automatically."
                )

        # Use the selected profile (but override build_type from command line)
        if hasattr(self, "selected_profile") and self.selected_profile != "default":
            cmd.extend(["-pr", self.selected_profile])
        elif self.args.profile and self.args.profile != "default":
            cmd.extend(["-pr", self.args.profile])

        # For clang-cl on Windows, we need to set compiler settings explicitly
        # since profiles use 'clang' compiler type, not 'msvc'
        compiler = getattr(self.args, "compiler", None) or self.detected_compiler
        if self.system == "Windows" and compiler == "clang-cl":
            # Don't override - the profile already sets compiler=clang
            # Just make sure the build executables are set
            cmd.extend(
                [
                    "-c",
                    "tools.build:compiler_executables={'c': 'clang-cl', 'cpp': 'clang-cl'}",
                ]
            )
            # Set VS version for finding Windows SDK
            cmd.extend(["-c", "tools.microsoft.msbuild:vs_version=17"])

        # Output folder - use nested structure: build/<build_type>/<platform>/
        output_folder = self.get_conan_dir(self.args.build_type)
        output_folder.mkdir(parents=True, exist_ok=True)
        cmd.extend(["--output-folder", str(output_folder)])

        # Add build missing
        cmd.append("--build=missing")

        print_info("")
        print_info(f"Running: {' '.join(cmd)}")
        if use_mixed_mode and system_packages:
            print_info("")
            print_info("Note: By default, system packages will be preferred over Conan")
            print_info("      when both are available. Only missing dependencies will")
            print_info("      be provided by Conan.")
            print_info("")
        print_info("This may take a while as dependencies are being built...")

        # On Windows, ensure MSVC environment is set up for Conan builds
        # This is needed because Conan will build dependencies from source
        # and needs access to MSVC tools like mt.exe, link.exe, etc.
        env = None
        if self.system == "Windows":
            env = self._setup_msvc_environment()

        # Run conan install with the MSVC environment
        if env:
            result = subprocess.run(cmd, cwd=self.project_root, env=env)
            returncode = result.returncode
        else:
            returncode, _, _ = run_command(
                cmd, cwd=self.project_root, check=False, live_output=True
            )

        if returncode != 0:
            print_error("Failed to install Conan dependencies")
            return False

        print_success("Conan dependencies installed successfully")
        print_info(f"Conan files installed to: {output_folder}")

        # Store info about mixed mode for later
        self._using_mixed_mode = use_mixed_mode and bool(system_packages)

        # Create/update CMakeUserPresets.json to include Conan presets
        self._setup_cmake_user_presets()

        return True

    def install_system_dependencies(self) -> bool:
        """Install dependencies using system package manager"""

        if not self.package_manager:
            print_warning(
                "No system package manager detected, skipping system package installation"
            )
            return True

        print_info(f"Detected package manager: {self.package_manager}")

        # Define package mappings
        # Note: TBB is included for Linux because libstdc++ (GCC) parallel STL requires it
        packages = {
            "apt": ["libboost-all-dev", "libspdlog-dev", "doctest-dev", "cmake", "git"],
            "dnf": ["boost-devel", "spdlog-devel", "doctest-devel", "cmake", "git"],
            "yum": ["boost-devel", "spdlog-devel", "doctest-devel", "cmake", "git"],
            "pacman": ["boost", "spdlog", "doctest", "cmake", "git"],
            "zypper": ["boost-devel", "spdlog-devel", "doctest-devel", "cmake", "git"],
            "brew": ["boost", "spdlog", "doctest", "cmake", "git"],
            "choco": ["boost-msvc-14.3", "spdlog", "cmake", "git", "llvm"],
            "scoop": ["cmake", "git", "llvm"],
        }

        # TBB package names per package manager (for libstdc++ parallel STL on Linux/GCC)
        tbb_packages = {
            "apt": "libtbb-dev",
            "dnf": "tbb-devel",
            "yum": "tbb-devel",
            "pacman": "onetbb",
            "zypper": "tbb-devel",
        }

        pkg_list = packages.get(self.package_manager, [])
        if not pkg_list:
            print_warning(f"No package list defined for {self.package_manager}")
            return True

        # Add TBB if needed for libstdc++ parallel STL (Linux with GCC)
        if self._needs_system_tbb():
            tbb_pkg = tbb_packages.get(self.package_manager)
            if tbb_pkg:
                pkg_list = pkg_list + [tbb_pkg]
                print_info(
                    f"Including TBB ({tbb_pkg}) - required for libstdc++ parallel STL"
                )

        if not ask_yes_no(
            f"Install system packages using {self.package_manager}?",
            default=True,
            interactive=self.interactive,
        ):
            print_info("Skipping system package installation")
            return True

        # Build install command
        if self.package_manager == "apt":
            cmd = ["sudo", "apt-get", "update"]
            run_command(cmd, check=False)
            cmd = ["sudo", "apt-get", "install", "-y"] + pkg_list
        elif self.package_manager in ("dnf", "yum"):
            cmd = ["sudo", self.package_manager, "install", "-y"] + pkg_list
        elif self.package_manager == "pacman":
            cmd = ["sudo", "pacman", "-S", "--needed", "--noconfirm"] + pkg_list
        elif self.package_manager == "zypper":
            cmd = ["sudo", "zypper", "install", "-y"] + pkg_list
        elif self.package_manager == "brew":
            cmd = ["brew", "install"] + pkg_list
        elif self.package_manager == "choco":
            cmd = ["choco", "install", "-y"] + pkg_list
        elif self.package_manager == "scoop":
            cmd = ["scoop", "install"] + pkg_list
        else:
            return True

        print_info(f"Running: {' '.join(cmd)}")
        returncode, _, stderr = run_command(cmd, check=False)

        if returncode != 0:
            print_warning(f"Some packages may have failed to install: {stderr}")
            return True  # Don't fail completely

        print_success("System dependencies installed successfully")

        return True

    def _setup_cmake_user_presets(self) -> None:
        """Create or update CMakeUserPresets.json to include Conan-generated presets"""

        user_presets_file = self.project_root / "CMakeUserPresets.json"

        # Check if Conan presets file exists
        conan_presets = self.project_root / "build" / "CMakePresets.json"
        if not conan_presets.exists():
            print_warning("Conan CMakePresets.json not found in build directory")
            return

        # Create the user presets file
        user_presets_content = {
            "version": 4,
            "vendor": {"conan": {}},
            "include": ["build/CMakePresets.json"],
        }

        try:
            import json

            with open(user_presets_file, "w") as f:
                json.dump(user_presets_content, f, indent=2)
            print_success("Created CMakeUserPresets.json to include Conan presets")
            print_info(f"  File: {user_presets_file}")
            print_info("  Includes: build/CMakePresets.json")
        except Exception as e:
            print_warning(f"Failed to create CMakeUserPresets.json: {e}")
            print_info("You can manually create it or use presets without Conan")

    def run(self) -> int:
        """Run the installer"""

        print_header("Helios Engine Dependency Installer")

        print_info(f"Platform: {self.system} {platform.machine()}")
        print_info(f"Project root: {self.project_root}")

        # Check prerequisites
        print_header("Checking Prerequisites")

        if not self.check_python_version():
            return 1

        if not self.check_cmake():
            return 1

        if not self.check_git():
            return 1

        if not self.check_compiler():
            return 1

        # Install dependencies
        if self.args.use_conan:
            print_header("Installing Dependencies with Conan")

            if not self.install_conan():
                print_warning(
                    "Conan installation failed, falling back to system packages"
                )
                self.args.use_conan = False
                self.args.use_system = True

        if self.args.use_conan:
            if not self.setup_conan_profile():
                return 1

            if not self.install_conan_dependencies():
                return 1

        if self.args.use_system and not self.args.use_conan:
            print_header("Installing System Dependencies")
            self.install_system_dependencies()

        # Summary
        print_header("Installation Complete")
        print_success("All dependencies have been configured successfully!")
        print_info("")
        print_info("Next steps:")

        # Windows-specific instructions
        if self.system == "Windows" and self.args.use_conan:
            print_info("  Build the project (Conan will set up MSVC environment):")
            print_info("     python scripts/build.py --use-conan")
            print_info("")
            print_info("  Note: On Windows, Conan automatically configures the MSVC")
            print_info("        environment, so no Developer Command Prompt is needed.")
        else:
            print_info("  1. Configure the project:")
            print_info("     python scripts/configure.py")
            print_info("")
            print_info("  2. Build the project:")
            print_info("     python scripts/build.py")
            print_info("")
            print_info("  Or use the Makefile:")
            print_info("     make build")

        print_info("")

        return 0


def main():
    """Main entry point"""

    parser = argparse.ArgumentParser(
        description="Helios Engine Dependency Installer",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Interactive installation (asks about Conan)
  python install_deps.py

  # Install with Conan (recommended)
  python install_deps.py --use-conan

  # Install with system packages only
  python install_deps.py --use-system

  # Non-interactive installation with Conan
  python install_deps.py --use-conan --no-interactive

  # Install Conan automatically if missing
  python install_deps.py --use-conan --install-conan
""",
    )

    parser.add_argument(
        "--use-conan",
        action="store_true",
        help="Use Conan for dependency management (recommended)",
    )
    parser.add_argument(
        "--use-system",
        action="store_true",
        help="Install system packages (in addition to or instead of Conan)",
    )
    parser.add_argument(
        "--no-interactive",
        action="store_true",
        help="Don't ask for user input, use defaults",
    )
    parser.add_argument(
        "--install-conan",
        action="store_true",
        help="Automatically install Conan if not found",
    )
    parser.add_argument(
        "--profile",
        default="default",
        help="Conan profile to use (default: auto-detect based on compiler)",
    )
    parser.add_argument(
        "--compiler",
        choices=["msvc", "clang", "clang-cl", "gcc"],
        help="Compiler to use (default: auto-detect)",
    )
    parser.add_argument(
        "--build-type",
        default="Release",
        choices=["Debug", "Release", "RelWithDebInfo"],
        help="Build type (default: Release)",
    )
    parser.add_argument(
        "--generator",
        default=None,
        choices=["Ninja", "Unix Makefiles", "Visual Studio 17 2022", "Xcode"],
        help="CMake generator for Conan to use (default: auto-detect)",
    )

    args = parser.parse_args()

    # Ask about Conan if not specified
    if not args.use_conan and not args.use_system:
        if not args.no_interactive:
            print_header("Helios Engine Dependency Installer")
            print_info("This script will help you install dependencies.")
            print_info("")
            print_info("Conan can automatically manage and build dependencies.")
            print_info("Alternative: use system packages (if available).")
            print_info("")

            if ask_yes_no("Do you want to use Conan?", default=True, interactive=True):
                args.use_conan = True
            else:
                args.use_system = True
        else:
            # Default to Conan in non-interactive mode
            args.use_conan = True

    installer = DependencyInstaller(args)
    return installer.run()


if __name__ == "__main__":
    sys.exit(main())
