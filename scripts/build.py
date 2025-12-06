#!/usr/bin/env python3
"""
HeliosEngine Build Script

This script handles building the project after configuration.
It calls configure.py first if needed, then builds the configured project.
"""

import argparse
import json
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
    NC = "\033[0m"  # No Color

    @classmethod
    def disable(cls):
        """Disable colors (for Windows terminals that don't support ANSI)"""
        cls.RED = ""
        cls.GREEN = ""
        cls.YELLOW = ""
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


def get_platform_name() -> str:
    """Get lowercase platform name for build directory"""
    system = platform.system()
    if system == "Windows":
        return "windows"
    elif system == "Linux":
        return "linux"
    elif system == "Darwin":
        return "macos"
    return "unknown"


def get_build_dir(project_root: Path, build_type: str) -> Path:
    """Get the build directory based on build type and platform"""
    build_type_lower = build_type.lower()
    platform_name = get_platform_name()
    return project_root / "build" / build_type_lower / platform_name


def find_configured_build_dir(project_root: Path) -> Optional[Path]:
    """Find the most recently configured build directory"""
    build_root = project_root / "build"
    if not build_root.exists():
        return None

    # Find all configured build directories and return the most recently modified
    configured_dirs = []
    for build_type_dir in build_root.iterdir():
        if build_type_dir.is_dir():
            for platform_dir in build_type_dir.iterdir():
                if platform_dir.is_dir():
                    cmake_cache = platform_dir / "CMakeCache.txt"
                    if cmake_cache.exists():
                        configured_dirs.append(platform_dir)

    if not configured_dirs:
        return None

    # Return the most recently modified directory (by CMakeCache.txt modification time)
    return max(configured_dirs, key=lambda d: (d / "CMakeCache.txt").stat().st_mtime)


def find_all_configured_build_dirs(project_root: Path) -> List[Path]:
    """Find all configured build directories"""
    build_root = project_root / "build"
    if not build_root.exists():
        return []

    configured_dirs = []
    # Look for configured build directories
    for build_type_dir in build_root.iterdir():
        if build_type_dir.is_dir():
            for platform_dir in build_type_dir.iterdir():
                if platform_dir.is_dir():
                    cmake_cache = platform_dir / "CMakeCache.txt"
                    if cmake_cache.exists():
                        configured_dirs.append(platform_dir)

    # Sort by build type (debug, release, relwithdebinfo)
    def sort_key(path: Path):
        build_type = path.parent.name.lower()
        order = {"debug": 0, "release": 1, "relwithdebinfo": 2}
        return order.get(build_type, 999)

    return sorted(configured_dirs, key=sort_key)


def detect_build_type_from_dir(build_dir: Path) -> str:
    """Detect build type from build directory path"""
    # Path structure: build/<build_type>/<platform>/
    try:
        build_type = build_dir.parent.name
        if build_type.lower() in ("debug", "release", "relwithdebinfo"):
            if build_type.lower() == "relwithdebinfo":
                return "RelWithDebInfo"
            return build_type.capitalize()
    except:
        pass
    return "Release"


def is_configured(build_dir: Path) -> bool:
    """Check if CMake has been configured"""
    cmake_cache = build_dir / "CMakeCache.txt"
    return cmake_cache.exists()


