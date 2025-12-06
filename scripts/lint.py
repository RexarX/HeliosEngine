#!/usr/bin/env python3
"""
HeliosEngine Code Linting Tool

This script runs clang-tidy on C++ source code for static analysis.
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path
from typing import List


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
    """Print info message in blue"""

    print(f"{Colors.BLUE}{msg}{Colors.NC}")


def print_success(msg: str) -> None:
    """Print success message in green"""

    print(f"{Colors.GREEN}{msg}{Colors.NC}")


def print_warning(msg: str) -> None:
    """Print warning message in yellow"""

    print(f"{Colors.YELLOW}{msg}{Colors.NC}")


def print_error(msg: str) -> None:
    """Print error message in red"""

    print(f"{Colors.RED}{msg}{Colors.NC}", file=sys.stderr)


def check_clang_tidy() -> bool:
    """Check if clang-tidy is installed"""

    try:
        subprocess.run(
            ["clang-tidy", "--version"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def find_source_files(
    source_dirs: List[Path], extensions: List[str], exclude_dirs: List[str]
) -> List[Path]:
    """Find all source files in the given directories"""

    source_files = []

    for source_dir in source_dirs:
        if not source_dir.exists():
            print_warning(f"Directory does not exist: {source_dir}")
            continue

        print_info(f"Scanning directory: {source_dir}")

        for ext in extensions:
            for file_path in source_dir.rglob(f"*.{ext}"):
                # Check if file is in an excluded directory
                excluded = False
                for exclude_dir in exclude_dirs:
                    try:
                        file_path.relative_to(exclude_dir)
                        excluded = True
                        break
                    except ValueError:
                        pass

                if not excluded:
                    source_files.append(file_path)

    return sorted(source_files)


def lint_file(file_path: Path, build_dir: Path, fix: bool = False) -> bool:
    """
    Lint a single file using clang-tidy

    Args:
        file_path: Path to the file to lint
        build_dir: Path to the build directory containing compile_commands.json
        fix: If True, automatically fix issues

    Returns:
        True if linting passed, False otherwise
    """

    try:
        cmd = ["clang-tidy", f"-p={build_dir}", str(file_path)]
        if fix:
            cmd.append("--fix")

        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )

        # Print output
        if result.stdout:
            print(result.stdout)

        return result.returncode == 0
    except subprocess.CalledProcessError as e:
        print_error(f"Failed to lint {file_path}: {e}")
        return False


def find_compile_commands(base_dir: Path) -> Path:
    """
    Find compile_commands.json in the build directory or its subdirectories.
    Searches common locations like build/conan/build for Conan layouts.

    Args:
        base_dir: Base build directory to search from

    Returns:
        Path to directory containing compile_commands.json

    Raises:
        FileNotFoundError: If compile_commands.json is not found
    """

    # Check direct path first
    if (base_dir / "compile_commands.json").exists():
        return base_dir

    # Check common nested locations (Conan, multi-config generators, etc.)
    common_subdirs = [
        "build",
        "debug",
        "relwithdebinfo",
        "release",
    ]

    for subdir in common_subdirs:
        candidate = base_dir / subdir
        if (candidate / "compile_commands.json").exists():
            return candidate

    # Try to find it recursively (limit depth to 3 levels)
    for depth in range(3):
        pattern = "/".join(["*"] * (depth + 1))
        for candidate in base_dir.glob(f"{pattern}/compile_commands.json"):
            return candidate.parent

    raise FileNotFoundError(
        f"compile_commands.json not found in {base_dir} or its subdirectories"
    )


def detect_platform(platform_arg: str = None) -> str:
    """
    Detect the platform name (linux, macos, windows).
    If platform_arg is provided, use it.
    """

    if platform_arg:
        return platform_arg.lower()
    uname = os.uname().sysname if hasattr(os, "uname") else sys.platform
    if uname.lower().startswith("linux"):
        return "linux"
    elif uname.lower().startswith("darwin"):
        return "macos"
    elif sys.platform.startswith("win"):
        return "windows"
    return "unknown"


def main() -> int:
    """Main entry point"""

    parser = argparse.ArgumentParser(
        description="Run clang-tidy on C++ source code for static analysis"
    )
    parser.add_argument(
        "--fix", action="store_true", help="Automatically fix issues where possible"
    )
    parser.add_argument(
        "--platform",
        type=str,
        default=None,
        help="Target platform (linux, macos, windows). Default: auto-detect",
    )
    parser.add_argument(
        "--type",
        type=str,
        default=None,
        help="Build type (debug, release, relwithdebinfo). Default: release",
    )
    parser.add_argument(
        "--build-dir",
        type=str,
        default=None,
        help="Build directory containing compile_commands.json (default: build/<type>/<platform>)",
    )

    args = parser.parse_args()

    # Get script and project directories
    script_dir = Path(__file__).parent.resolve()
    project_root = script_dir.parent

    print_info("HeliosEngine Code Linting Tool")
    print_info("=" * 30)

    # Check if clang-tidy is installed
    if not check_clang_tidy():
        print_error("clang-tidy is not installed. Please install it first.")
        return 1

    # Detect platform and build type
    platform = detect_platform(args.platform)
    build_type = args.type.lower() if args.type else "release"

    # Resolve build directory
    if args.build_dir:
        build_dir = Path(args.build_dir)
        if not build_dir.is_absolute():
            build_dir = project_root / build_dir
    else:
        build_dir = project_root / "build" / build_type / platform
    build_dir = build_dir.resolve()

    print_info(f"Platform: {platform}")
    print_info(f"Build type: {build_type}")
    print_info(f"Searching for compile_commands.json in: {build_dir}")

    # Find compile_commands.json
    try:
        compile_commands_dir = find_compile_commands(build_dir)
        print_success(f"Found compile_commands.json in: {compile_commands_dir}")
    except FileNotFoundError:
        print_error(
            f"compile_commands.json not found in {build_dir} or its subdirectories."
        )
        print_error(
            "Please build the project first with CMAKE_EXPORT_COMPILE_COMMANDS=ON."
        )
        return 1

    # Use the directory containing compile_commands.json
    build_dir = compile_commands_dir

    # Define source directories
    source_dirs = [
        project_root / "src",
        # project_root / "tests",
        project_root / "examples",
    ]

    # Define file extensions to process (only source files)
    extensions = ["cpp", "h", "hpp", "inl"]

    # Define directories to exclude
    exclude_dirs = [project_root / "third-party"]

    # Find all source files
    print_info("Finding source files...")
    source_files = find_source_files(source_dirs, extensions, exclude_dirs)

    if not source_files:
        print_warning(
            f"No source files found. Checked directories: {', '.join(str(d) for d in source_dirs)}"
        )
        return 0

    print_info(f"Found {len(source_files)} source files to process.")

    # Process files
    if args.fix:
        print_info("Running clang-tidy with automatic fixes enabled...")
    else:
        print_info("Running clang-tidy...")

    has_errors = False
    for file_path in source_files:
        print_info(f"Linting: {file_path}")
        if not lint_file(file_path, build_dir, args.fix):
            print_error(f"Linting failed for {file_path}")
            has_errors = True

    if not has_errors:
        print_success("All files passed linting successfully.")
        return 0
    else:
        print_error("Linting failed for some files.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
