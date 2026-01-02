# This module handles finding TBB from system packages only.
# TBB is required on Linux when using GCC's libstdc++ because the parallel STL implementation uses TBB as its backend.
#
# This is NOT managed via Conan - TBB should be installed as a system package.

include_guard(GLOBAL)
include(CheckCXXSymbolExists)

# Determine the standard library in use
if (cxx_std_20 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(header version)  # Use <version> for C++20 and later
else()
    set(header ciso646)  # Use <ciso646> for older standards
endif()

check_cxx_symbol_exists(__GLIBCXX__ ${header} GLIBCXX)
check_cxx_symbol_exists(_LIBCPP_VERSION ${header} LIBCPP)

# Only proceed if using libstdc++ on Linux
if (NOT (UNIX AND NOT APPLE AND GLIBCXX))
    message(STATUS "TBB: Not required (not using libstdc++)")
    return()
endif()

message(STATUS "Configuring TBB dependency (system package for libstdc++ parallel STL)...")

# Try to find TBB using CMake's find_package
find_package(TBB QUIET)

if (TBB_FOUND OR TARGET TBB::tbb)
    message(STATUS "  ✓ TBB found via find_package")
else()
    # Try pkg-config as fallback
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
        pkg_check_modules(TBB_PKG QUIET tbb)
        if (TBB_PKG_FOUND)
            message(STATUS "  ✓ TBB found via pkg-config")

            # Create imported target
            add_library(TBB::tbb INTERFACE IMPORTED)
            target_include_directories(TBB::tbb INTERFACE ${TBB_PKG_INCLUDE_DIRS})
            target_link_libraries(TBB::tbb INTERFACE ${TBB_PKG_LIBRARIES})
            target_link_directories(TBB::tbb INTERFACE ${TBB_PKG_LIBRARY_DIRS})
        endif()
    endif()
endif()

# Create helios::tbb::tbb alias if TBB was found
if (NOT TARGET helios::tbb::tbb)
    if (TARGET TBB::tbb)
        # Check if TBB::tbb is an alias
        get_target_property(_tbb_aliased TBB::tbb ALIASED_TARGET)
        if (_tbb_aliased)
            add_library(helios::tbb::tbb ALIAS ${_tbb_aliased})
        else()
            add_library(helios::tbb::tbb ALIAS TBB::tbb)
        endif()
        set(HELIOS_HAS_SYSTEM_TBB TRUE CACHE INTERNAL "System TBB is available")
        message(STATUS "  ✓ TBB configured (helios::tbb::tbb)")
    elseif (TARGET TBB::TBB)
        add_library(helios::tbb::tbb ALIAS TBB::TBB)
        set(HELIOS_HAS_SYSTEM_TBB TRUE CACHE INTERNAL "System TBB is available")
        message(STATUS "  ✓ TBB configured (helios::tbb::tbb)")
    else()
        set(HELIOS_HAS_SYSTEM_TBB FALSE CACHE INTERNAL "System TBB is not available")
        message(WARNING "  ✗ TBB not found - parallel STL algorithms will use serial backend")
        message(STATUS "    Install TBB using your package manager:")
        message(STATUS "      Debian/Ubuntu: sudo apt install libtbb-dev")
        message(STATUS "      Fedora/RHEL:   sudo dnf install tbb-devel")
        message(STATUS "      Arch Linux:    sudo pacman -S onetbb")
    endif()
else()
    set(HELIOS_HAS_SYSTEM_TBB TRUE CACHE INTERNAL "System TBB is available")
    message(STATUS "  ✓ TBB already configured (helios::tbb::tbb)")
endif()

# Create helios::tbb convenience target that brings in all TBB targets
if(NOT TARGET _helios_tbb_all)
    add_library(_helios_tbb_all INTERFACE)
    if(TARGET helios::tbb::tbb)
        target_link_libraries(_helios_tbb_all INTERFACE helios::tbb::tbb)
    endif()
endif()

if(NOT TARGET helios::tbb)
    add_library(helios::tbb ALIAS _helios_tbb_all)
endif()