def select_build_dir_interactive(
    project_root: Path,
) -> Optional[Tuple[Optional[Path], bool, Optional[str]]]:
    """
    Interactively select what to build.
    Returns: (build_dir, needs_configure, build_type) or None if user wants to exit
    """
    configured_dirs = find_all_configured_build_dirs(project_root)

    if not configured_dirs:
        # No configured directories, proceed with configuration
        return None, True, None

    print_header("Build Directory Selection")
    print_info("Found the following configured build directories:")
    print_info("")

    for i, build_dir in enumerate(configured_dirs, 1):
        build_type = detect_build_type_from_dir(build_dir)
        platform = build_dir.name
        print_info(f"  {i}. {build_type} ({platform})")

    print_info(f"  {len(configured_dirs) + 1}. Configure new build")
    print_info(f"  {len(configured_dirs) + 2}. Reconfigure existing")
    print_info("")

    while True:
        try:
            choice = input(f"{Colors.YELLOW}Select option [1]: {Colors.NC}").strip()
        except (KeyboardInterrupt, EOFError):
            print()
            return None  # User wants to exit

        if not choice:
            choice = "1"

        try:
            idx = int(choice)
            if 1 <= idx <= len(configured_dirs):
                # Build existing configuration
                selected_dir = configured_dirs[idx - 1]
                build_type = detect_build_type_from_dir(selected_dir)
                print_success(f"Selected: {build_type} build")
                return selected_dir, False, build_type
            elif idx == len(configured_dirs) + 1:
                # Configure new build
                print_info("Creating new build configuration...")
                return None, True, None
            elif idx == len(configured_dirs) + 2:
                # Reconfigure existing
                print_info("Select build to reconfigure:")
                for i, build_dir in enumerate(configured_dirs, 1):
                    build_type = detect_build_type_from_dir(build_dir)
                    platform = build_dir.name
                    print_info(f"  {i}. {build_type} ({platform})")

                reconfig_choice = input(
                    f"{Colors.YELLOW}Select build to reconfigure [1]: {Colors.NC}"
                ).strip()
                if not reconfig_choice:
                    reconfig_choice = "1"

                reconfig_idx = int(reconfig_choice)
                if 1 <= reconfig_idx <= len(configured_dirs):
                    selected_dir = configured_dirs[reconfig_idx - 1]
                    build_type = detect_build_type_from_dir(selected_dir)
                    print_success(f"Reconfiguring {build_type} build...")
                    return selected_dir, True, build_type
                else:
                    print_error("Invalid choice")
                    continue
            else:
                print_error("Invalid choice")
                continue
        except ValueError:
            print_error("Invalid choice, please enter a number")
            continue

        break

    return None, True, None


def run_configure(
    args,
    script_dir: Path,
    project_root: Path,
    existing_build_dir: Optional[Path] = None,
) -> Tuple[bool, Optional[Path]]:
    """Run the configure script and return success status and build directory

    Args:
        args: Command-line arguments
        script_dir: Path to scripts directory
        project_root: Path to project root
        existing_build_dir: If reconfiguring, path to existing build directory to preserve settings
    """
    print_header("Configuring Project")

    configure_script = script_dir / "configure.py"
    if not configure_script.exists():
        print_error("configure.py not found!")
        return False, None

    cmd = [sys.executable, str(configure_script)]

    # If reconfiguring an existing build, try to preserve settings from cache
    if existing_build_dir and existing_build_dir.exists():
        cache = read_cmake_cache(existing_build_dir)

        # Read compiler from cache if not specified
        if not args.compiler and "CMAKE_CXX_COMPILER" in cache:
            cxx_compiler = cache["CMAKE_CXX_COMPILER"]
            if "g++" in cxx_compiler or "gcc" in cxx_compiler:
                cmd.extend(["--compiler", "gcc"])
            elif "clang++" in cxx_compiler or "clang" in cxx_compiler:
                cmd.extend(["--compiler", "clang"])
            elif "cl.exe" in cxx_compiler or "msvc" in cxx_compiler.lower():
                cmd.extend(["--compiler", "msvc"])
        # Read build system from cache if not specified
        if not args.build_system and "CMAKE_GENERATOR" in cache:
            generator = cache["CMAKE_GENERATOR"]
            cmd.extend(["--build-system", generator])

        # Read other options from cache if not specified
        if args.core_only is None and "HELIOS_BUILD_CORE_ONLY" in cache:
            if cache["HELIOS_BUILD_CORE_ONLY"] == "ON":
                cmd.append("--core-only")

        if args.build_tests is None and "HELIOS_BUILD_TESTS" in cache:
            if cache["HELIOS_BUILD_TESTS"] == "ON":
                cmd.append("--tests")
            else:
                cmd.append("--no-tests")

        if args.build_examples is None and "HELIOS_BUILD_EXAMPLES" in cache:
            if cache["HELIOS_BUILD_EXAMPLES"] == "ON":
                cmd.append("--examples")
            else:
                cmd.append("--no-examples")

        # Check if Conan was used
        if args.use_conan is None:
            conan_toolchain = existing_build_dir / "conan_toolchain.cmake"
            if conan_toolchain.exists():
                cmd.append("--use-conan")

    # Pass through explicit arguments (these override cache)
    if args.type:
        cmd.extend(["--type", args.type])
    if args.compiler:
        cmd.extend(["--compiler", args.compiler])
    if args.build_system:
        cmd.extend(["--build-system", args.build_system])
    if args.platform:
        cmd.extend(["--platform", args.platform])
    if args.core_only:
        cmd.append("--core-only")
    if args.build_tests is not None:
        if args.build_tests:
            cmd.append("--tests")
        else:
            cmd.append("--no-tests")
    if args.build_examples is not None:
        if args.build_examples:
            cmd.append("--examples")
        else:
            cmd.append("--no-examples")
    if args.use_conan is not None:
        if args.use_conan:
            cmd.append("--use-conan")
        else:
            cmd.append("--no-conan")
    if args.clean:
        cmd.append("--clean")
    if args.no_interactive:
        cmd.append("--no-interactive")
    if args.cmake_args:
        cmd.extend(["--cmake-args", args.cmake_args])

    print_info(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd)

    if result.returncode == 0:
        # Find the configured build directory
        build_dir = find_configured_build_dir(project_root)
        return True, build_dir

    return False, None


