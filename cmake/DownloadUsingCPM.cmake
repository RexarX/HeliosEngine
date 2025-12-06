# CPM (CMake Package Manager) integration for Helios Engine
# Provides automatic dependency downloading as a fallback when system/Conan packages aren't available

include_guard(GLOBAL)

# CPM configuration options
set(CPM_DOWNLOAD_VERSION 0.40.2 CACHE STRING "CPM version to download")
set(CPM_SOURCE_CACHE "${CMAKE_SOURCE_DIR}/.cpm_cache" CACHE PATH "CPM source cache directory")
set(CPM_USE_LOCAL_PACKAGES ON CACHE BOOL "Try to use local packages before downloading")

# Download CPM.cmake if not already available
if(NOT EXISTS "${CMAKE_BINARY_DIR}/cmake/CPM.cmake")
    message(STATUS "Downloading CPM.cmake v${CPM_DOWNLOAD_VERSION}...")

    file(
        DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CMAKE_BINARY_DIR}/cmake/CPM.cmake"
        EXPECTED_HASH SHA256=c8cdc32c03816538ce22781ed72964dc864b2a34a310d3b7104812a5ca2d835d
        SHOW_PROGRESS
    )
endif()

# Include CPM
include("${CMAKE_BINARY_DIR}/cmake/CPM.cmake")

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

        message(STATUS "Downloaded ${CPM_ARG_NAME} via CPM")

        # Mark as system to suppress warnings (unless explicitly disabled)
        if((CPM_ARG_SYSTEM OR NOT DEFINED CPM_ARG_SYSTEM) AND ${CPM_ARG_NAME}_SOURCE_DIR)
            if(EXISTS "${${CPM_ARG_NAME}_SOURCE_DIR}")
                set_property(
                    DIRECTORY "${${CPM_ARG_NAME}_SOURCE_DIR}"
                    PROPERTY SYSTEM TRUE
                )

                # Also mark all subdirectories
                file(GLOB_RECURSE _subdirs "${${CPM_ARG_NAME}_SOURCE_DIR}/*CMakeLists.txt")
                foreach(_subdir ${_subdirs})
                    get_filename_component(_dir "${_subdir}" DIRECTORY)
                    set_property(DIRECTORY "${_dir}" PROPERTY SYSTEM TRUE)
                endforeach()

                # Mark all targets from this package with SYSTEM includes
                get_directory_property(_cpm_targets DIRECTORY "${${CPM_ARG_NAME}_SOURCE_DIR}" BUILDSYSTEM_TARGETS)
                foreach(_target ${_cpm_targets})
                    if(TARGET ${_target})
                        get_target_property(_target_type ${_target} TYPE)
                        # Only process library targets
                        if(_target_type MATCHES "INTERFACE_LIBRARY|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY")
                            get_target_property(_target_includes ${_target} INTERFACE_INCLUDE_DIRECTORIES)
                            if(_target_includes AND NOT _target_includes STREQUAL "_target_includes-NOTFOUND")
                                set_target_properties(${_target} PROPERTIES
                                    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_target_includes}"
                                )
                            endif()
                        endif()
                    endif()
                endforeach()
            endif()
        endif()
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
            if(${_pkg}_VERSION)
                message(STATUS "  ↓ ${_pkg} ${${_pkg}_VERSION}")
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
        get_target_property(_include_dirs ${target} INTERFACE_INCLUDE_DIRECTORIES)
        if(_include_dirs AND NOT _include_dirs STREQUAL "_include_dirs-NOTFOUND")
            set_target_properties(${target} PROPERTIES
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
