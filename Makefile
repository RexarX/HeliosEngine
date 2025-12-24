
# Simple wrapper around Python build scripts
# Use `make help` to see available commands

# ============================================================================
# Configuration
# ============================================================================

# Python interpreter
PYTHON := python3

# Script paths
SCRIPTS_DIR := scripts
BUILD_SCRIPT := $(SCRIPTS_DIR)/build.py
CONFIGURE_SCRIPT := $(SCRIPTS_DIR)/configure.py
INSTALL_DEPS_SCRIPT := $(SCRIPTS_DIR)/install_deps.py
LINT_SCRIPT := $(SCRIPTS_DIR)/lint.py
FORMAT_SCRIPT := $(SCRIPTS_DIR)/format.py
DOCS_SCRIPT := $(SCRIPTS_DIR)/docs.py

# Detect platform
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	DETECTED_PLATFORM := linux
endif
ifeq ($(UNAME_S),Darwin)
	DETECTED_PLATFORM := macos
endif
ifeq ($(OS),Windows_NT)
	DETECTED_PLATFORM := windows
endif

# Build configuration
PLATFORM ?= $(DETECTED_PLATFORM)


# Build type (must be set explicitly or inherited)
# BUILD_TYPE is not set by default

# Build directory (nested structure: build/<type>/<platform>/)
BUILD_DIR := build/$(BUILD_TYPE)/$(PLATFORM)

# User can provide variables via command line:
# make build BUILD_TYPE=Release COMPILER=clang CORE_ONLY=ON
BUILD_OPTS :=
ifdef BUILD_TYPE
	BUILD_OPTS += --type $(BUILD_TYPE)
endif
ifdef COMPILER
	BUILD_OPTS += --compiler $(COMPILER)
endif
ifdef BUILD_SYSTEM
	BUILD_OPTS += --build-system "$(BUILD_SYSTEM)"
endif
ifdef JOBS
	BUILD_OPTS += --jobs $(JOBS)
endif
ifdef CORE_ONLY
	BUILD_OPTS += --core-only
endif
ifdef NO_TESTS
	BUILD_OPTS += --no-tests
endif
ifdef TESTS
	BUILD_OPTS += --tests
endif
ifdef EXAMPLES
	BUILD_OPTS += --examples
endif
ifdef NO_EXAMPLES
	BUILD_OPTS += --no-examples
endif

# ============================================================================
# Phony Targets
# ============================================================================

.PHONY: all build configure clean rebuild test help
.PHONY: format format-check lint docs docs-clean
.PHONY: install-deps check

# ============================================================================
# Main Targets
# ============================================================================

# Default target
all: build

# Configure the project
configure:
	@$(PYTHON) $(CONFIGURE_SCRIPT) $(BUILD_OPTS)

# Build the project
build:
	@$(PYTHON) $(BUILD_SCRIPT) $(BUILD_OPTS)

# Clean build directory
clean:
	@echo "Cleaning build directory..."
	@rm -rf build/
	@echo "✓ Build directory cleaned"

# Full rebuild
rebuild: clean build

# Run tests
test:
	@if [ -z "$(BUILD_TYPE)" ]; then \
		echo "Error: BUILD_TYPE is not set."; \
		echo "Usage: make test BUILD_TYPE=release PLATFORM=linux"; \
		exit 1; \
	fi; \
	echo "Running tests in $(BUILD_DIR)..."; \
	if [ -d "$(BUILD_DIR)" ]; then \
		cd $(BUILD_DIR) && ctest --output-on-failure; \
	else \
		echo "Error: Build directory not found at $(BUILD_DIR)"; \
		echo "Run 'make build BUILD_TYPE=$(BUILD_TYPE) PLATFORM=$(PLATFORM)' first."; \
		exit 1; \
	fi

# ============================================================================
# Dependency Management
# ============================================================================

# Install dependencies
install-deps:
	@$(PYTHON) $(INSTALL_DEPS_SCRIPT)

# ============================================================================
# Code Quality
# ============================================================================

# Format code
format:
	@if [ -f "$(FORMAT_SCRIPT)" ]; then \
		$(PYTHON) $(FORMAT_SCRIPT); \
	else \
		echo "Warning: $(FORMAT_SCRIPT) not found"; \
	fi

# Check code formatting
format-check:
	@if [ -f "$(FORMAT_SCRIPT)" ]; then \
		$(PYTHON) $(FORMAT_SCRIPT) --check; \
	else \
		echo "Warning: $(FORMAT_SCRIPT) not found"; \
	fi

