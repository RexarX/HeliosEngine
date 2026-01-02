# This module handles finding spdlog from multiple sources:
# 1. Conan (if using Conan)
# 2. System packages (pacman, apt, etc.)
# 3. CPM download (fallback)

include_guard(GLOBAL)

message(STATUS "Configuring spdlog dependency...")

# Use helios_module system for standard package finding
helios_dep_begin(
    NAME spdlog
    VERSION 1.12
    DEBIAN_NAMES libspdlog-dev
    RPM_NAMES spdlog-devel
    PACMAN_NAMES spdlog
    BREW_NAMES spdlog
    PKG_CONFIG_NAMES spdlog
    CPM_NAME spdlog
    CPM_VERSION 1.16.0
    CPM_GITHUB_REPOSITORY gabime/spdlog
    CPM_OPTIONS
        "SPDLOG_BUILD_SHARED OFF"
        "SPDLOG_BUILD_EXAMPLE OFF"
        "SPDLOG_BUILD_TESTS OFF"
        "SPDLOG_USE_STD_FORMAT ON"
        "SPDLOG_FMT_EXTERNAL OFF"
)

helios_dep_end()

# Helper function to get the real (non-alias) target from a potentially aliased target
function(_helios_get_real_target target_name out_var)
    if(NOT TARGET ${target_name})
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(_current_target ${target_name})
    set(_max_iterations 10)
    set(_iteration 0)

    while(_iteration LESS _max_iterations)
        get_target_property(_aliased ${_current_target} ALIASED_TARGET)
        if(_aliased)
            set(_current_target ${_aliased})
        else()
            break()
        endif()
        math(EXPR _iteration "${_iteration} + 1")
    endwhile()

    set(${out_var} ${_current_target} PARENT_SCOPE)
endfunction()

# Create helios::spdlog::spdlog alias target for spdlog

if(TARGET spdlog::spdlog)
    if(NOT TARGET helios::spdlog::spdlog)
        _helios_get_real_target(spdlog::spdlog _spdlog_real)
        if(_spdlog_real)
            add_library(helios::spdlog::spdlog ALIAS ${_spdlog_real})
        else()
            add_library(helios::spdlog::spdlog ALIAS spdlog::spdlog)
        endif()
    endif()
    message(STATUS "  ✓ spdlog configured (compiled)")
endif()

if(TARGET spdlog::spdlog_header_only)
    if(NOT TARGET helios::spdlog::spdlog_header_only)
        _helios_get_real_target(spdlog::spdlog_header_only _spdlog_header_real)
        if(_spdlog_header_real)
            add_library(helios::spdlog::spdlog_header_only ALIAS ${_spdlog_header_real})
        else()
            add_library(helios::spdlog::spdlog_header_only ALIAS spdlog::spdlog_header_only)
        endif()
    endif()
    message(STATUS "  ✓ spdlog header-only configured")
endif()

# Fallbacks for older or non-namespaced targets
if(TARGET spdlog AND NOT TARGET helios::spdlog::spdlog)
    _helios_get_real_target(spdlog _spdlog_real)
    if(_spdlog_real)
        add_library(helios::spdlog::spdlog ALIAS ${_spdlog_real})
    else()
        add_library(helios::spdlog::spdlog ALIAS spdlog)
    endif()
    message(STATUS "  ✓ spdlog configured (fallback)")
endif()

if(TARGET spdlog_header_only AND NOT TARGET helios::spdlog::spdlog_header_only)
    _helios_get_real_target(spdlog_header_only _spdlog_header_real)
    if(_spdlog_header_real)
        add_library(helios::spdlog::spdlog_header_only ALIAS ${_spdlog_header_real})
    else()
        add_library(helios::spdlog::spdlog_header_only ALIAS spdlog_header_only)
    endif()
    message(STATUS "  ✓ spdlog header-only configured (fallback)")
endif()

# If we have helios::spdlog::spdlog but not helios::spdlog::spdlog_header_only, create the header-only alias
# This handles cases where pkg-config only provides the compiled library target
if(TARGET helios::spdlog::spdlog AND NOT TARGET helios::spdlog::spdlog_header_only)
    # Get the real target that helios::spdlog::spdlog points to
    _helios_get_real_target(helios::spdlog::spdlog _spdlog_real)
    if(_spdlog_real)
        add_library(helios::spdlog::spdlog_header_only ALIAS ${_spdlog_real})
    else()
        # If we can't get the real target, create an interface library instead
        add_library(_helios_spdlog_header_only_compat INTERFACE)
        target_link_libraries(_helios_spdlog_header_only_compat INTERFACE helios::spdlog::spdlog)
        add_library(helios::spdlog::spdlog_header_only ALIAS _helios_spdlog_header_only_compat)
    endif()
    message(STATUS "  ✓ spdlog header-only configured (using compiled library)")
endif()

# Create helios::spdlog convenience target that brings in all spdlog targets
if(NOT TARGET _helios_spdlog_all)
    add_library(_helios_spdlog_all INTERFACE)
    if(TARGET helios::spdlog::spdlog)
        target_link_libraries(_helios_spdlog_all INTERFACE helios::spdlog::spdlog)
    endif()
endif()

if(NOT TARGET helios::spdlog)
    add_library(helios::spdlog ALIAS _helios_spdlog_all)
endif()

# Warn if neither target is found
if(NOT TARGET helios::spdlog::spdlog AND NOT TARGET helios::spdlog::spdlog_header_only)
    message(WARNING "  ✗ spdlog not found")
endif()
