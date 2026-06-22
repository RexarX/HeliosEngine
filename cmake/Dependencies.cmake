# Main dependency configuration for Helios Engine
#
# Dependencies are now decentralized - each plugin includes its own
# dependencies using helios_require_dependency() in its CMakeLists.txt.
#
# This file provides:
# - Initial setup and status messages
# - Test dependencies (loaded here since tests are built after modules)
# - Summary printing at the end of configuration
#
# Strategy: System packages -> CPM fallback

include_guard(GLOBAL)

# Load dependency management helpers
include(DependencyFinder)
include(DownloadUsingCPM)

# Print configuration header
message(STATUS "")
message(STATUS "========== Helios Dependency Configuration ==========")
message(STATUS "  → Dependencies loaded on-demand by modules")
message(STATUS "  → System packages checked FIRST")
message(STATUS "  → CPM downloads for missing dependencies")
message(STATUS "Allow CPM downloads: ${HELIOS_DOWNLOAD_PACKAGES}")
message(STATUS "Check package versions: ${HELIOS_CHECK_PACKAGE_VERSIONS}")
message(STATUS "=====================================================")
message(STATUS "")

# ============================================================================
# Test Dependencies (optional, only if building tests)
# ============================================================================

if(HELIOS_BUILD_TESTS)
  message(STATUS "Finding Test Dependencies...")
  message(STATUS "")

  helios_require_dependency(doctest)

  message(STATUS "")
endif()

# ============================================================================
# Summary Function (called after modules are built)
# ============================================================================

# This function should be called after all modules are configured to print
# a summary of all found dependencies.
function(helios_print_dependency_summary)
  helios_print_dependencies()
  if(CPM_PACKAGES)
    helios_print_cpm_packages()
  endif()
  message(STATUS "")
endfunction()
