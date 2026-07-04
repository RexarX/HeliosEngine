#!/usr/bin/env python3
"""
HeliosEngine Documentation Builder

Builds Doxygen HTML via the CMake target ``helios_docs`` when available,
or configures docs/doxygen/Doxyfile.in into the build tree and runs Doxygen
directly. Version comes from project(VERSION ...) in CMakeLists.txt.
"""

import argparse
import os
import re
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


def check_dependencies(root_dir: Path) -> Tuple[bool, list, list]:
    """Check for required dependencies"""

    missing = []
    warnings = []

    if not find_executable("doxygen"):
        missing.append("doxygen")

    theme_css = root_dir / "third-party" / "doxygen-awesome-css" / "doxygen-awesome.css"
    if not theme_css.exists():
        missing.append(
            "doxygen-awesome-css (run: git submodule update --init "
            "third-party/doxygen-awesome-css)"
        )

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


def parse_version(version: str) -> Tuple[int, int, int]:
    """Parse a Doxygen version string into (major, minor, patch)."""

    match = re.match(r"(\d+)\.(\d+)\.(\d+)", version.strip())
    if not match:
        return 0, 0, 0
    return int(match.group(1)), int(match.group(2)), int(match.group(3))


def is_doxygen_version_supported(version: str, minimum: str = "1.17.0") -> bool:
    """Return True when version meets the minimum required for the HTML header."""

    return parse_version(version) >= parse_version(minimum)


def count_warnings(output: str) -> int:
    """Count warnings in Doxygen output"""

    return output.lower().count("warning:")


def read_project_version(root_dir: Path) -> str:
    """Read project VERSION from the root CMakeLists.txt."""

    cmake_file = root_dir / "CMakeLists.txt"
    text = cmake_file.read_text(encoding="utf-8")
    match = re.search(
        r"project\s*\(\s*HeliosEngine\s+VERSION\s+([\d.]+)",
        text,
        re.IGNORECASE | re.DOTALL,
    )
    if not match:
        print_warning(f"Could not parse VERSION from {cmake_file}; using 0.0.0")
        return "0.0.0"
    return match.group(1)


def detect_docs_label() -> str:
    """Resolve documentation edition label from env or current git branch."""

    label = os.environ.get("DOCS_LABEL", "").strip()
    if label:
        return label

    try:
        result = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode == 0:
            branch = result.stdout.strip()
            if branch and branch != "HEAD":
                return branch
    except Exception:
        pass

    return "local"


def format_project_number(version: str) -> str:
    """Build Doxygen PROJECT_NUMBER from version and optional DOCS_LABEL env."""

    label = detect_docs_label()
    if not label or label in ("main", "local"):
        return version
    return f"{version} ({label})"


def resolve_project_number(root_dir: Path) -> str:
    """Return the Doxygen project number for the current build."""

    return os.environ.get(
        "HELIOS_DOXYGEN_PROJECT_NUMBER", ""
    ).strip() or format_project_number(read_project_version(root_dir))


def cmake_path(path: Path) -> str:
    """Normalize a path for Doxygen (forward slashes)."""

    return str(path.resolve()).replace("\\", "/")


def configure_doxyfile(root_dir: Path, build_doxygen_dir: Path) -> Path:
    """Materialize Doxyfile.in into the build tree."""

    template = root_dir / "docs/doxygen/Doxyfile.in"
    if not template.exists():
        raise FileNotFoundError(f"Doxyfile template not found: {template}")

    project_number = resolve_project_number(root_dir)
    text = template.read_text(encoding="utf-8")
    text = text.replace("@HELIOS_SOURCE_DIR@", cmake_path(root_dir))
    text = text.replace(
        "@HELIOS_DOXYGEN_OUTPUT_DIR@", cmake_path(root_dir / "docs/doxygen")
    )
    text = text.replace("@HELIOS_DOXYGEN_PROJECT_NUMBER@", project_number)

    build_doxygen_dir.mkdir(parents=True, exist_ok=True)
    doxyfile = build_doxygen_dir / "Doxyfile"
    doxyfile.write_text(text, encoding="utf-8")
    print_info(f"Project version: {project_number}")
    return doxyfile


def html_output_dir(root_dir: Path) -> Path:
    """Return the generated HTML output directory."""

    return root_dir / "docs/doxygen/html"


