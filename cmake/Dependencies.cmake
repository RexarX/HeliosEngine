# Main dependency configuration for Helios Engine
#
# This file orchestrates all dependency finding and configuration.
# Each dependency is defined in its own file in cmake/dependencies/
#
# Strategy: Conan (if available) -> System packages -> CPM fallback
#
# The three-tier approach:
# 1. Conan packages (if using Conan toolchain)
# 2. System packages (find_package, pkg-config)
# 3. CPM automatic download (as last resort)

include_guard(GLOBAL)

# Load dependency management helpers
include(DependencyFinder)
include(DownloadUsingCPM)

# Print configuration header
message(STATUS "")
message(STATUS "========== Helios Dependency Configuration ==========")
message(STATUS "Using Conan: ${HELIOS_USE_CONAN}")
message(STATUS "Force Conan priority: ${HELIOS_FORCE_CONAN}")
if(HELIOS_FORCE_CONAN)
    message(STATUS "  → Conan packages will be checked FIRST")
    message(STATUS "  → System packages used as fallback")
    message(STATUS "  → CPM downloads for missing dependencies")
else()
    message(STATUS "  → System packages will be checked FIRST")
    message(STATUS "  → Conan used for missing dependencies (if available)")
    message(STATUS "  → CPM downloads as last resort")
endif()
message(STATUS "Allow CPM downloads: ${HELIOS_DOWNLOAD_PACKAGES}")
message(STATUS "Check package versions: ${HELIOS_CHECK_PACKAGE_VERSIONS}")
message(STATUS "=====================================================")
message(STATUS "")

# ============================================================================
# Core Dependencies (always required)
# ============================================================================

message(STATUS "Finding Core Dependencies...")
message(STATUS "")

# Include each core dependency
# Each file handles finding the dependency from Conan, system, or CPM
include(cmake/dependencies/Boost.cmake)
include(cmake/dependencies/spdlog.cmake)
include(cmake/dependencies/stduuid.cmake)
include(cmake/dependencies/TBB.cmake)
include(cmake/dependencies/Taskflow.cmake)

message(STATUS "")

# ============================================================================
# Module Dependencies (only if not building core-only)
# ============================================================================

if(NOT HELIOS_BUILD_CORE_ONLY)
    message(STATUS "Finding Module Dependencies...")
    message(STATUS "")

    # Add module-specific dependencies here as needed
    # Example:
    # include(cmake/dependencies/Vulkan.cmake)
    # include(cmake/dependencies/GLFW.cmake)

    message(STATUS "")
endif()

# ============================================================================
# Test Dependencies (optional, only if building tests)
# ============================================================================

if(HELIOS_BUILD_TESTS)
    message(STATUS "Finding Test Dependencies...")
    message(STATUS "")

    include(cmake/dependencies/doctest.cmake)

    message(STATUS "")
endif()

# ============================================================================
# Summary
# ============================================================================

helios_print_dependencies()
if(CPM_PACKAGES)
    helios_print_cpm_packages()
endif()
message(STATUS "")