def setup_msvc_environment() -> dict:
    """Setup MSVC environment for building on Windows.

    Returns:
        Environment dictionary with MSVC variables set, or empty dict on failure.
    """

    env = os.environ.copy()

    if platform.system() != "Windows":
        return env

    # Check if we already have MSVC environment (e.g., running from Developer Command Prompt)
    if shutil.which("cl") and "INCLUDE" in os.environ:
        print_info("MSVC environment already configured")
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
            Path("C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional"),
            Path("C:/Program Files (x86)/Microsoft Visual Studio/2019/Enterprise"),
        ]
        for p in common_paths:
            if p.exists():
                vs_path = str(p)
                break

    if not vs_path:
        print_warning("Visual Studio installation not found")
        return env

    # Find vcvarsall.bat
    vcvars = Path(vs_path) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
    if not vcvars.exists():
        print_warning(f"vcvarsall.bat not found at {vcvars}")
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

        print_success("MSVC environment configured for build")
        return env
    except subprocess.CalledProcessError as e:
        print_warning(f"Failed to setup MSVC environment: {e}")
        return env


def build_project(build_dir: Path, build_type: str, jobs: int) -> bool:
    """Build the project"""
    print_header("Building Project")

    project_root = build_dir.parent.parent.parent

    if not is_configured(build_dir):
        print_error(f"Build directory not configured: {build_dir}")
        print_error("This should not happen - configure.py should have run first")
        return False

    # On Windows, set up build environment
    env = os.environ.copy()
    if platform.system() == "Windows":
        # First check for Conan environment script
        conan_env_script = build_dir / "conanbuild.bat"
        if conan_env_script.exists():
            print_info("Setting up Conan environment for MSVC...")
            # Run conanbuild.bat and capture environment variables
            try:
                result = subprocess.run(
                    f'"{conan_env_script}" && set',
                    shell=True,
                    capture_output=True,
                    text=True,
                    check=True,
                )
                # Parse and update environment variables
                for line in result.stdout.splitlines():
                    if "=" in line:
                        key, _, value = line.partition("=")
                        env[key] = value
                print_success("Conan environment configured")
            except subprocess.CalledProcessError as e:
                print_warning(
                    "Failed to set up Conan environment, trying MSVC setup..."
                )
                env = setup_msvc_environment()
        else:
            # No Conan, set up MSVC environment directly
            env = setup_msvc_environment()

    # Build command
    cmd = [
        "cmake",
        "--build",
        str(build_dir),
        "--parallel",
        str(jobs),
    ]

    # Add config for multi-config generators
    cmd.extend(["--config", build_type])

    print_info(f"Build directory: {build_dir}")
    print_info(f"Running: {' '.join(cmd)}")

    result = subprocess.run(cmd, cwd=project_root, env=env)

    if result.returncode != 0:
        print_error("Build failed!")
        return False

    print_success("Build successful!")
    print_info("")
    return True


