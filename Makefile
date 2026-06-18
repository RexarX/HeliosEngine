# Wrapper for formatting, linting, and documentation scripts.
# Build and test via CMake presets — see README.md.
# Use `make help` to see available commands.

# ============================================================================
# Configuration
# ============================================================================

# Python interpreter (python on Windows, python3 elsewhere)
ifeq ($(OS),Windows_NT)
	PYTHON ?= python
else
	PYTHON ?= python3
endif

SCRIPTS_DIR := scripts
FORMAT_SCRIPT := $(SCRIPTS_DIR)/format.py
LINT_SCRIPT := $(SCRIPTS_DIR)/lint.py
DOCS_SCRIPT := $(SCRIPTS_DIR)/docs.py

# Lint: build directory containing compile_commands.json (CMake preset layout)
CMAKE_PRESET ?= linux-gcc-release
BUILD_DIR ?= build/$(CMAKE_PRESET)

# Optional extra arguments (e.g. FORMAT_OPTS="src/core", LINT_OPTS=--fix)
FORMAT_OPTS ?=
LINT_OPTS ?=
DOCS_OPTS ?=

# ============================================================================
# Phony Targets
# ============================================================================

.PHONY: help format format-check lint docs docs-clean docs-check

.DEFAULT_GOAL := help

# ============================================================================
# Code Quality
# ============================================================================

format:
	@$(PYTHON) $(FORMAT_SCRIPT) $(FORMAT_OPTS)

format-check:
	@$(PYTHON) $(FORMAT_SCRIPT) --check $(FORMAT_OPTS)

lint:
	@$(PYTHON) $(LINT_SCRIPT) --build-dir $(BUILD_DIR) $(LINT_OPTS)

# ============================================================================
# Documentation
# ============================================================================

docs:
	@$(PYTHON) $(DOCS_SCRIPT) $(DOCS_OPTS)

docs-clean:
	@$(PYTHON) $(DOCS_SCRIPT) --clean

docs-check:
	@$(PYTHON) $(DOCS_SCRIPT) --check-only

# ============================================================================
# Help
# ============================================================================

help:
	@echo "Helios Engine Makefile"
	@echo "======================"
	@echo ""
	@echo "Usage:"
	@echo "  make <target> [VARIABLE=value ...]"
	@echo ""
	@echo "Code Quality:"
	@echo "  format           Format C++ sources (clang-format)"
	@echo "  format-check     Check formatting without modifying files"
	@echo "  lint             Run clang-tidy (requires a configured build)"
	@echo ""
	@echo "Documentation:"
	@echo "  docs             Build Doxygen documentation"
	@echo "  docs-clean       Remove generated documentation"
	@echo "  docs-check       Verify documentation dependencies"
	@echo ""
	@echo "Utilities:"
	@echo "  help             Show this help"
	@echo ""
	@echo "Optional Variables:"
	@echo "  CMAKE_PRESET     CMake preset name for lint (default: linux-gcc-release)"
	@echo "  BUILD_DIR        Build dir with compile_commands.json (default: build/\$$CMAKE_PRESET)"
	@echo "  FORMAT_OPTS      Extra args passed to format.py (e.g. paths)"
	@echo "  LINT_OPTS        Extra args passed to lint.py (e.g. --fix)"
	@echo "  DOCS_OPTS        Extra args passed to docs.py (e.g. --verbose)"
	@echo "  PYTHON           Python interpreter (default: auto-detected)"
	@echo ""
	@echo "Examples:"
	@echo "  make format"
	@echo "  make format-check"
	@echo "  make lint CMAKE_PRESET=linux-clang-debug"
	@echo "  make lint BUILD_DIR=build/linux-gcc-release LINT_OPTS=--fix"
	@echo "  make docs"
	@echo "  make docs-clean"
	@echo ""
	@echo "Build and test (CMake presets):"
	@echo "  cmake --preset linux-gcc-release"
	@echo "  cmake --build --preset linux-gcc-release"
	@echo "  ctest --preset linux-gcc-release"
