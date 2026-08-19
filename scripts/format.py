#!/usr/bin/env python3
"""
HeliosEngine Code Formatting Tool

This script formats C++ source code using clang-format.
"""

import argparse
import subprocess
import sys
from pathlib import Path
from typing import List, Optional

from common import (
    Colors,
    add_config_override_argument,
    add_extra_args_argument,
    check_tool_installed,
    enable_windows_colors,
    find_source_files,
    print_error,
    print_info,
    print_success,
    print_warning,
    resolve_config_file,
)

enable_windows_colors()


def format_file(
    file_path: Path,
    check_only: bool = False,
    config_file: Optional[Path] = None,
    extra_args: Optional[List[str]] = None,
) -> bool:
    """
    Format a single file using clang-format

    Args:
        file_path: Path to the file to format
        check_only: If True, only check formatting without modifying
        config_file: Optional explicit .clang-format config to use instead
            of the one clang-format would auto-discover via -style=file
        extra_args: Additional raw arguments to pass through to clang-format

    Returns:
        True if file is correctly formatted (or was formatted successfully),
        False otherwise
    """

    style_arg = f"-style=file:{config_file}" if config_file else "-style=file"

    try:
        if check_only:
            cmd = ["clang-format", style_arg, "--dry-run", "--Werror"]
            if extra_args:
                cmd.extend(extra_args)
            cmd.append(str(file_path))

            result = subprocess.run(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            return result.returncode == 0
        else:
            cmd = ["clang-format", style_arg, "-i"]
            if extra_args:
                cmd.extend(extra_args)
            cmd.append(str(file_path))

            subprocess.run(
                cmd,
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
    add_config_override_argument(parser, "clang-format", ".clang-format")
    add_extra_args_argument(parser, "clang-format")
    parser.add_argument(
        "--no-color", action="store_true", help="Disable colored output"
    )

    args = parser.parse_args()

    if args.no_color:
        Colors.disable()

    # Get script and project directories
    script_dir = Path(__file__).parent.resolve()
    project_root = script_dir.parent

    print_info("HeliosEngine Code Formatting Tool")
    print_info("=" * 33)

    # Check if clang-format is installed
    if not check_tool_installed("clang-format"):
        print_error("clang-format is not installed. Please install it first.")
        return 1

    # Resolve an explicit config file override, if given
    try:
        config_file = resolve_config_file(args.config_file, project_root)
    except FileNotFoundError as error:
        print_error(str(error))
        return 1

    # Define file extensions to process
    extensions = ["cpp", "h", "hpp", "inl"]

    # Define directories to exclude
    exclude_dirs = [project_root / "third-party"]

    # Find source files
    print_info("Finding source files...")

    source_dirs = [
        project_root / "src",
        project_root / "tests",
        project_root / "examples",
    ]

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
        source_files = find_source_files(source_dirs, extensions, exclude_dirs)

    if not source_files:
        print_warning(
            f"No source files found. Checked directories: {', '.join(str(d) for d in source_dirs)}"
        )
        return 0

    print_info(f"Found {len(source_files)} source files to process.")

    if config_file:
        print_info(f"Using config file override: {config_file}")
    if args.extra_args:
        print_info(f"Extra clang-format arguments: {' '.join(args.extra_args)}")

    # Process files
    if args.check:
        print_info("Checking format only (not modifying files)...")
        needs_formatting = []

        for file_path in source_files:
            if not format_file(
                file_path,
                check_only=True,
                config_file=config_file,
                extra_args=args.extra_args,
            ):
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
            if not format_file(
                file_path,
                check_only=False,
                config_file=config_file,
                extra_args=args.extra_args,
            ):
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
