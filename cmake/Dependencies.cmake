# Main dependency configuration for Helios Engine
#
# This file orchestrates all dependency finding and configuration.
# Each dependency is defined in its own file in cmake/dependencies/
#
# Strategy: Conan (if available) -> System packages -> CPM fallback
#
# The three-tier approach:
# 1. Conan packages (if using Conan toolchain)
# 2. System packages (find_package, pkg-config)
# 3. CPM automatic download (as last resort)
#
# Module-aware dependency loading:
# - Core dependencies are always loaded
# - Module dependencies are only loaded if the module is enabled
# - Dependencies are discovered by scanning Module.cmake files

include_guard(GLOBAL)

# Load dependency management helpers
include(DependencyFinder)
include(DownloadUsingCPM)

# Print configuration header
message(STATUS "")
message(STATUS "========== Helios Dependency Configuration ==========")
message(STATUS "Using Conan: ${HELIOS_USE_CONAN}")
message(STATUS "Force Conan priority: ${HELIOS_FORCE_CONAN}")
if(HELIOS_FORCE_CONAN)
    message(STATUS "  → Conan packages will be checked FIRST")
    message(STATUS "  → System packages used as fallback")
    message(STATUS "  → CPM downloads for missing dependencies")
else()
    message(STATUS "  → System packages will be checked FIRST")
    message(STATUS "  → Conan used for missing dependencies (if available)")
    message(STATUS "  → CPM downloads as last resort")
endif()
message(STATUS "Allow CPM downloads: ${HELIOS_DOWNLOAD_PACKAGES}")
message(STATUS "Check package versions: ${HELIOS_CHECK_PACKAGE_VERSIONS}")
message(STATUS "=====================================================")
message(STATUS "")

# ============================================================================
# Core Dependencies (always required)
# ============================================================================

message(STATUS "Finding Core Dependencies...")
message(STATUS "")

# Include each core dependency
# Each file handles finding the dependency from Conan, system, or CPM
include(cmake/dependencies/Boost.cmake)
include(cmake/dependencies/spdlog.cmake)
include(cmake/dependencies/stduuid.cmake)
include(cmake/dependencies/TBB.cmake)
include(cmake/dependencies/Taskflow.cmake)
include(cmake/dependencies/concurrentqueue.cmake)

message(STATUS "")

# ============================================================================
# Module Dependencies (only if not building core-only)
# Scan Module.cmake files to determine which dependencies are needed
# ============================================================================

