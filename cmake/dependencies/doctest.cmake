# This module handles finding doctest from multiple sources:
# 1. Conan (if using Conan)
# 2. System packages (pacman, apt, etc.)
# 3. CPM download (fallback)

include_guard(GLOBAL)

message(STATUS "Configuring doctest dependency...")

# Use helios_module system for standard package finding
helios_dep_begin(
    NAME doctest
    VERSION 2.4.11
    DEBIAN_NAMES doctest-dev
    RPM_NAMES doctest-devel
    PACMAN_NAMES doctest
    BREW_NAMES doctest
    PKG_CONFIG_NAMES doctest
    CPM_NAME doctest
    CPM_VERSION 2.4.11
    CPM_GITHUB_REPOSITORY doctest/doctest
    CPM_GIT_TAG v2.4.11
    CPM_OPTIONS
        "DOCTEST_WITH_TESTS OFF"
        "DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF"
)

helios_dep_end()

# Create helios::doctest alias if doctest was found
if(NOT TARGET helios::doctest)
    if(TARGET doctest::doctest)
        add_library(helios::doctest ALIAS doctest::doctest)
        message(STATUS "  ✓ doctest configured (doctest::doctest)")
    elseif(TARGET doctest)
        add_library(helios::doctest ALIAS doctest)
        message(STATUS "  ✓ doctest configured (doctest)")
    else()
        message(WARNING "  ✗ doctest not found")
    endif()
else()
    message(STATUS "  ✓ doctest configured (helios::doctest)")
endif()
