# This module handles finding Boost from multiple sources:
# 1. Conan (if using Conan)
# 2. System packages (pacman, apt, etc.)
# 3. CPM download (fallback)

include_guard(GLOBAL)

message(STATUS "Configuring Boost dependency...")

# Boost components required by Helios Engine
# Note: unordered is header-only and doesn't need to be in this list
set(HELIOS_BOOST_REQUIRED_COMPONENTS
    stacktrace
)
# Dynamically create aliases for each Boost component after Boost is found

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
        find_package(Boost 1.82 QUIET COMPONENTS ${HELIOS_BOOST_REQUIRED_COMPONENTS})
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

    # Create stacktrace-specific alias: helios::boost_stacktrace
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

        include(DownloadUsingCPM)
        helios_cpm_add_package(
            NAME Boost
            VERSION 1.82.0
            URL https://github.com/boostorg/boost/releases/download/boost-1.82.0/boost-1.82.0-cmake.tar.xz
            OPTIONS
                "BOOST_ENABLE_CMAKE ON"
                "BOOST_INCLUDE_LIBRARIES stacktrace\\\\;unordered"
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

            if (NOT TARGET helios::boost_stacktrace)
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

# Dynamically create aliases for each Boost component

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
        message(WARNING "  ✗ Boost component '${component}' not found. Ensure the component is available in the Conan package or adjust the required components list.")
    endif()
endforeach()
