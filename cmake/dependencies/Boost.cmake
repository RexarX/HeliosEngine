# This module handles finding Boost from multiple sources:
# 1. Conan (if using Conan)
# 2. System packages (pacman, apt, etc.)
# 3. CPM download (fallback)
#
# Additionally, it checks for C++23 <stacktrace> header availability
# and uses STL stacktrace instead of Boost if available.

include_guard(GLOBAL)

message(STATUS "Configuring Boost dependency...")

# Check if C++23 <stacktrace> header is available
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_REQUIRED_LINK_OPTIONS "")

# Try to detect the compiler and set appropriate flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC requires libstdc++_libbacktrace for std::stacktrace
    list(APPEND CMAKE_REQUIRED_LINK_OPTIONS "-lstdc++_libbacktrace")
    set(_stl_stacktrace_link_libs "stdc++_libbacktrace")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang with libc++ may need different handling
    if(CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
        # libc++ doesn't have full stacktrace support yet in most versions
        set(_stl_stacktrace_link_libs "")
    else()
        # Clang with libstdc++
        list(APPEND CMAKE_REQUIRED_LINK_OPTIONS "-lstdc++_libbacktrace")
        set(_stl_stacktrace_link_libs "stdc++_libbacktrace")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC has built-in support, no extra libs needed
    set(_stl_stacktrace_link_libs "")
endif()

check_cxx_source_compiles("
#include <stacktrace>
#include <string>

int main() {
    auto st = std::stacktrace::current();
    if (st.size() > 0) {
        std::string desc = std::to_string(st[0]);
        (void)desc;
    }
    return 0;
}
" HELIOS_HAS_STL_STACKTRACE)

cmake_pop_check_state()

if(HELIOS_HAS_STL_STACKTRACE)
    message(STATUS "  ✓ C++23 <stacktrace> header is available, using STL stacktrace")
    set(HELIOS_USE_STL_STACKTRACE ON CACHE INTERNAL "Use C++23 STL stacktrace instead of Boost")

    # Create a target for STL stacktrace
    if(NOT TARGET helios::stl_stacktrace)
        add_library(helios::stl_stacktrace INTERFACE IMPORTED GLOBAL)
        target_compile_definitions(helios::stl_stacktrace INTERFACE HELIOS_USE_STL_STACKTRACE)

        # Link required libraries for STL stacktrace
        if(_stl_stacktrace_link_libs)
            target_link_libraries(helios::stl_stacktrace INTERFACE ${_stl_stacktrace_link_libs})
        endif()
    endif()

    # Create helios::boost_stacktrace as alias to STL stacktrace
    # This allows existing code to link against helios::boost_stacktrace without changes
    if(NOT TARGET helios::boost_stacktrace)
        add_library(helios::boost_stacktrace INTERFACE IMPORTED GLOBAL)
        target_link_libraries(helios::boost_stacktrace INTERFACE helios::stl_stacktrace)
    endif()
else()
    message(STATUS "  ✗ C++23 <stacktrace> header not available, using Boost stacktrace")
    set(HELIOS_USE_STL_STACKTRACE OFF CACHE INTERNAL "Use C++23 STL stacktrace instead of Boost")
endif()

# Boost components required by Helios Engine
# Note: unordered is header-only and doesn't need to be in this list
# stacktrace is only required if STL stacktrace is not available
if(NOT HELIOS_USE_STL_STACKTRACE)
    set(HELIOS_BOOST_REQUIRED_COMPONENTS
        stacktrace
    )
else()
    set(HELIOS_BOOST_REQUIRED_COMPONENTS "")
endif()

# Try to find Boost in order of preference
set(_boost_found_via "")

# 1. Try Conan first if using Conan and not preferring system packages
if(HELIOS_USE_CONAN AND NOT HELIOS_PREFER_SYSTEM_PACKAGES)
    find_package(Boost 1.82 QUIET CONFIG)
    if(Boost_FOUND)
        set(_boost_found_via "Conan")
    endif()
endif()

# 2. Try system package manager if not found via Conan
if(NOT Boost_FOUND)
    # Try CONFIG mode first (modern CMake packages like Arch Linux, Ubuntu 22.04+)
    find_package(Boost 1.82 QUIET CONFIG)
    if(Boost_FOUND)
        set(_boost_found_via "system (CONFIG)")
    else()
        # Fall back to MODULE mode with specific components
        if(HELIOS_BOOST_REQUIRED_COMPONENTS)
            find_package(Boost 1.82 QUIET COMPONENTS ${HELIOS_BOOST_REQUIRED_COMPONENTS})
        else()
            find_package(Boost 1.82 QUIET)
        endif()
        if(Boost_FOUND)
            set(_boost_found_via "system (MODULE)")
        endif()
    endif()
endif()

# 3. Create Boost targets if found
if(Boost_FOUND)
    if(_boost_found_via)
        message(STATUS "  ✓ Boost found via ${_boost_found_via}")
    else()
        message(STATUS "  ✓ Boost found at ${Boost_DIR}")
    endif()

    # Create convenience target: helios::boost
    if(NOT TARGET helios::boost)
        add_library(helios::boost INTERFACE IMPORTED)
        target_link_libraries(helios::boost INTERFACE Boost::boost)
    endif()

    # Create stacktrace-specific alias: helios::boost_stacktrace (only if not using STL stacktrace)
    if(NOT HELIOS_USE_STL_STACKTRACE)
        if(NOT TARGET helios::boost_stacktrace)
            add_library(helios::boost_stacktrace INTERFACE IMPORTED GLOBAL)
            if(TARGET Boost::stacktrace)
                target_link_libraries(helios::boost_stacktrace INTERFACE Boost::stacktrace)
            endif()
        endif()

        if(TARGET Boost::stacktrace_basic AND NOT TARGET helios::boost_stacktrace_basic)
            add_library(helios::boost_stacktrace_basic INTERFACE IMPORTED GLOBAL)
            target_link_libraries(helios::boost_stacktrace_basic INTERFACE Boost::stacktrace_basic)
        endif()

        if(TARGET Boost::stacktrace_backtrace AND NOT TARGET helios::boost_stacktrace_backtrace)
            add_library(helios::boost_stacktrace_backtrace INTERFACE IMPORTED GLOBAL)
            target_link_libraries(helios::boost_stacktrace_backtrace INTERFACE Boost::stacktrace_backtrace)
        endif()

        if(TARGET Boost::stacktrace_addr2line AND NOT TARGET helios::boost_stacktrace_addr2line)
            add_library(helios::boost_stacktrace_addr2line INTERFACE IMPORTED GLOBAL)
            target_link_libraries(helios::boost_stacktrace_addr2line INTERFACE Boost::stacktrace_addr2line)
        endif()

        if(TARGET Boost::stacktrace_windbg AND NOT TARGET helios::boost_stacktrace_windbg)
            add_library(helios::boost_stacktrace_windbg INTERFACE IMPORTED GLOBAL)
            target_link_libraries(helios::boost_stacktrace_windbg INTERFACE Boost::stacktrace_windbg)
        endif()
    endif()

    # Create alias for header-only unordered (not a separate component)
    if(NOT TARGET helios::boost_unordered)
        add_library(helios::boost_unordered INTERFACE IMPORTED GLOBAL)
        target_link_libraries(helios::boost_unordered INTERFACE Boost::boost)
    endif()

    # Mark as found for dependency tracking
    list(APPEND _HELIOS_DEPENDENCIES_FOUND "Boost")
else()
    # 4. Try CPM fallback if system packages not found
    if(HELIOS_DOWNLOAD_PACKAGES)
        message(STATUS "  ⬇ Boost not found in system, downloading via CPM...")

        # Only include stacktrace in download if STL stacktrace is not available
        if(HELIOS_USE_STL_STACKTRACE)
            set(_boost_include_libs "unordered")
        else()
            set(_boost_include_libs "stacktrace\\\\;unordered")
        endif()

        include(DownloadUsingCPM)
        helios_cpm_add_package(
            NAME Boost
            VERSION 1.90.0
            URL https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz
            OPTIONS
                "BOOST_ENABLE_CMAKE ON"
                "BOOST_INCLUDE_LIBRARIES ${_boost_include_libs}"
                "BUILD_SHARED_LIBS OFF"
            SYSTEM
        )

        # Create aliases if Boost was just added or already cached
        if(Boost_ADDED OR TARGET Boost::boost)
            if(NOT TARGET helios::boost)
                add_library(helios::boost INTERFACE IMPORTED GLOBAL)
                if(TARGET Boost::boost)
                    target_link_libraries(helios::boost INTERFACE Boost::boost)
                endif()
            endif()

            # Only create Boost stacktrace target if not using STL stacktrace
            if(NOT HELIOS_USE_STL_STACKTRACE)
                if(NOT TARGET helios::boost_stacktrace)
                    add_library(helios::boost_stacktrace INTERFACE IMPORTED GLOBAL)
                endif()

                if(TARGET Boost::stacktrace)
                    target_link_libraries(helios::boost_stacktrace INTERFACE Boost::stacktrace)
                else()
                    target_link_libraries(helios::boost_stacktrace INTERFACE helios::boost)
                endif()

                if(TARGET Boost::stacktrace_backtrace AND NOT TARGET helios::boost_stacktrace_backtrace)
                    add_library(helios::boost_stacktrace_backtrace INTERFACE IMPORTED GLOBAL)
                    target_link_libraries(helios::boost_stacktrace_backtrace INTERFACE Boost::stacktrace_backtrace)
                endif()

                if(TARGET Boost::stacktrace_addr2line AND NOT TARGET helios::boost_stacktrace_addr2line)
                    add_library(helios::boost_stacktrace_addr2line INTERFACE IMPORTED GLOBAL)
                    target_link_libraries(helios::boost_stacktrace_addr2line INTERFACE Boost::stacktrace_addr2line)
                endif()
            endif()

            # Create alias for header-only unordered (not a separate component)
            if(NOT TARGET helios::boost_unordered)
                add_library(helios::boost_unordered INTERFACE IMPORTED GLOBAL)
                target_link_libraries(helios::boost_unordered INTERFACE helios::boost)
            endif()
        endif()

        # Mark as found for dependency tracking
        list(APPEND _HELIOS_DEPENDENCIES_FOUND "Boost")
        list(APPEND CPM_PACKAGES "Boost 1.82.0")
    else()
        message(WARNING "  ✗ Boost not found and HELIOS_DOWNLOAD_PACKAGES is OFF. Consider enabling HELIOS_DOWNLOAD_PACKAGES or manually providing the required Boost components.")
    endif()
endif()

# Dynamically create aliases for each Boost component (only if not using STL stacktrace for stacktrace component)
foreach(component IN LISTS HELIOS_BOOST_REQUIRED_COMPONENTS)
    set(target_name "helios::boost_${component}") # Use underscores in aliases

    if(TARGET "Boost::${component}")
        if(NOT TARGET ${target_name})
            add_library(${target_name} INTERFACE IMPORTED GLOBAL)
            target_link_libraries(${target_name} INTERFACE "Boost::${component}")
            message(STATUS "  ✓ Created alias: ${target_name}")
        endif()
        # Link the component to the main helios::boost target (excluding pool and container)
        if(NOT component STREQUAL "pool" AND NOT component STREQUAL "container")
            target_link_libraries(helios::boost INTERFACE "Boost::${component}")
        endif()
    else()
        if(NOT HELIOS_USE_STL_STACKTRACE OR NOT component STREQUAL "stacktrace")
            message(WARNING "  ✗ Boost component '${component}' not found. Ensure the component is available in the Conan package or adjust the required components list.")
        endif()
    endif()
endforeach()
