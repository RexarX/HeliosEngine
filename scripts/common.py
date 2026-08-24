#!/usr/bin/env python3
"""
HeliosEngine Dev Script Common Utilities

Shared helpers used by lint.py, format.py, and docs.py: colored console
output, executable discovery, source-file scanning, and small argparse
helpers for tools that shell out to external commands (clang-tidy,
clang-format, doxygen, ...).
"""

import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Sequence


class Colors:
    """ANSI color codes for terminal output"""

    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[1;33m"
    BLUE = "\033[0;34m"
    NC = "\033[0m"  # No Color

    @classmethod
    def disable(cls):
        """Disable colors (for terminals that don't support ANSI)"""

        cls.RED = ""
        cls.GREEN = ""
        cls.YELLOW = ""
        cls.BLUE = ""
        cls.NC = ""


def enable_windows_colors() -> None:
    """Enable ANSI color support on Windows 10+, falling back to disabling
    colors entirely if that isn't possible. Call this once at startup."""

    if sys.platform != "win32":
        return
    try:
        import ctypes

        kernel32 = ctypes.windll.kernel32
        kernel32.SetConsoleMode(kernel32.GetStdHandle(-11), 7)
    except Exception:
        Colors.disable()


def print_header(message: str) -> None:
    """Print a colored, boxed header message"""

    print(f"\n{Colors.GREEN}{'=' * 60}{Colors.NC}")
    print(f"{Colors.GREEN}{message:^60}{Colors.NC}")
    print(f"{Colors.GREEN}{'=' * 60}{Colors.NC}\n")


def print_info(msg: str) -> None:
    """Print info message in blue"""

    print(f"{msg}{Colors.NC}")


def print_success(msg: str) -> None:
    """Print success message in green"""

    print(f"{Colors.GREEN}{msg}{Colors.NC}")


def print_warning(msg: str) -> None:
    """Print warning message in yellow"""

    print(f"{Colors.YELLOW}{msg}{Colors.NC}")


def print_error(msg: str) -> None:
    """Print error message in red"""

    print(f"{Colors.RED}{msg}{Colors.NC}", file=sys.stderr)


def find_executable(name: str) -> Optional[Path]:
    """Find an executable in PATH, returning its Path or None."""

    import shutil

    result = shutil.which(name)
    return Path(result) if result else None


def check_tool_installed(name: str, version_flag: str = "--version") -> bool:
    """Check whether a command-line tool is installed and runnable."""

    try:
        subprocess.run(
            [name, version_flag],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=True,
        )
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def find_source_files(
    source_dirs: Sequence[Path],
    extensions: Sequence[str],
    exclude_dirs: Sequence[Path],
) -> List[Path]:
    """Find all source files with the given extensions under source_dirs,
    skipping anything inside exclude_dirs. Missing directories are skipped
    with a warning rather than raising."""

    source_files = []

    for source_dir in source_dirs:
        if not source_dir.exists():
            print_warning(f"Directory does not exist: {source_dir}")
            continue

        print_info(f"Scanning directory: {source_dir}")

        for ext in extensions:
            for file_path in source_dir.rglob(f"*.{ext}"):
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


def add_extra_args_argument(parser, tool_name: str) -> None:
    """Add a repeatable --extra-arg option for passing raw arguments
    straight through to an underlying tool (clang-tidy, clang-format, ...).

    Usage: --extra-arg=-Wno-foo --extra-arg=-DSOME_DEFINE
    """

    parser.add_argument(
        "--extra-arg",
        dest="extra_args",
        action="append",
        default=[],
        metavar="ARG",
        help=(
            f"Additional argument to pass through to {tool_name}. "
            "May be given multiple times."
        ),
    )


def add_config_override_argument(parser, tool_name: str, config_filename: str) -> None:
    """Add a --config option for overriding the config file a tool
    would otherwise auto-discover (e.g. .clang-tidy, .clang-format)."""

    parser.add_argument(
        "--config",
        dest="config_file",
        type=Path,
        default=None,
        metavar="PATH",
        help=f"Path to a {config_filename} file to use instead of the auto-discovered one",
    )


def resolve_config_file(
    config_file: Optional[Path], project_root: Path
) -> Optional[Path]:
    """Resolve a user-supplied config path (relative to project_root if not
    absolute) and verify it exists. Returns None if config_file is None."""

    if config_file is None:
        return None

    path = config_file if config_file.is_absolute() else (project_root / config_file)
    path = path.resolve()

    if not path.exists():
        raise FileNotFoundError(f"Config file not found: {path}")

    return path