def write_local_version_selector(output_dir: Path, root_dir: Path) -> None:
    """Write a single-edition selector for local preview (http.server)."""

    project_number = resolve_project_number(root_dir)
    label = detect_docs_label()
    if label in ("main", "local"):
        display = f"{project_number} (local)"
    else:
        display = project_number

    html = (
        '<select id="versionSelector" title="Documentation edition" disabled>\n'
        f'  <option value="local">{display}</option>\n'
        "</select>\n"
    )
    (output_dir / "version_selector.html").write_text(html, encoding="utf-8")


def post_process_docs(root_dir: Path) -> None:
    """Copy assets and write the local edition selector."""

    docs_dir = root_dir / "docs"
    output_dir = html_output_dir(root_dir)

    img_src = docs_dir / "img"
    img_dst = output_dir / "docs" / "img"
    if img_src.is_dir():
        shutil.copytree(img_src, img_dst, dirs_exist_ok=True)

    write_local_version_selector(output_dir, root_dir)


def find_cmake_build_dir(root_dir: Path) -> Optional[Path]:
    """Find a configured build directory with HELIOS_BUILD_DOCS enabled."""

    build_root = root_dir / "build"
    if not build_root.is_dir():
        return None

    candidates = sorted(
        (path for path in build_root.iterdir() if path.is_dir()),
        key=lambda path: path.stat().st_mtime,
        reverse=True,
    )
    for candidate in candidates:
        cache = candidate / "CMakeCache.txt"
        if not cache.exists():
            continue
        cache_text = cache.read_text(encoding="utf-8", errors="ignore")
        if "HELIOS_BUILD_DOCS:BOOL=ON" in cache_text:
            return candidate
    return None


def build_docs_with_cmake(build_dir: Path, quiet: bool) -> bool:
    """Build documentation through the helios_docs CMake target."""

    print_info(f"Building documentation via CMake: {build_dir}")
    command = ["cmake", "--build", str(build_dir), "--target", "helios_docs"]
    if quiet:
        command.append("--quiet")

    result = subprocess.run(command, check=False)
    return result.returncode == 0


def build_docs_standalone(root_dir: Path, quiet: bool) -> bool:
    """Configure Doxyfile.in in the build tree and run Doxygen directly."""

    build_doxygen_dir = root_dir / "build/docs-doxygen"
    output_dir = html_output_dir(root_dir)

    try:
        configure_doxyfile(root_dir, build_doxygen_dir)
    except FileNotFoundError as error:
        print_error(str(error))
        return False

    print_info(f"Building documentation from: {build_doxygen_dir / 'Doxyfile'}")
    print_info(f"Output directory: {output_dir}")
    print()

    try:
        result = subprocess.run(
            ["doxygen", "Doxyfile"],
            cwd=str(build_doxygen_dir),
            capture_output=True,
            text=True,
            check=False,
        )

        if not quiet:
            if result.stdout:
                print(result.stdout)
            if result.stderr:
                print(result.stderr, file=sys.stderr)

        if result.returncode != 0:
            print_error("Doxygen build failed!")
            if result.stderr and quiet:
                print(result.stderr, file=sys.stderr)
            return False

        post_process_docs(root_dir)

        output = (result.stdout or "") + (result.stderr or "")
        warning_count = count_warnings(output)
        print_build_success(output_dir, warning_count, quiet)
        return True

    except FileNotFoundError:
        print_error("Doxygen not found in PATH")
        return False
    except Exception as error:
        print_error(f"Unexpected error during build: {error}")
        return False


def print_build_success(output_dir: Path, warning_count: int, quiet: bool) -> None:
    """Print the standard success summary."""

    print()
    print_header("Documentation Built Successfully!")
    print()
    print_info(f"Output location: {output_dir / 'index.html'}")
    print()
    print("To view the documentation:")
    print("  1. Open in browser:")

    if sys.platform == "win32":
        print(f"     start {output_dir / 'index.html'}")
    elif sys.platform == "darwin":
        print(f"     open {output_dir / 'index.html'}")
    else:
        print(f"     xdg-open {output_dir / 'index.html'}")

    print()
    print("  2. Or run a local server:")
    print(f"     cd {output_dir}")
    print("     python3 -m http.server 8000")
    print("     Then open: http://localhost:8000")
    print()
    print_info(
        "Use a local server (not file://) so the edition label and selector load correctly"
    )
    print()
    print_info(
        "Tip: cmake -B build-docs -DHELIOS_DOCS_ONLY=ON -DHELIOS_BUILD_DOCS=ON "
        "&& cmake --build build-docs --target helios_docs"
    )
    print()

    if warning_count > 0:
        print_warning(f"Build completed with {warning_count} warning(s)")
        if not quiet:
            print_info("Tip: Run without --quiet to see full build output")
    else:
        print_success("Build completed with no warnings!")


