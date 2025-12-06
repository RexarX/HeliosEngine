# This module handles finding stduuid from multiple sources:
# 1. Conan (if using Conan)
# 2. System packages (pacman, apt, etc.)
# 3. CPM download (fallback)

include_guard(GLOBAL)

message(STATUS "Configuring stduuid dependency...")

# Use helios_module system for standard package finding
helios_dep_begin(
    NAME stduuid
    VERSION 1.2
    RPM_NAMES stduuid-devel
    PKG_CONFIG_NAMES stduuid
    CPM_NAME stduuid
    CPM_VERSION 1.2.3
    CPM_GITHUB_REPOSITORY mariusbancila/stduuid
    CPM_OPTIONS
        "STDUUID_BUILD_TESTS OFF"
)

helios_dep_end()

# Create helios::stduuid alias if stduuid was found
if(NOT TARGET helios::stduuid)
    if(TARGET stduuid::stduuid)
        add_library(helios::stduuid ALIAS stduuid::stduuid)
        message(STATUS "  ✓ stduuid configured (stduuid::stduuid)")
    elseif(TARGET stduuid)
        add_library(helios::stduuid ALIAS stduuid)
        message(STATUS "  ✓ stduuid configured (stduuid)")
    else()
        message(WARNING "  ✗ stduuid not found")
    endif()
else()
    message(STATUS "  ✓ stduuid configured (helios::stduuid)")
endif()