if(NOT HELIOS_BUILD_CORE_ONLY)
    message(STATUS "Scanning modules for dependencies...")
    message(STATUS "")

    # Collect all external dependencies required by enabled modules
    set(_HELIOS_MODULE_EXTERNAL_DEPS "")

    # Get list of module directories
    set(_modules_dir "${HELIOS_ROOT_DIR}/src/modules")
    if(EXISTS "${_modules_dir}")
        file(GLOB _module_dirs RELATIVE "${_modules_dir}" "${_modules_dir}/*")

        foreach(_module_dir ${_module_dirs})
            set(_module_path "${_modules_dir}/${_module_dir}")

            # Skip if not a directory
            if(NOT IS_DIRECTORY "${_module_path}")
                continue()
            endif()

            # Skip if no CMakeLists.txt (not a valid module)
            if(NOT EXISTS "${_module_path}/CMakeLists.txt")
                continue()
            endif()

            # Convert module name to uppercase for option name
            string(TOUPPER "${_module_dir}" _module_upper)
            set(_option_name "HELIOS_BUILD_${_module_upper}_MODULE")

            # Check if module is enabled (default ON if option not set)
            if(NOT DEFINED ${_option_name})
                set(${_option_name} ON)
            endif()

            if(${_option_name})
                # Check for Module.cmake and extract external dependencies
                set(_module_cmake "${_module_path}/Module.cmake")
                if(EXISTS "${_module_cmake}")
                    # Read the Module.cmake file content
                    file(READ "${_module_cmake}" _module_cmake_content)

                    # Extract EXTERNAL_DEPENDS from helios_register_module call
                    # Look for pattern: EXTERNAL_DEPENDS followed by package names
                    string(REGEX MATCH "EXTERNAL_DEPENDS[^\n)]*" _ext_deps_match "${_module_cmake_content}")
                    if(_ext_deps_match)
                        # Remove "EXTERNAL_DEPENDS" prefix and extract package names
                        string(REPLACE "EXTERNAL_DEPENDS" "" _ext_deps_cleaned "${_ext_deps_match}")
                        # Remove leading/trailing whitespace and split
                        string(STRIP "${_ext_deps_cleaned}" _ext_deps_cleaned)
                        # Convert to list by separating on whitespace
                        string(REGEX REPLACE "[ \t\r\n]+" ";" _ext_deps_list "${_ext_deps_cleaned}")
                        # Filter out empty strings
                        foreach(_dep ${_ext_deps_list})
                            string(STRIP "${_dep}" _dep)
                            if(_dep AND NOT _dep STREQUAL "")
                                list(APPEND _HELIOS_MODULE_EXTERNAL_DEPS "${_dep}")
                                message(STATUS "  Module '${_module_dir}' requires: ${_dep}")
                            endif()
                        endforeach()
                    endif()
                endif()
            endif()
        endforeach()

        # Remove duplicates from dependency list
        if(_HELIOS_MODULE_EXTERNAL_DEPS)
            list(REMOVE_DUPLICATES _HELIOS_MODULE_EXTERNAL_DEPS)
        endif()
    endif()

    message(STATUS "")
    message(STATUS "Finding Module Dependencies...")
    message(STATUS "")

    # Include dependency files only for required external dependencies
    # Map dependency names to their cmake files
    set(_DEP_FILE_MAP_glfw "cmake/dependencies/glfw.cmake")
    set(_DEP_FILE_MAP_glfw3 "cmake/dependencies/glfw.cmake")
    # Add more mappings as needed:
    # set(_DEP_FILE_MAP_vulkan "cmake/dependencies/Vulkan.cmake")
    # set(_DEP_FILE_MAP_opengl "cmake/dependencies/OpenGL.cmake")

    # Track which dependency files have been included
    set(_INCLUDED_DEP_FILES "")

    foreach(_dep ${_HELIOS_MODULE_EXTERNAL_DEPS})
        # Normalize dependency name to lowercase for lookup
        string(TOLOWER "${_dep}" _dep_lower)

        # Check if we have a mapping for this dependency
        if(DEFINED _DEP_FILE_MAP_${_dep_lower})
            set(_dep_file "${_DEP_FILE_MAP_${_dep_lower}}")
        elseif(DEFINED _DEP_FILE_MAP_${_dep})
            set(_dep_file "${_DEP_FILE_MAP_${_dep}}")
        else()
            # Try default naming: cmake/dependencies/{dep}.cmake
            set(_dep_file "cmake/dependencies/${_dep}.cmake")
        endif()

        # Check if file exists and hasn't been included yet
        if(EXISTS "${HELIOS_ROOT_DIR}/${_dep_file}")
            list(FIND _INCLUDED_DEP_FILES "${_dep_file}" _already_included)
            if(_already_included EQUAL -1)
                message(STATUS "Loading dependency: ${_dep} (${_dep_file})")
                include("${_dep_file}")
                list(APPEND _INCLUDED_DEP_FILES "${_dep_file}")
            endif()
        else()
            message(WARNING "No dependency file found for '${_dep}' (expected: ${_dep_file})")
        endif()
    endforeach()

    message(STATUS "")
endif()

# ============================================================================
# Test Dependencies (optional, only if building tests)
# ============================================================================

if(HELIOS_BUILD_TESTS)
    message(STATUS "Finding Test Dependencies...")
    message(STATUS "")

    include(cmake/dependencies/doctest.cmake)

    message(STATUS "")
endif()

# ============================================================================
# Summary
# ============================================================================

helios_print_dependencies()
if(CPM_PACKAGES)
    helios_print_cpm_packages()
endif()
message(STATUS "")
