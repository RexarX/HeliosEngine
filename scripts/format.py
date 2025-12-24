#!/usr/bin/env python3
"""
HeliosEngine Code Formatting Tool

This script formats C++ source code using clang-format.
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


def check_clang_format() -> bool:
    """Check if clang-format is installed"""

    try:
        subprocess.run(
            ["clang-format", "--version"],
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


def format_file(file_path: Path, check_only: bool = False) -> bool:
    """
    Format a single file using clang-format

    Args:
        file_path: Path to the file to format
        check_only: If True, only check formatting without modifying

    Returns:
        True if file is correctly formatted (or was formatted successfully),
        False otherwise
    """

    try:
        if check_only:
            result = subprocess.run(
                [
                    "clang-format",
                    "-style=file",
                    "--dry-run",
                    "--Werror",
                    str(file_path),
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            return result.returncode == 0
        else:
            result = subprocess.run(
                ["clang-format", "-style=file", "-i", str(file_path)],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=True,
            )
            return True
    except subprocess.CalledProcessError:
        return False


def main() -> int:
    """Main entry point"""

    parser = argparse.ArgumentParser(
        description="Format C++ source code using clang-format"
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Check formatting only (don't modify files)",
    )
    parser.add_argument(
        "paths",
        nargs="*",
        help="Specific files or directories to format (default: all source files)",
    )

    args = parser.parse_args()

    # Get script and project directories
    script_dir = Path(__file__).parent.resolve()
    project_root = script_dir.parent

    print_info("HeliosEngine Code Formatting Tool")
    print_info("=" * 33)

    # Check if clang-format is installed
    if not check_clang_format():
        print_error("clang-format is not installed. Please install it first.")
        return 1

    # Define file extensions to process
    extensions = ["cpp", "h", "hpp", "inl"]

    # Define directories to exclude
    exclude_dirs = [project_root / "third-party"]

    # Find source files
    print_info("Finding source files...")

    if args.paths:
        # Process specific paths provided by user
        source_files = []
        for path_str in args.paths:
            path = Path(path_str)

            # Handle absolute and relative paths
            if not path.is_absolute():
                path = (project_root / path).resolve()

            if not path.exists():
                print_warning(f"Path does not exist: {path}")
                continue

            if path.is_file():
                # Check if it's a valid source file
                if path.suffix[1:] in extensions:
                    # Check if it's not in excluded directories
                    excluded = False
                    for exclude_dir in exclude_dirs:
                        try:
                            path.relative_to(exclude_dir)
                            excluded = True
                            break
                        except ValueError:
                            pass

                    if not excluded:
                        source_files.append(path)
                    else:
                        print_warning(f"Skipping excluded file: {path}")
                else:
                    print_warning(f"Skipping non-source file: {path}")
            elif path.is_dir():
                # Recursively find source files in directory
                source_files.extend(find_source_files([path], extensions, exclude_dirs))
            else:
                print_warning(f"Unknown path type: {path}")

        source_files = sorted(set(source_files))
    else:
        # Default: process all source directories
        source_dirs = [
            project_root / "src",
            project_root / "tests",
            project_root / "examples",
        ]
        source_files = find_source_files(source_dirs, extensions, exclude_dirs)

    if not source_files:
        print_warning(
            f"No source files found. Checked directories: {', '.join(str(d) for d in source_dirs)}"
        )
        return 0

    print_info(f"Found {len(source_files)} source files to process.")

    # Process files
    if args.check:
        print_info("Checking format only (not modifying files)...")
        needs_formatting = []

        for file_path in source_files:
            if not format_file(file_path, check_only=True):
                print_warning(f"File needs formatting: {file_path}")
                needs_formatting.append(file_path)

        if not needs_formatting:
            print_success("All files are correctly formatted.")
            return 0
        else:
            print_error(
                f"{len(needs_formatting)} file(s) need formatting. "
                f"Run 'python {script_dir / 'format.py'}' to format them."
            )
            return 1
    else:
        print_info("Formatting files...")
        failed_files = []

        for file_path in source_files:
            print_info(f"Formatting: {file_path}")
            if not format_file(file_path, check_only=False):
                print_error(f"Failed to format: {file_path}")
                failed_files.append(file_path)

        if not failed_files:
            print_success("All files formatted successfully.")
            return 0
        else:
            print_error(f"Failed to format {len(failed_files)} file(s).")
            return 1


if __name__ == "__main__":
    sys.exit(main())