# Run linter
lint:
	@if [ -z "$(BUILD_TYPE)" ]; then \
		echo "Error: BUILD_TYPE is not set."; \
		echo "Usage: make lint BUILD_TYPE=release PLATFORM=linux"; \
		exit 1; \
	fi; \
	if [ -f "$(LINT_SCRIPT)" ]; then \
		$(PYTHON) $(LINT_SCRIPT) --platform $(PLATFORM) --type $(BUILD_TYPE); \
	else \
		echo "Warning: $(LINT_SCRIPT) not found"; \
	fi

# ============================================================================
# Documentation
# ============================================================================

# Generate documentation
docs:
	@if [ -f "$(DOCS_SCRIPT)" ]; then \
		$(PYTHON) $(DOCS_SCRIPT); \
	else \
		echo "Warning: $(DOCS_SCRIPT) not found"; \
	fi

# Clean documentation
docs-clean:
	@if [ -f "$(DOCS_SCRIPT)" ]; then \
		$(PYTHON) $(DOCS_SCRIPT) --clean; \
	else \
		echo "✓ Documentation cleaned"; \
	fi

# ============================================================================
# Utilities
# ============================================================================

# Check prerequisites
check:
	@echo "Checking prerequisites..."
	@echo ""
	@command -v cmake >/dev/null 2>&1 && echo "✓ CMake found" || echo "✗ CMake not found"
	@command -v $(PYTHON) >/dev/null 2>&1 && echo "✓ Python found" || echo "✗ Python not found"
	@command -v ninja >/dev/null 2>&1 && echo "✓ Ninja found" || echo "  Ninja not found (optional)"
	@command -v make >/dev/null 2>&1 && echo "✓ Make found" || echo "  Make not found (optional)"
	@command -v gcc >/dev/null 2>&1 && echo "✓ GCC found" || echo "  GCC not found (optional)"
	@command -v clang >/dev/null 2>&1 && echo "✓ Clang found" || echo "  Clang not found (optional)"
	@echo ""

# Help message
help:
	@echo "Helios Engine Makefile"
	@echo "======================"
	@echo ""
	@echo "Usage:"
	@echo "  make <target> [VARIABLE=value ...]"
	@echo ""
	@echo "Main Targets:"
	@echo "  configure        Configure CMake (calls configure.py)"
	@echo "  build            Build the project (calls build.py, which configures if needed)"
	@echo "  clean            Clean build directory"
	@echo "  rebuild          Clean and rebuild"
	@echo "  test             Run tests"
	@echo "  install-deps     Install dependencies"
	@echo ""
	@echo "Code Quality:"
	@echo "  format           Format code"
	@echo "  format-check     Check code formatting"
	@echo "  lint             Run linter (respects PLATFORM and BUILD_TYPE)"
	@echo ""
	@echo "Documentation:"
	@echo "  docs             Generate documentation"
	@echo "  docs-clean       Clean documentation"
	@echo ""
	@echo "Utilities:"
	@echo "  check            Check build prerequisites"
	@echo "  help             Show this help"
	@echo ""
	@echo "Optional Variables:"
	@echo "  BUILD_TYPE       Build type (debug, release, relwithdebinfo) - default: release"
	@echo "  PLATFORM         Target platform (linux, macos, windows) - default: auto-detected"
	@echo "  COMPILER         Compiler (gcc, clang, cl)"
	@echo "  BUILD_SYSTEM     Build system (Ninja, Unix Makefiles, Visual Studio 17 2022)"
	@echo "  JOBS             Number of parallel jobs"
	@echo "  CORE_ONLY        Build core only (any value enables)"
	@echo "  TESTS            Enable tests (any value enables)"
	@echo "  NO_TESTS         Disable tests (any value enables)"
	@echo "  EXAMPLES         Enable examples (any value enables)"
	@echo "  NO_EXAMPLES      Disable examples (any value enables)"
	@echo ""
	@echo "Examples:"
	@echo "  make install-deps                           # Install dependencies"
	@echo "  make configure                              # Configure CMake"
	@echo "  make build                                  # Build (configures first if needed)"
	@echo "  make build BUILD_TYPE=debug                 # Debug build"
	@echo "  make build BUILD_TYPE=release COMPILER=clang # Release with Clang"
	@echo "  make build CORE_ONLY=1 NO_TESTS=1          # Core only, no tests"
	@echo "  make test BUILD_TYPE=debug                  # Run tests"
	@echo "  make lint PLATFORM=linux BUILD_TYPE=debug   # Lint for Linux debug build"
	@echo "  make clean build                            # Clean and rebuild"
	@echo ""
	@echo "For more options, use the scripts directly:"
	@echo "  python $(INSTALL_DEPS_SCRIPT) --help"
	@echo "  python $(CONFIGURE_SCRIPT) --help"
	@echo "  python $(BUILD_SCRIPT) --help"
	@echo ""
