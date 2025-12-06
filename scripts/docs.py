#!/usr/bin/env python3
"""
HeliosEngine Documentation Builder

Script for building Doxygen documentation.
"""

import argparse
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional, Tuple


class Colors:
    """ANSI color codes for terminal output"""

    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[1;33m"
    BLUE = "\033[0;34m"
    NC = "\033[0m"  # No Color

    @classmethod
    def disable(cls):
        """Disable colors for non-ANSI terminals"""

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


def print_header(message: str) -> None:
    """Print a colored header message"""
    print(f"\n{Colors.GREEN}{'=' * 60}{Colors.NC}")
    print(f"{Colors.GREEN}{message:^60}{Colors.NC}")
    print(f"{Colors.GREEN}{'=' * 60}{Colors.NC}\n")


def print_error(message: str) -> None:
    """Print a colored error message"""

    print(f"{Colors.RED}Error: {message}{Colors.NC}", file=sys.stderr)


def print_warning(message: str) -> None:
    """Print a colored warning message"""

    print(f"{Colors.YELLOW}Warning: {message}{Colors.NC}")


def print_success(message: str) -> None:
    """Print a colored success message"""

    print(f"{Colors.GREEN}{message}{Colors.NC}")


def print_info(message: str) -> None:
    """Print a colored info message"""

    print(f"{Colors.BLUE}{message}{Colors.NC}")


def find_executable(name: str) -> Optional[Path]:
    """Find an executable in PATH"""

    result = shutil.which(name)
    return Path(result) if result else None


def check_dependencies() -> Tuple[bool, list]:
    """Check for required dependencies"""

    missing = []
    warnings = []

    # Check for doxygen
    if not find_executable("doxygen"):
        missing.append("doxygen")

    # Check for dot (graphviz) - optional but recommended
    if not find_executable("dot"):
        warnings.append("graphviz (dot) - graphs will not be generated")

    return len(missing) == 0, missing, warnings


def get_doxygen_version() -> Optional[str]:
    """Get Doxygen version"""

    try:
        result = subprocess.run(
            ["doxygen", "--version"], capture_output=True, text=True, check=True
        )
        return result.stdout.strip()
    except Exception:
        return None


def count_warnings(output: str) -> int:
    """Count warnings in Doxygen output"""

    return output.lower().count("warning:")


def build_docs(docs_dir: Path, clean: bool = False, quiet: bool = False) -> bool:
    """Build documentation using Doxygen"""

    doxygen_dir = docs_dir / "doxygen"
    doxyfile = doxygen_dir / "Doxyfile"
    output_dir = doxygen_dir / "html"

    # Verify Doxyfile exists
    if not doxyfile.exists():
        print_error(f"Doxyfile not found: {doxyfile}")
        return False

    # Clean if requested
    if clean:
        if output_dir.exists():
            print_info(f"Cleaning output directory: {output_dir}")
            try:
                shutil.rmtree(output_dir)
                print_success("Documentation cleaned successfully!")
            except Exception as e:
                print_error(f"Failed to clean output directory: {e}")
                return False
        else:
            print_info("Output directory already clean")
        return True

    # Build documentation
    print_info(f"Building documentation from: {doxyfile}")
    print_info(f"Output directory: {output_dir}")
    print()

    try:
        # Run doxygen
        result = subprocess.run(
            ["doxygen", str(doxyfile)],
            cwd=str(doxygen_dir),
            capture_output=not quiet,
            text=True,
            check=False,
        )

        if result.returncode != 0:
            print_error("Doxygen build failed!")
            if result.stderr and not quiet:
                print(result.stderr, file=sys.stderr)
            return False

        # Count warnings
        output = (
            result.stdout + result.stderr if result.stdout and result.stderr else ""
        )
        warning_count = count_warnings(output)

        # Success!
        print()
        print_header("Documentation Built Successfully!")
        print()
        print_info(f"Output location: {output_dir / 'index.html'}")
        print()
        print("To view the documentation:")
        print(f"  1. Open in browser:")

        if sys.platform == "win32":
            print(f"     start {output_dir / 'index.html'}")
        elif sys.platform == "darwin":
            print(f"     open {output_dir / 'index.html'}")
        else:
            print(f"     xdg-open {output_dir / 'index.html'}")

        print()
        print(f"  2. Or run a local server:")
        print(f"     cd {output_dir}")
        print(f"     python3 -m http.server 8000")
        print(f"     Then open: http://localhost:8000")
        print()

        if warning_count > 0:
            print_warning(f"Build completed with {warning_count} warning(s)")
            if not quiet:
                print_info("Tip: Run without --quiet to see full build output")
        else:
            print_success("Build completed with no warnings!")

        return True

    except FileNotFoundError:
        print_error("Doxygen not found in PATH")
        return False
    except Exception as e:
        print_error(f"Unexpected error during build: {e}")
        return False


def main():
    """Main entry point"""

    parser = argparse.ArgumentParser(
        description="Build Doxygen documentation for Helios Engine",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Build documentation
  python docs.py

  # Build with verbose output
  python docs.py --verbose

  # Clean documentation
  python docs.py --clean

  # Check dependencies only
  python docs.py --check-only
        """,
    )

    parser.add_argument(
        "--clean", action="store_true", help="Clean generated documentation"
    )

    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Only check for required dependencies, don't build",
    )

    parser.add_argument(
        "--quiet",
        "-q",
        action="store_true",
        help="Suppress Doxygen output (only show summary)",
    )

    parser.add_argument(
        "--verbose",
        "-v",
        action="store_true",
        help="Show verbose output (opposite of --quiet)",
    )

    parser.add_argument(
        "--docs-dir", type=Path, help="Path to docs directory (default: auto-detect)"
    )

    parser.add_argument(
        "--no-color", action="store_true", help="Disable colored output"
    )

    args = parser.parse_args()

    # Disable colors if requested
    if args.no_color:
        Colors.disable()

    # Handle quiet/verbose
    quiet = args.quiet and not args.verbose

    # Print header
    print_header("Helios Engine Documentation Builder")

    # Determine docs directory
    if args.docs_dir:
        docs_dir = args.docs_dir.resolve()
    else:
        # Auto-detect: script is in scripts/, docs is in ../docs
        script_dir = Path(__file__).parent
        root_dir = script_dir.parent
        docs_dir = root_dir / "docs"

    if not docs_dir.exists():
        print_error(f"Documentation directory not found: {docs_dir}")
        return 1

    print_info(f"Documentation directory: {docs_dir}")

    # Check dependencies
    print_info("Checking dependencies...")
    deps_ok, missing, warnings = check_dependencies()

    if not deps_ok:
        print_error("Missing required dependencies:")
        for dep in missing:
            print(f"  - {dep}")
        print()
        print("Please install missing dependencies:")

        if sys.platform == "linux":
            print("  Ubuntu/Debian: sudo apt-get install doxygen graphviz")
            print("  Arch Linux:    sudo pacman -S doxygen graphviz")
        elif sys.platform == "darwin":
            print("  macOS:         brew install doxygen graphviz")
        elif sys.platform == "win32":
            print("  Windows:       choco install doxygen.install graphviz")
            print(
                "                 or download from https://www.doxygen.nl/download.html"
            )

        return 1

    # Show version
    version = get_doxygen_version()
    if version:
        print_success(f"Found Doxygen version {version}")
    else:
        print_warning("Could not determine Doxygen version")

    # Show warnings
    for warning in warnings:
        print_warning(f"Optional dependency missing: {warning}")

    # Exit if check-only
    if args.check_only:
        print()
        print_success("Dependency check complete!")
        return 0

    print()

    # Build or clean
    success = build_docs(docs_dir, clean=args.clean, quiet=quiet)

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
