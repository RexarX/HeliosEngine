# CPM (CMake Package Manager) integration for Helios Engine
# Provides automatic dependency downloading as a fallback when system/Conan packages aren't available

include_guard(GLOBAL)

# CPM configuration options
set(CPM_DOWNLOAD_VERSION 0.40.2 CACHE STRING "CPM version to download")
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpm_cache" CACHE PATH "CPM source cache directory")
set(CPM_USE_LOCAL_PACKAGES ON CACHE BOOL "Try to use local packages before downloading")

# Determine CPM.cmake location
set(_cpm_path "")

# 1. Check if already in build directory
if(EXISTS "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
    file(SIZE "${CMAKE_BINARY_DIR}/cmake/CPM.cmake" _cpm_size)
    if(_cpm_size GREATER 0)
        set(_cpm_path "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
    endif()
endif()

# 2. Check source tree for vendored copy
if(NOT _cpm_path AND EXISTS "${CMAKE_SOURCE_DIR}/cmake/CPM.cmake")
    file(SIZE "${CMAKE_SOURCE_DIR}/cmake/CPM.cmake" _cpm_size)
    if(_cpm_size GREATER 0)
        set(_cpm_path "${CMAKE_SOURCE_DIR}/cmake/CPM.cmake")
        message(STATUS "Using vendored CPM.cmake from source tree")
    endif()
endif()

# 3. Download if not found
if(NOT _cpm_path)
    message(STATUS "Downloading CPM.cmake v${CPM_DOWNLOAD_VERSION}...")

    file(
        DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CMAKE_BINARY_DIR}/cmake/CPM.cmake"
        STATUS _cpm_download_status
        SHOW_PROGRESS
    )

    # Check download status
    list(GET _cpm_download_status 0 _cpm_status_code)
    list(GET _cpm_download_status 1 _cpm_status_message)

    if(NOT _cpm_status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download CPM.cmake: ${_cpm_status_message}\nPlease check your network connection or manually place CPM.cmake in ${CMAKE_BINARY_DIR}/cmake/ or ${CMAKE_SOURCE_DIR}/cmake/")
    else()
        # Verify the hash only on successful download
        file(SHA256 "${CMAKE_BINARY_DIR}/cmake/CPM.cmake" _cpm_hash)
        if(NOT _cpm_hash STREQUAL "c8cdc32c03816538ce22781ed72964dc864b2a34a310d3b7104812a5ca2d835d")
            message(WARNING "CPM.cmake hash mismatch. Expected: c8cdc32c03816538ce22781ed72964dc864b2a34a310d3b7104812a5ca2d835d, Got: ${_cpm_hash}")
        endif()
        set(_cpm_path "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
    endif()
endif()

# Include CPM
include("${_cpm_path}")

# Track downloaded packages
if(NOT DEFINED CPM_PACKAGES)
    set(CPM_PACKAGES "" CACHE INTERNAL "List of packages downloaded via CPM")
endif()

# Wrapper around CPMAddPackage that tracks packages and marks them as system
macro(helios_cpm_add_package)
    set(options DOWNLOAD_ONLY EXCLUDE_FROM_ALL SYSTEM)
    set(oneValueArgs NAME VERSION GIT_TAG GITHUB_REPOSITORY URL SOURCE_SUBDIR)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(CPM_ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT CPM_ARG_NAME)
        message(FATAL_ERROR "helios_cpm_add_package: NAME is required")
    endif()

    # Build CPMAddPackage arguments
    set(_cpm_args NAME ${CPM_ARG_NAME})

    if(CPM_ARG_VERSION)
        list(APPEND _cpm_args VERSION ${CPM_ARG_VERSION})
    endif()

    if(CPM_ARG_GIT_TAG)
        list(APPEND _cpm_args GIT_TAG ${CPM_ARG_GIT_TAG})
    endif()

    if(CPM_ARG_GITHUB_REPOSITORY)
        list(APPEND _cpm_args GITHUB_REPOSITORY ${CPM_ARG_GITHUB_REPOSITORY})
    endif()

    if(CPM_ARG_URL)
        list(APPEND _cpm_args URL ${CPM_ARG_URL})
    endif()

    if(CPM_ARG_SOURCE_SUBDIR)
        list(APPEND _cpm_args SOURCE_SUBDIR ${CPM_ARG_SOURCE_SUBDIR})
    endif()

    if(CPM_ARG_OPTIONS)
        list(APPEND _cpm_args OPTIONS ${CPM_ARG_OPTIONS})
    endif()

    if(CPM_ARG_DOWNLOAD_ONLY)
        list(APPEND _cpm_args DOWNLOAD_ONLY YES)
    endif()

    if(CPM_ARG_EXCLUDE_FROM_ALL)
        list(APPEND _cpm_args EXCLUDE_FROM_ALL YES)
    endif()

    # Add the package
    CPMAddPackage(${_cpm_args})

    # Track the package
    if(${CPM_ARG_NAME}_ADDED)
        list(APPEND CPM_PACKAGES ${CPM_ARG_NAME})
        set(CPM_PACKAGES "${CPM_PACKAGES}" CACHE INTERNAL "List of packages downloaded via CPM")

        # Store the version that was actually requested for this package
        if(CPM_ARG_VERSION)
            set(CPM_${CPM_ARG_NAME}_VERSION "${CPM_ARG_VERSION}" CACHE INTERNAL "CPM version for ${CPM_ARG_NAME}")
        endif()

        message(STATUS "Downloaded ${CPM_ARG_NAME} via CPM")

        # Note: SYSTEM marking is deferred - use helios_cpm_mark_as_system() function
        # after the package is fully processed to mark specific targets as system includes
    endif()

    unset(_cpm_args)
endmacro()

# Helper function to print all CPM packages
function(helios_print_cpm_packages)
    if(CPM_PACKAGES)
        message(STATUS "========== CPM Downloaded Packages ==========")
        list(REMOVE_DUPLICATES CPM_PACKAGES)
        list(SORT CPM_PACKAGES)
        foreach(_pkg ${CPM_PACKAGES})
            # Use CPM's stored version instead of the package's version variable
            # to avoid stale cached values from find_package() attempts
            if(CPM_${_pkg}_VERSION)
                message(STATUS "  ↓ ${_pkg} ${CPM_${_pkg}_VERSION}")
            else()
                message(STATUS "  ↓ ${_pkg}")
            endif()
        endforeach()
        message(STATUS "=============================================")
    endif()
endfunction()

# Convenience function for common package patterns
function(helios_cpm_add_github_package)
    set(options)
    set(oneValueArgs NAME VERSION REPOSITORY GIT_TAG)
    set(multiValueArgs OPTIONS)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_NAME OR NOT ARG_REPOSITORY)
        message(FATAL_ERROR "helios_cpm_add_github_package: NAME and REPOSITORY are required")
    endif()

    set(_args NAME ${ARG_NAME} GITHUB_REPOSITORY ${ARG_REPOSITORY})

    if(ARG_VERSION)
        list(APPEND _args VERSION ${ARG_VERSION})
    endif()

    if(ARG_GIT_TAG)
        list(APPEND _args GIT_TAG ${ARG_GIT_TAG})
    endif()

    if(ARG_OPTIONS)
        list(APPEND _args OPTIONS ${ARG_OPTIONS})
    endif()

    helios_cpm_add_package(${_args})
endfunction()

# Mark targets from CPM as system includes
# This function can be called manually if automatic marking doesn't work
function(helios_cpm_mark_as_system target)
    if(TARGET ${target})
        # Resolve ALIAS targets to their actual target
        get_target_property(_aliased_target ${target} ALIASED_TARGET)
        if(_aliased_target)
            set(_actual_target ${_aliased_target})
        else()
            set(_actual_target ${target})
        endif()

        get_target_property(_include_dirs ${_actual_target} INTERFACE_INCLUDE_DIRECTORIES)
        if(_include_dirs AND NOT _include_dirs STREQUAL "_include_dirs-NOTFOUND")
            set_target_properties(${_actual_target} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_include_dirs}"
            )
            message(STATUS "Marked ${target} with SYSTEM includes")
        endif()
    endif()
endfunction()

# Configure CPM cache
if(CPM_SOURCE_CACHE)
    file(MAKE_DIRECTORY "${CPM_SOURCE_CACHE}")
    message(STATUS "CPM cache directory: ${CPM_SOURCE_CACHE}")
endif()

message(STATUS "CPM.cmake v${CPM_DOWNLOAD_VERSION} loaded")
