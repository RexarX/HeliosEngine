# This module handles finding spdlog from multiple sources:
# 1. Conan (if using Conan)
# 2. System packages (pacman, apt, etc.)
# 3. CPM download (fallback)

include_guard(GLOBAL)

message(STATUS "Configuring spdlog dependency...")

# Use helios_module system for standard package finding
helios_dep_begin(
    NAME spdlog
    VERSION 1.14
    DEBIAN_NAMES libspdlog-dev
    RPM_NAMES spdlog-devel
    PACMAN_NAMES spdlog
    BREW_NAMES spdlog
    PKG_CONFIG_NAMES spdlog
    CPM_NAME spdlog
    CPM_VERSION 1.14.1
    CPM_GITHUB_REPOSITORY gabime/spdlog
    CPM_OPTIONS
        "SPDLOG_BUILD_SHARED OFF"
        "SPDLOG_BUILD_EXAMPLE OFF"
        "SPDLOG_BUILD_TESTS OFF"
        "SPDLOG_USE_STD_FORMAT ON"
        "SPDLOG_FMT_EXTERNAL OFF"
)

helios_dep_end()

# Create helios alias targets for spdlog

if(TARGET spdlog::spdlog)
    if(NOT TARGET helios::spdlog)
        get_target_property(_spdlog_aliased spdlog::spdlog ALIASED_TARGET)
        if(_spdlog_aliased)
            add_library(helios::spdlog ALIAS ${_spdlog_aliased})
        else()
            add_library(helios::spdlog ALIAS spdlog::spdlog)
        endif()
    endif()
    message(STATUS "  ✓ spdlog configured (compiled)")
endif()

if(TARGET spdlog::spdlog_header_only)
    if(NOT TARGET helios::spdlog_header_only)
        get_target_property(_spdlog_header_aliased spdlog::spdlog_header_only ALIASED_TARGET)
        if(_spdlog_header_aliased)
            add_library(helios::spdlog_header_only ALIAS ${_spdlog_header_aliased})
        else()
            add_library(helios::spdlog_header_only ALIAS spdlog::spdlog_header_only)
        endif()
    endif()
    message(STATUS "  ✓ spdlog header-only configured")
endif()

# Fallbacks for older or non-namespaced targets
if(TARGET spdlog AND NOT TARGET helios::spdlog)
    add_library(helios::spdlog ALIAS spdlog)
    message(STATUS "  ✓ spdlog configured (fallback)")
endif()

if(TARGET spdlog_header_only AND NOT TARGET helios::spdlog_header_only)
    add_library(helios::spdlog_header_only ALIAS spdlog_header_only)
    message(STATUS "  ✓ spdlog header-only configured (fallback)")
endif()

# If we have helios::spdlog but not helios::spdlog_header_only, create the header-only alias
# This handles cases where pkg-config only provides the compiled library target
if(TARGET helios::spdlog AND NOT TARGET helios::spdlog_header_only)
    add_library(helios::spdlog_header_only ALIAS helios::spdlog)
    message(STATUS "  ✓ spdlog header-only configured (using compiled library)")
endif()

# Warn if neither target is found
if(NOT TARGET helios::spdlog AND NOT TARGET helios::spdlog_header_only)
    message(WARNING "  ✗ spdlog not found")
endif()
