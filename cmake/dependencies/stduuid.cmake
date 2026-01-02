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

# Create helios::stduuid::stduuid alias if stduuid was found
if(NOT TARGET helios::stduuid::stduuid)
    if(TARGET stduuid::stduuid)
        add_library(helios::stduuid::stduuid ALIAS stduuid::stduuid)
        message(STATUS "  ✓ stduuid configured (stduuid::stduuid)")
    elseif(TARGET stduuid)
        add_library(helios::stduuid::stduuid ALIAS stduuid)
        message(STATUS "  ✓ stduuid configured (stduuid)")
    else()
        message(WARNING "  ✗ stduuid not found")
    endif()
else()
    message(STATUS "  ✓ stduuid configured (helios::stduuid::stduuid)")
endif()

# Create helios::stduuid convenience target that brings in all stduuid targets
if(NOT TARGET _helios_stduuid_all)
    add_library(_helios_stduuid_all INTERFACE)
    if(TARGET helios::stduuid::stduuid)
        target_link_libraries(_helios_stduuid_all INTERFACE helios::stduuid::stduuid)
    endif()
endif()

if(NOT TARGET helios::stduuid)
    add_library(helios::stduuid ALIAS _helios_stduuid_all)
endif()
