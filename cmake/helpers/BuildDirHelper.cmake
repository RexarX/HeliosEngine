# BuildDirHelper.cmake
# Helper module to manage build directory structure based on platform and build type

include_guard(GLOBAL)

# Get the platform identifier in lowercase with standardized names
# Uses: windows, linux, macos (not Darwin)
function(helios_get_platform_identifier OUTPUT_VAR)
    if(WIN32)
        set(${OUTPUT_VAR} "windows" PARENT_SCOPE)
    elseif(APPLE)
        set(${OUTPUT_VAR} "macos" PARENT_SCOPE)
    elseif(UNIX)
        set(${OUTPUT_VAR} "linux" PARENT_SCOPE)
    else()
        string(TOLOWER "${CMAKE_SYSTEM_NAME}" _platform_lower)
        set(${OUTPUT_VAR} "${_platform_lower}" PARENT_SCOPE)
    endif()
endfunction()

# Get the build type in lowercase
function(helios_get_build_type_identifier OUTPUT_VAR)
    if(CMAKE_BUILD_TYPE)
        string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lower)
        set(${OUTPUT_VAR} "${_build_type_lower}" PARENT_SCOPE)
    elseif(CMAKE_CONFIGURATION_TYPES)
        # Multi-config generator, use generator expression
        set(${OUTPUT_VAR} "$<LOWER_CASE:$<CONFIG>>" PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} "unknown" PARENT_SCOPE)
    endif()
endfunction()

# Get the standardized build directory path
# Structure: build/<build_type>/<platform>/
function(helios_get_build_directory OUTPUT_VAR)
    cmake_parse_arguments(ARG "" "BASE_DIR" "" ${ARGN})

    if(NOT ARG_BASE_DIR)
        set(ARG_BASE_DIR "${CMAKE_SOURCE_DIR}/build")
    endif()

    helios_get_build_type_identifier(_build_type)
    helios_get_platform_identifier(_platform)

    # For multi-config generators, we can't use generator expressions in paths
    # So we'll use a simpler structure
    if(CMAKE_CONFIGURATION_TYPES)
        # Multi-config: build/<platform>/ (config is handled by generator)
        set(${OUTPUT_VAR} "${ARG_BASE_DIR}/${_platform}" PARENT_SCOPE)
    else()
        # Single-config: build/<build_type>/<platform>/
        set(${OUTPUT_VAR} "${ARG_BASE_DIR}/${_build_type}/${_platform}" PARENT_SCOPE)
    endif()
endfunction()

# Get Conan toolchain path for current configuration
function(helios_get_conan_toolchain_path OUTPUT_VAR)
    helios_get_build_type_identifier(_build_type)
    helios_get_platform_identifier(_platform)

    if(CMAKE_CONFIGURATION_TYPES)
        # Multi-config: build/<platform>/
        set(_conan_dir "${CMAKE_SOURCE_DIR}/build/${_platform}")
    else()
        # Single-config: build/<build_type>/<platform>/
        set(_conan_dir "${CMAKE_SOURCE_DIR}/build/${_build_type}/${_platform}")
    endif()

    set(${OUTPUT_VAR} "${_conan_dir}/conan_toolchain.cmake" PARENT_SCOPE)
endfunction()

# Print current build directory configuration
function(helios_print_build_dir_info)
    helios_get_build_type_identifier(_build_type)
    helios_get_platform_identifier(_platform)
    helios_get_build_directory(_build_dir)

    message(STATUS "========== Build Directory Configuration ==========")
    message(STATUS "  Platform:   ${_platform}")
    message(STATUS "  Build Type: ${_build_type}")
    message(STATUS "  Build Dir:  ${_build_dir}")
    if(CMAKE_CONFIGURATION_TYPES)
        message(STATUS "  Generator:  Multi-config (${CMAKE_GENERATOR})")
    else()
        message(STATUS "  Generator:  Single-config (${CMAKE_GENERATOR})")
    endif()
    message(STATUS "===================================================")
endfunction()