def parse_arguments():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(
        description="Build script for Helios Engine",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Build with default settings (calls configure.py first)
  python build.py

  # Build with specific configuration
  python build.py --type Release --compiler gcc --use-conan

  # Build core only with tests
  python build.py --core-only --tests --no-examples

  # Non-interactive build
  python build.py --type Debug --no-interactive

  # Clean build
  python build.py --clean
""",
    )

    # Build type
    parser.add_argument(
        "-t",
        "--type",
        choices=["Debug", "Release", "RelWithDebInfo"],
        help="Build type",
    )

    # Compiler
    parser.add_argument(
        "-c",
        "--compiler",
        choices=["gcc", "clang", "clang-cl", "msvc", "cl"],
        help="Compiler to use",
    )

    # Build system
    parser.add_argument(
        "-b",
        "--build-system",
        choices=["Ninja", "Unix Makefiles", "Visual Studio 17 2022"],
        help="CMake generator",
    )

    # Platform
    parser.add_argument(
        "--platform",
        choices=["linux", "windows", "macos"],
        help="Target platform (default: auto-detect)",
    )

    # Module options
    parser.add_argument(
        "--core-only",
        action="store_true",
        help="Build core module only",
    )

    parser.add_argument(
        "--tests",
        dest="build_tests",
        action="store_true",
        help="Enable tests",
    )

    parser.add_argument(
        "--no-tests",
        dest="build_tests",
        action="store_false",
        help="Disable tests",
    )

    parser.add_argument(
        "--examples",
        dest="build_examples",
        action="store_true",
        help="Enable examples",
    )

    parser.add_argument(
        "--no-examples",
        dest="build_examples",
        action="store_false",
        help="Disable examples",
    )

    # Conan options
    parser.add_argument(
        "--use-conan",
        dest="use_conan",
        action="store_true",
        help="Use Conan for dependencies",
    )

    parser.add_argument(
        "--no-conan",
        dest="use_conan",
        action="store_false",
        help="Don't use Conan (use system packages or CPM)",
    )

    # Build options
    parser.add_argument(
        "-j",
        "--jobs",
        type=int,
        default=get_cpu_count(),
        help="Number of parallel jobs (default: CPU count)",
    )

    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean build directory before building",
    )

    parser.add_argument(
        "--no-interactive",
        action="store_true",
        help="Don't prompt for input, use defaults or command line args",
    )

    parser.add_argument(
        "--skip-configure",
        action="store_true",
        help="Skip configuration step (build only)",
    )

    # Code quality options
    parser.add_argument(
        "--format",
        action="store_true",
        help="Format code using clang-format before building",
    )

    parser.add_argument(
        "--lint",
        action="store_true",
        help="Run clang-tidy after building",
    )

    parser.add_argument(
        "--cmake-args",
        type=str,
        default="",
        help='Additional CMake arguments (e.g., "--cmake-args "-DHELIOS_ENABLE_PROFILING=ON -DHELIOS_BUILD_TESTS=ON"")',
    )

    parser.set_defaults(build_tests=None, build_examples=None, use_conan=None)

    return parser.parse_args()


def main() -> int:
    """Main entry point"""
    args = parse_arguments()

    script_dir = Path(__file__).parent.resolve()
    project_root = script_dir.parent

    print_header("Helios Engine Build Script")

    # Format code if requested
    if args.format:
        print_header("Formatting Code")
        format_script = script_dir / "format.py"
        if format_script.exists():
            result = subprocess.run([sys.executable, str(format_script)])
            if result.returncode != 0:
                print_error("Code formatting failed")
                return 1
        else:
            print_warning("format.py not found, skipping code formatting")

    # Determine build directory and type
    build_dir = None
    build_type = args.type if args.type else "Release"
    needs_configure = False

    # Check if any configuration options are provided via command line
    has_config_args = any(
        [
            args.type,
            args.compiler,
            args.build_system,
            args.core_only,
            args.build_tests is not None,
            args.build_examples is not None,
            args.use_conan is not None,
            args.clean,
        ]
    )

    # Interactive mode: show build directory selection if no args and not skip_configure
    if not has_config_args and not args.skip_configure and not args.no_interactive:
        result = select_build_dir_interactive(project_root)
        if result is None:
            # User cancelled
            print_info("Build cancelled")
            return 0

        build_dir, needs_configure, selected_build_type = result
        if selected_build_type:
            build_type = selected_build_type
            # Set args.type for reconfigure case
            args.type = build_type
    else:
        # Non-interactive or has args: use old logic
        # First, try to find any configured build directory
        configured_dir = find_configured_build_dir(project_root)

        # Check if we need to configure
        if args.clean:
            needs_configure = True
        elif configured_dir is None:
            needs_configure = True
        elif args.type:
            # User specified a build type, check if it matches
            expected_dir = get_build_dir(project_root, args.type)
            if configured_dir != expected_dir:
                needs_configure = True

        # Also configure if any configuration options are provided
        if has_config_args:
            needs_configure = True

        if not needs_configure and configured_dir:
            build_dir = configured_dir

    # Run configure if needed
    if not args.skip_configure and needs_configure:
        # Pass existing build_dir for reconfiguration to preserve settings
        success, new_build_dir = run_configure(
            args, script_dir, project_root, build_dir
        )
        if not success:
            return 1
        if new_build_dir is None:
            new_build_dir = find_configured_build_dir(project_root)
            if new_build_dir is None:
                print_error("Failed to find configured build directory")
                return 1
        build_dir = new_build_dir
    elif build_dir is None:
        # Use existing configured directory or default
        configured_dir = find_configured_build_dir(project_root)
        if configured_dir:
            build_dir = configured_dir
        else:
            build_dir = get_build_dir(project_root, build_type)

        if not is_configured(build_dir):
            if args.skip_configure:
                print_error(f"Build directory not configured: {build_dir}")
                print_info("Run without --skip-configure to configure first")
                return 1
            else:
                # No configuration found, run configure
                success, new_build_dir = run_configure(
                    args, script_dir, project_root, None
                )
                if not success:
                    return 1
                if new_build_dir is None:
                    new_build_dir = find_configured_build_dir(project_root)
                    if new_build_dir is None:
                        print_error("Failed to find configured build directory")
                        return 1
                build_dir = new_build_dir

    # Detect build type from directory if not specified
    if not args.type:
        build_type = detect_build_type_from_dir(build_dir)

    # Build the project
    if not build_project(build_dir, build_type, args.jobs):
        return 1

    # Run linting if requested
    if args.lint:
        print_header("Running Linter")
        lint_script = script_dir / "lint.py"
        if lint_script.exists():
            result = subprocess.run(
                [sys.executable, str(lint_script), "--build-dir", str(build_dir)]
            )
            if result.returncode != 0:
                print_warning("Linting found issues")
        else:
            print_warning("lint.py not found, skipping linting")

    print_header("Build Complete!")
    print_success("Project built successfully!")
    print_info("")
    print_info(f"Build directory: {build_dir}")
    print_info("Run tests with:")
    print_info(f"  cd {build_dir} && ctest")
    print_info("")
    return 0


if __name__ == "__main__":
    sys.exit(main())
