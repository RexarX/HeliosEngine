# Helper macros for finding and configuring external dependencies (NOT engine modules)
#
# NOTE: This file is for EXTERNAL DEPENDENCIES (Boost, spdlog, etc.), not Helios modules.
# For Helios engine modules, see ModuleUtils.cmake

include_guard(GLOBAL)

cmake_policy(SET CMP0054 NEW)

# Initialize global variables for dependency tracking
if(NOT DEFINED _HELIOS_DEPENDENCIES_FOUND)
    set(_HELIOS_DEPENDENCIES_FOUND "" CACHE INTERNAL "List of found dependencies")
endif()

# Detect if using Conan - check multiple possible locations
if(DEFINED HELIOS_USE_CONAN)
    # Already set, use it
elseif(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    set(HELIOS_USE_CONAN TRUE CACHE BOOL "Using Conan for dependencies")
elseif(DEFINED CONAN_TOOLCHAIN)
    set(HELIOS_USE_CONAN TRUE CACHE BOOL "Using Conan for dependencies")
else()
    set(HELIOS_USE_CONAN FALSE CACHE BOOL "Using Conan for dependencies")
endif()

# Options for package management
option(HELIOS_DOWNLOAD_PACKAGES "Download missing packages using CPM" ON)
option(HELIOS_FORCE_DOWNLOAD_PACKAGES "Force download all packages even if system version exists" OFF)
option(HELIOS_CHECK_PACKAGE_VERSIONS "Check and enforce package version requirements" ON)
option(HELIOS_FORCE_CONAN "Force Conan packages first, use system only as fallback" OFF)

# Macro to begin package search
# Usage: helios_module_begin(
#     NAME <name>
#     [VERSION <version>]
#     [DEBIAN_NAMES <pkg1> <pkg2> ...]
#     [RPM_NAMES <pkg1> <pkg2> ...]
#     [PACMAN_NAMES <pkg1> <pkg2> ...]
#     [BREW_NAMES <pkg1> <pkg2> ...]
#     [PKG_CONFIG_NAMES <pkg1> <pkg2> ...]
#     [CPM_NAME <name>]
#     [CPM_VERSION <version>]
#     [CPM_GITHUB_REPOSITORY <repo>]
#     [CPM_URL <url>]
#     [CPM_OPTIONS <opt1> <opt2> ...]
#     [CPM_DOWNLOAD_ONLY]
# )
macro(helios_dep_begin)
    set(options CPM_DOWNLOAD_ONLY)
    set(oneValueArgs NAME VERSION CPM_NAME CPM_VERSION CPM_GITHUB_REPOSITORY CPM_URL CPM_GIT_TAG CPM_SOURCE_SUBDIR)
    set(multiValueArgs DEBIAN_NAMES RPM_NAMES PACMAN_NAMES BREW_NAMES PKG_CONFIG_NAMES CPM_OPTIONS)

    cmake_parse_arguments(HELIOS_PKG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(_PKG_NAME "${HELIOS_PKG_NAME}")

    # Set up CPM name if not provided
    if(NOT HELIOS_PKG_CPM_NAME)
        set(HELIOS_PKG_CPM_NAME "${HELIOS_PKG_NAME}")
    endif()

    string(TOUPPER "${HELIOS_PKG_CPM_NAME}" _PKG_CPM_NAME_UPPER)
    string(REPLACE "-" "_" _PKG_CPM_NAME_UPPER "${_PKG_CPM_NAME_UPPER}")

    # Create download options for this package
    option(
        HELIOS_DOWNLOAD_${_PKG_CPM_NAME_UPPER}
        "Download and setup ${HELIOS_PKG_CPM_NAME} if not found"
        ${HELIOS_DOWNLOAD_PACKAGES}
    )
    option(
        HELIOS_FORCE_DOWNLOAD_${_PKG_CPM_NAME_UPPER}
        "Force download ${HELIOS_PKG_CPM_NAME} even if system package exists"
        ${HELIOS_FORCE_DOWNLOAD_PACKAGES}
    )

    # Set version requirement
    if(HELIOS_PKG_VERSION)
        if(NOT ${_PKG_NAME}_FIND_VERSION OR "${${_PKG_NAME}_FIND_VERSION}" VERSION_LESS "${HELIOS_PKG_VERSION}")
            set("${_PKG_NAME}_FIND_VERSION" "${HELIOS_PKG_VERSION}")
        endif()
    endif()

    # Skip version checks if disabled
    if(NOT HELIOS_CHECK_PACKAGE_VERSIONS)
        unset("${_PKG_NAME}_FIND_VERSION")
    endif()

    # Check if already found
    if(TARGET ${_PKG_NAME} OR TARGET helios::${_PKG_NAME})
        if(NOT ${_PKG_NAME}_FIND_VERSION)
            set("${_PKG_NAME}_FOUND" ON)
            set("${_PKG_NAME}_SKIP_HELIOS_FIND" ON)
            return()
        endif()

        if(${_PKG_NAME}_VERSION)
            if(${_PKG_NAME}_FIND_VERSION VERSION_LESS_EQUAL ${_PKG_NAME}_VERSION)
                set("${_PKG_NAME}_FOUND" ON)
                set("${_PKG_NAME}_SKIP_HELIOS_FIND" ON)
                return()
            else()
                message(FATAL_ERROR
                    "Already using version ${${_PKG_NAME}_VERSION} of ${_PKG_NAME} "
                    "when version ${${_PKG_NAME}_FIND_VERSION} was requested."
                )
            endif()
        endif()
    endif()

    # Build error message for missing packages
    set(_ERROR_MESSAGE "Could not find `${_PKG_NAME}` package.")
    if(HELIOS_PKG_DEBIAN_NAMES)
        list(JOIN HELIOS_PKG_DEBIAN_NAMES " " _pkg_names)
        string(APPEND _ERROR_MESSAGE "\n\tDebian/Ubuntu: sudo apt install ${_pkg_names}")
    endif()
    if(HELIOS_PKG_RPM_NAMES)
        list(JOIN HELIOS_PKG_RPM_NAMES " " _pkg_names)
        string(APPEND _ERROR_MESSAGE "\n\tFedora/RHEL: sudo dnf install ${_pkg_names}")
    endif()
    if(HELIOS_PKG_PACMAN_NAMES)
        list(JOIN HELIOS_PKG_PACMAN_NAMES " " _pkg_names)
        string(APPEND _ERROR_MESSAGE "\n\tArch Linux: sudo pacman -S ${_pkg_names}")
    endif()
    if(HELIOS_PKG_BREW_NAMES)
        list(JOIN HELIOS_PKG_BREW_NAMES " " _pkg_names)
        string(APPEND _ERROR_MESSAGE "\n\tmacOS: brew install ${_pkg_names}")
    endif()
    string(APPEND _ERROR_MESSAGE "\n")

    # Initialize search result variables
    set("${_PKG_NAME}_LIBRARIES")
    set("${_PKG_NAME}_INCLUDE_DIRS")
    set("${_PKG_NAME}_EXECUTABLE")
    set("${_PKG_NAME}_FOUND" FALSE)
endmacro()

# Helper to search for package components
macro(_helios_dep_find_part)
    set(options)
    set(oneValueArgs PART_TYPE)
    set(multiValueArgs NAMES PATHS PATH_SUFFIXES)

    cmake_parse_arguments(PART "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(${_PKG_NAME}_SKIP_HELIOS_FIND)
        return()
    endif()

    # Determine variable name based on type
    if("${PART_PART_TYPE}" STREQUAL "library")
        set(_variable "${_PKG_NAME}_LIBRARIES")
        find_library(${_PKG_NAME}_LIBRARIES_${PART_NAMES}
            NAMES ${PART_NAMES}
            PATHS ${PART_PATHS}
            PATH_SUFFIXES ${PART_PATH_SUFFIXES}
        )
        list(APPEND "${_variable}" "${${_PKG_NAME}_LIBRARIES_${PART_NAMES}}")
    elseif("${PART_PART_TYPE}" STREQUAL "include")
        set(_variable "${_PKG_NAME}_INCLUDE_DIRS")
        find_path(${_PKG_NAME}_INCLUDE_DIRS_${PART_NAMES}
            NAMES ${PART_NAMES}
            PATHS ${PART_PATHS}
            PATH_SUFFIXES ${PART_PATH_SUFFIXES}
        )
        list(APPEND "${_variable}" "${${_PKG_NAME}_INCLUDE_DIRS_${PART_NAMES}}")
    elseif("${PART_PART_TYPE}" STREQUAL "program")
        set(_variable "${_PKG_NAME}_EXECUTABLE")
        find_program(${_PKG_NAME}_EXECUTABLE_${PART_NAMES}
            NAMES ${PART_NAMES}
            PATHS ${PART_PATHS}
            PATH_SUFFIXES ${PART_PATH_SUFFIXES}
        )
        list(APPEND "${_variable}" "${${_PKG_NAME}_EXECUTABLE_${PART_NAMES}}")
    else()
        message(FATAL_ERROR "Invalid PART_TYPE: ${PART_PART_TYPE}")
    endif()
endmacro()

# Find library component
macro(helios_dep_find_library)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs NAMES PATHS PATH_SUFFIXES)

    cmake_parse_arguments(LIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    _helios_dep_find_part(
        PART_TYPE library
        NAMES ${LIB_NAMES}
        PATHS ${LIB_PATHS}
        PATH_SUFFIXES ${LIB_PATH_SUFFIXES}
    )
endmacro()

# Find include component
macro(helios_dep_find_include)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs NAMES PATHS PATH_SUFFIXES)

    cmake_parse_arguments(INC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    _helios_dep_find_part(
        PART_TYPE include
        NAMES ${INC_NAMES}
        PATHS ${INC_PATHS}
        PATH_SUFFIXES ${INC_PATH_SUFFIXES}
    )
endmacro()

# Find program component
macro(helios_dep_find_program)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs NAMES PATHS PATH_SUFFIXES)

    cmake_parse_arguments(PROG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    _helios_dep_find_part(
        PART_TYPE program
        NAMES ${PROG_NAMES}
        PATHS ${PROG_PATHS}
        PATH_SUFFIXES ${PROG_PATH_SUFFIXES}
    )
endmacro()

# Finalize package search
macro(helios_dep_end)
    if(${_PKG_NAME}_SKIP_HELIOS_FIND)
        return()
    endif()

    # Try to find via standard mechanisms first
    set(_FOUND_VIA "")

    # Exclude vcpkg paths when using Conan to prevent conflicts
    set(_ORIG_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    set(_ORIG_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
    if(HELIOS_USE_CONAN AND DEFINED ENV{VCPKG_ROOT})
        # Remove vcpkg paths from search
        list(FILTER CMAKE_PREFIX_PATH EXCLUDE REGEX "vcpkg")
        list(FILTER CMAKE_FIND_ROOT_PATH EXCLUDE REGEX "vcpkg")
    endif()

    # Determine search order based on preferences
    # Default (HELIOS_FORCE_CONAN=OFF):
    #   1. System packages (preferred)
    #   2. Conan (if available)
    #   3. pkg-config
    #   4. Manual search
    #   5. CPM download
    #
    # With HELIOS_FORCE_CONAN=ON (--use-conan flag):
    #   1. Conan packages (preferred)
    #   2. System packages (fallback)
    #   3. pkg-config
    #   4. Manual search
    #   5. CPM download

    set(_tried_system FALSE)
    set(_tried_conan FALSE)

    # If HELIOS_FORCE_CONAN is ON, try Conan first
    if(HELIOS_FORCE_CONAN AND HELIOS_USE_CONAN AND NOT HELIOS_FORCE_DOWNLOAD_${_PKG_CPM_NAME_UPPER})
        find_package(${_PKG_NAME} ${${_PKG_NAME}_FIND_VERSION} CONFIG QUIET)
        if(${_PKG_NAME}_FOUND)
            set(_FOUND_VIA "Conan")
        endif()
        set(_tried_conan TRUE)
    endif()

    # Try system packages (either first by default, or second if forcing Conan)
    if(NOT ${_PKG_NAME}_FOUND AND NOT HELIOS_FORCE_DOWNLOAD_${_PKG_CPM_NAME_UPPER})
        # Try CONFIG mode first (for CMake-aware packages)
        find_package(${_PKG_NAME} ${${_PKG_NAME}_FIND_VERSION} CONFIG QUIET)
        if(${_PKG_NAME}_FOUND)
            set(_FOUND_VIA "system (CONFIG)")
        else()
            # Try MODULE mode (for Find*.cmake files)
            find_package(${_PKG_NAME} ${${_PKG_NAME}_FIND_VERSION} MODULE QUIET)
            if(${_PKG_NAME}_FOUND)
                set(_FOUND_VIA "system (MODULE)")
            endif()
        endif()
        set(_tried_system TRUE)
    endif()

    # Try Conan if not found yet and we didn't try it earlier
    if(NOT ${_PKG_NAME}_FOUND AND NOT _tried_conan AND HELIOS_USE_CONAN AND NOT HELIOS_FORCE_DOWNLOAD_${_PKG_CPM_NAME_UPPER})
        find_package(${_PKG_NAME} ${${_PKG_NAME}_FIND_VERSION} CONFIG QUIET)
        if(${_PKG_NAME}_FOUND)
            set(_FOUND_VIA "Conan")
        endif()
        set(_tried_conan TRUE)
    endif()

    # Restore original paths
    set(CMAKE_PREFIX_PATH ${_ORIG_CMAKE_PREFIX_PATH})
    set(CMAKE_FIND_ROOT_PATH ${_ORIG_CMAKE_FIND_ROOT_PATH})

    # 3. Try pkg-config
    if(NOT ${_PKG_NAME}_FOUND AND HELIOS_PKG_PKG_CONFIG_NAMES)
        find_package(PkgConfig QUIET)
        if(PKG_CONFIG_FOUND)
            list(GET HELIOS_PKG_PKG_CONFIG_NAMES 0 _pkg_config_name)
            pkg_check_modules(${_PKG_NAME}_PC QUIET IMPORTED_TARGET ${_pkg_config_name})
            if(${_PKG_NAME}_PC_FOUND)
                set(${_PKG_NAME}_FOUND TRUE)
                set(_FOUND_VIA "pkg-config")
                if(NOT TARGET ${_PKG_NAME})
                    add_library(${_PKG_NAME} INTERFACE IMPORTED)
                    target_link_libraries(${_PKG_NAME} INTERFACE PkgConfig::${_PKG_NAME}_PC)
                endif()
            endif()
        endif()
    endif()

    # 4. Try manual search if we have search criteria
    set(_required_vars)
    if(NOT "${${_PKG_NAME}_LIBRARIES}" STREQUAL "")
        list(APPEND _required_vars "${_PKG_NAME}_LIBRARIES")
    endif()
    if(NOT "${${_PKG_NAME}_INCLUDE_DIRS}" STREQUAL "")
        list(APPEND _required_vars "${_PKG_NAME}_INCLUDE_DIRS")
    endif()
    if(NOT "${${_PKG_NAME}_EXECUTABLE}" STREQUAL "")
        list(APPEND _required_vars "${_PKG_NAME}_EXECUTABLE")
    endif()

    if(_required_vars AND NOT ${_PKG_NAME}_FOUND)
        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(
            ${_PKG_NAME}
            REQUIRED_VARS ${_required_vars}
            VERSION_VAR ${_PKG_NAME}_VERSION
            FAIL_MESSAGE "${_ERROR_MESSAGE}"
        )
        if(${_PKG_NAME}_FOUND)
            set(_FOUND_VIA "manual search")
        endif()
    endif()

    # 5. Try CPM as last resort
    if(NOT ${_PKG_NAME}_FOUND AND HELIOS_DOWNLOAD_${_PKG_CPM_NAME_UPPER})
        if(HELIOS_PKG_CPM_GITHUB_REPOSITORY OR HELIOS_PKG_CPM_URL)
            include(DownloadUsingCPM)
            _helios_cpm_add_package()
            if(${_PKG_NAME}_ADDED OR TARGET ${_PKG_NAME})
                set(${_PKG_NAME}_FOUND TRUE)
                set(_FOUND_VIA "CPM")
            endif()
        endif()
    endif()

    # Report results
    if(${_PKG_NAME}_FOUND)
        message(STATUS "helios::${_PKG_NAME} found via ${_FOUND_VIA}")

        # Create interface target if needed
        if(_required_vars AND NOT TARGET ${_PKG_NAME})
            add_library(${_PKG_NAME} INTERFACE IMPORTED GLOBAL)

            if(${_PKG_NAME}_INCLUDE_DIRS)
                set_target_properties(${_PKG_NAME} PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${${_PKG_NAME}_INCLUDE_DIRS}"
                    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${${_PKG_NAME}_INCLUDE_DIRS}"
                )
            endif()

            if(${_PKG_NAME}_LIBRARIES)
                set_target_properties(${_PKG_NAME} PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${${_PKG_NAME}_LIBRARIES}"
                )
            endif()
        endif()

        # Create helios:: alias if it doesn't exist
        if(NOT TARGET helios::${_PKG_NAME})
            # Try different common target naming patterns
            # Check which target exists and get its ALIASED_TARGET property to avoid aliasing an alias
            set(_target_to_alias "")

            if(TARGET ${_PKG_NAME})
                get_target_property(_aliased ${_PKG_NAME} ALIASED_TARGET)
                if(_aliased)
                    set(_target_to_alias ${_aliased})
                else()
                    set(_target_to_alias ${_PKG_NAME})
                endif()
            elseif(TARGET ${_PKG_NAME}::${_PKG_NAME})
                # Many packages create targets like spdlog::spdlog, fmt::fmt, etc.
                get_target_property(_aliased ${_PKG_NAME}::${_PKG_NAME} ALIASED_TARGET)
                if(_aliased)
                    set(_target_to_alias ${_aliased})
                else()
                    set(_target_to_alias ${_PKG_NAME}::${_PKG_NAME})
                endif()
            elseif(TARGET ${_PKG_NAME}::${_PKG_NAME}_header_only)
                # Some header-only packages use this pattern
                get_target_property(_aliased ${_PKG_NAME}::${_PKG_NAME}_header_only ALIASED_TARGET)
                if(_aliased)
                    set(_target_to_alias ${_aliased})
                else()
                    set(_target_to_alias ${_PKG_NAME}::${_PKG_NAME}_header_only)
                endif()
            endif()

            if(_target_to_alias)
                add_library(helios::${_PKG_NAME} ALIAS ${_target_to_alias})

                # Mark target includes as SYSTEM to suppress warnings
                get_target_property(_target_includes ${_target_to_alias} INTERFACE_INCLUDE_DIRECTORIES)
                if(_target_includes)
                    set_target_properties(${_target_to_alias} PROPERTIES
                        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_target_includes}"
                    )
                endif()
            endif()

            unset(_target_to_alias)
            unset(_aliased)
        endif()

        # Track found dependency
        list(APPEND _HELIOS_DEPENDENCIES_FOUND ${_PKG_NAME})
        set(_HELIOS_DEPENDENCIES_FOUND "${_HELIOS_DEPENDENCIES_FOUND}" CACHE INTERNAL "List of found dependencies")
    else()
        if(${_PKG_NAME}_FIND_REQUIRED)
            message(FATAL_ERROR "${_ERROR_MESSAGE}")
        else()
            message(WARNING "${_ERROR_MESSAGE}")
        endif()
    endif()

    # Clean up
    unset(_FOUND_VIA)
    unset(_required_vars)
    unset(_ERROR_MESSAGE)
    unset(_PKG_NAME)
    unset(_PKG_CPM_NAME_UPPER)
endmacro()

# Internal helper to add package via CPM
macro(_helios_cpm_add_package)
    set(_cpm_args NAME ${HELIOS_PKG_CPM_NAME})

    if(HELIOS_PKG_CPM_VERSION)
        list(APPEND _cpm_args VERSION ${HELIOS_PKG_CPM_VERSION})
    endif()

    if(HELIOS_PKG_CPM_GITHUB_REPOSITORY)
        list(APPEND _cpm_args GITHUB_REPOSITORY ${HELIOS_PKG_CPM_GITHUB_REPOSITORY})
    endif()

    if(HELIOS_PKG_CPM_URL)
        list(APPEND _cpm_args URL ${HELIOS_PKG_CPM_URL})
    endif()

    if(HELIOS_PKG_CPM_GIT_TAG)
        list(APPEND _cpm_args GIT_TAG ${HELIOS_PKG_CPM_GIT_TAG})
    endif()

    if(HELIOS_PKG_CPM_SOURCE_SUBDIR)
        list(APPEND _cpm_args SOURCE_SUBDIR ${HELIOS_PKG_CPM_SOURCE_SUBDIR})
    endif()

    if(HELIOS_PKG_CPM_OPTIONS)
        list(APPEND _cpm_args OPTIONS ${HELIOS_PKG_CPM_OPTIONS})
    endif()

    if(HELIOS_PKG_CPM_DOWNLOAD_ONLY)
        list(APPEND _cpm_args DOWNLOAD_ONLY YES)
    endif()

    CPMAddPackage(${_cpm_args})

    # Mark as system to suppress warnings
    if(${HELIOS_PKG_CPM_NAME}_SOURCE_DIR)
        set_property(DIRECTORY ${${HELIOS_PKG_CPM_NAME}_SOURCE_DIR} PROPERTY SYSTEM TRUE)

        # Also mark all targets from this package as SYSTEM
        if(${HELIOS_PKG_CPM_NAME}_ADDED)
            # Get all targets defined in the source directory
            get_directory_property(_cpm_targets DIRECTORY ${${HELIOS_PKG_CPM_NAME}_SOURCE_DIR} BUILDSYSTEM_TARGETS)
            foreach(_target ${_cpm_targets})
                if(TARGET ${_target})
                    get_target_property(_target_type ${_target} TYPE)
                    if(_target_type MATCHES "INTERFACE_LIBRARY|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY")
                        get_target_property(_target_includes ${_target} INTERFACE_INCLUDE_DIRECTORIES)
                        if(_target_includes)
                            set_target_properties(${_target} PROPERTIES
                                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_target_includes}"
                            )
                        endif()
                    endif()
                endif()
            endforeach()
        endif()
    endif()

    unset(_cpm_args)
endmacro()

# Function to print all found dependencies
function(helios_print_dependencies)
    message(STATUS "========== Helios Engine Dependencies ==========")
    if(_HELIOS_DEPENDENCIES_FOUND)
        list(REMOVE_DUPLICATES _HELIOS_DEPENDENCIES_FOUND)
        list(SORT _HELIOS_DEPENDENCIES_FOUND)
        foreach(_dep ${_HELIOS_DEPENDENCIES_FOUND})
            message(STATUS "  ✓ ${_dep}")
        endforeach()
    else()
        message(STATUS "  No dependencies found")
    endif()
    message(STATUS "================================================")
endfunction()