def build_docs(
    root_dir: Path,
    *,
    clean: bool = False,
    quiet: bool = False,
    use_cmake: bool = False,
    build_dir: Optional[Path] = None,
) -> bool:
    """Build documentation using CMake when configured, otherwise standalone."""

    output_dir = html_output_dir(root_dir)

    if clean:
        if output_dir.exists():
            print_info(f"Cleaning output directory: {output_dir}")
            try:
                shutil.rmtree(output_dir)
                print_success("Documentation cleaned successfully!")
            except Exception as error:
                print_error(f"Failed to clean output directory: {error}")
                return False
        else:
            print_info("Output directory already clean")
        return True

    resolved_build_dir = build_dir or find_cmake_build_dir(root_dir)
    if use_cmake or resolved_build_dir:
        if not resolved_build_dir:
            print_error(
                "CMake docs build requested but no configured build directory was found. "
                "Configure with -DHELIOS_BUILD_DOCS=ON first."
            )
            return False
        if not build_docs_with_cmake(resolved_build_dir, quiet):
            print_error("CMake documentation build failed!")
            return False
        print_build_success(output_dir, 0, quiet)
        return True

    return build_docs_standalone(root_dir, quiet)


def main() -> int:
    """Main entry point"""

    parser = argparse.ArgumentParser(
        description="Build Doxygen documentation for Helios Engine",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Build documentation (standalone, or via CMake when configured)
  python scripts/docs.py

  # Build through CMake
  cmake --preset linux-clang-debug -DHELIOS_BUILD_DOCS=ON
  cmake --build --preset linux-clang-debug --target helios_docs

  # Clean documentation
  python scripts/docs.py --clean
        """,
    )

    parser.add_argument(
        "--clean", action="store_true", help="Clean generated documentation"
    )
    parser.add_argument(
        "--post-process",
        action="store_true",
        help="Only run post-processing steps (used by the helios_docs CMake target)",
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
        "--cmake",
        action="store_true",
        help="Require the helios_docs CMake target (fail if not configured)",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        help="CMake build directory for the helios_docs target",
    )
    parser.add_argument(
        "--docs-dir", type=Path, help="Path to docs directory (default: auto-detect)"
    )
    parser.add_argument(
        "--no-color", action="store_true", help="Disable colored output"
    )

    args = parser.parse_args()

    if args.no_color:
        Colors.disable()

    quiet = args.quiet and not args.verbose

    script_dir = Path(__file__).parent
    root_dir = script_dir.parent
    if args.docs_dir:
        docs_dir = args.docs_dir.resolve()
        root_dir = docs_dir.parent
    else:
        docs_dir = root_dir / "docs"

    if args.post_process:
        post_process_docs(root_dir)
        return 0

    print_header("Helios Engine Documentation Builder")

    if not docs_dir.exists():
        print_error(f"Documentation directory not found: {docs_dir}")
        return 1

    print_info(f"Documentation directory: {docs_dir}")

    print_info("Checking dependencies...")
    deps_ok, missing, warnings = check_dependencies(root_dir)

    if not deps_ok:
        print_error("Missing required dependencies:")
        for dep in missing:
            print(f"  - {dep}")
        return 1

    version = get_doxygen_version()
    if version:
        print_success(f"Found Doxygen version {version}")
        if not is_doxygen_version_supported(version):
            print_error(
                "Doxygen 1.17.0 or newer is required for docs/doxygen/header.html"
            )
            return 1
    else:
        print_warning("Could not determine Doxygen version")

    for warning in warnings:
        print_warning(f"Optional dependency missing: {warning}")

    if args.check_only:
        print()
        print_success("Dependency check complete!")
        return 0

    print()

    success = build_docs(
        root_dir,
        clean=args.clean,
        quiet=quiet,
        use_cmake=args.cmake,
        build_dir=args.build_dir.resolve() if args.build_dir else None,
    )
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
