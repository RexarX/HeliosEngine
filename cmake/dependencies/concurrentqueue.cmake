# This module handles finding concurrentqueue from multiple sources:
# 1. Conan (if using Conan)
# 2. System packages (pacman, apt, etc.)
# 3. CPM download (fallback)

include_guard(GLOBAL)

message(STATUS "Configuring concurrentqueue dependency...")

# Use helios_module system for standard package finding
helios_dep_begin(
    NAME concurrentqueue
    VERSION 1.0
    PKG_CONFIG_NAMES concurrentqueue
    CPM_NAME concurrentqueue
    CPM_VERSION 1.0
    CPM_GITHUB_REPOSITORY cameron314/concurrentqueue
)

helios_dep_end()

# Create helios::concurrentqueue alias if concurrentqueue was found
if(NOT TARGET helios::concurrentqueue)
    if(TARGET concurrentqueue::concurrentqueue)
        add_library(helios::concurrentqueue ALIAS concurrentqueue::concurrentqueue)
        message(STATUS "  ✓ concurrentqueue configured (concurrentqueue::concurrentqueue)")
    elseif(TARGET concurrentqueue)
        add_library(helios::concurrentqueue ALIAS concurrentqueue)
        message(STATUS "  ✓ concurrentqueue configured (concurrentqueue)")
    else()
        message(WARNING "  ✗ concurrentqueue not found")
    endif()
else()
    message(STATUS "  ✓ concurrentqueue configured (helios::concurrentqueue)")
endif()
