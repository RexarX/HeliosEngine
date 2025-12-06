# Helios Engine Module Utilities
#
# Provides consistent CMake functions for declaring engine modules.
# All modules follow a standardized structure and configuration.

include_guard(GLOBAL)

# Function: helios_add_module
#
# Creates a standardized Helios engine module with consistent configuration.
#
# Usage:
#   helios_add_module(
#     NAME <name>                          # Required: Module name
#     VERSION <version>                    # Optional: Module version (default: 1.0.0)
#
#     SOURCES <files...>                   # Source files
#     HEADERS <files...>                   # Header files
#     PUBLIC_HEADERS <files...>            # Public headers (alternative)
#     PRIVATE_HEADERS <files...>           # Private headers (alternative)
#
#     DEPENDENCIES <targets...>            # Public dependencies
#     PUBLIC_DEPENDENCIES <targets...>     # Explicit public dependencies
#     PRIVATE_DEPENDENCIES <targets...>    # Private dependencies
#
#     COMPILE_DEFINITIONS <defs...>        # Compile definitions
#     COMPILE_OPTIONS <opts...>            # Compile options
#     INCLUDE_DIRECTORIES <dirs...>        # Additional include directories
#
#     PCH <file>                           # Precompiled header file
#
#     # Options
#     HEADER_ONLY                          # Create header-only (INTERFACE) library
#     INTERFACE                            # Alias for HEADER_ONLY
#     STATIC_ONLY                          # Force static library
#     SHARED_ONLY                          # Force shared library
#
#     # Organization
#     FOLDER <folder>                      # IDE folder (default: "Helios/Modules")
#     OUTPUT_NAME <name>                   # Override output library name
#     TARGET_NAME <name>                   # Override target name (default: helios_module_{NAME})
#   )
#
# The function automatically:
# - Creates CMake target with name: helios_module_{NAME}
# - Creates alias target: helios::module::{NAME}
# - Applies standard Helios configurations (C++23, warnings, optimizations, etc.)
# - Sets up include directories (include/ and src/)
# - Links to helios::core by default
# - Applies PCH if specified
# - Enables Unity build if HELIOS_ENABLE_UNITY_BUILD is ON
# - Enables LTO if HELIOS_ENABLE_LTO is ON
# - Sets IDE folder for organization
#
# Example:
#   helios_add_module(
#     NAME window
#     VERSION 1.2.0
#     SOURCES src/window.cpp src/glfw_window.cpp
#     HEADERS include/helios/modules/window/window.hpp
#     PUBLIC_DEPENDENCIES helios::module::input glfw
#     PCH include/helios/modules/window/window_pch.hpp
#   )
#
function(helios_add_module)
    # Parse arguments
    set(options
        HEADER_ONLY
        INTERFACE
        STATIC_ONLY
        SHARED_ONLY
    )
    set(oneValueArgs
        NAME
        VERSION
        TARGET_NAME
        OUTPUT_NAME
        FOLDER
        PCH
    )
    set(multiValueArgs
        SOURCES
        HEADERS
        PUBLIC_HEADERS
        PRIVATE_HEADERS
        DEPENDENCIES
        PUBLIC_DEPENDENCIES
        PRIVATE_DEPENDENCIES
        COMPILE_DEFINITIONS
        COMPILE_OPTIONS
        INCLUDE_DIRECTORIES
    )

    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT MODULE_NAME)
        message(FATAL_ERROR "helios_add_module: NAME is required")
    endif()

    # Set defaults
    if(NOT MODULE_VERSION)
        set(MODULE_VERSION "1.0.0")
    endif()

    if(NOT MODULE_TARGET_NAME)
        set(MODULE_TARGET_NAME "helios_module_${MODULE_NAME}")
    endif()

    if(NOT MODULE_FOLDER)
        set(MODULE_FOLDER "Helios/Modules")
    endif()

    # Combine header lists
    set(_all_headers ${MODULE_HEADERS} ${MODULE_PUBLIC_HEADERS} ${MODULE_PRIVATE_HEADERS})

    # INTERFACE is alias for HEADER_ONLY
    if(MODULE_INTERFACE)
        set(MODULE_HEADER_ONLY TRUE)
    endif()

    # Determine library type
    if(MODULE_HEADER_ONLY)
        set(_library_type INTERFACE)
        set(_target_scope INTERFACE)
    elseif(MODULE_STATIC_ONLY)
        set(_library_type STATIC)
        set(_target_scope PUBLIC)
    elseif(MODULE_SHARED_ONLY)
        set(_library_type SHARED)
        set(_target_scope PUBLIC)
    else()
        # Use default (STATIC or SHARED based on BUILD_SHARED_LIBS)
        set(_library_type)
        set(_target_scope PUBLIC)
    endif()

    # Create library target
    if(MODULE_HEADER_ONLY)
        add_library(${MODULE_TARGET_NAME} INTERFACE)
    else()
        if(_library_type)
            add_library(${MODULE_TARGET_NAME} ${_library_type} ${MODULE_SOURCES} ${_all_headers})
        else()
            add_library(${MODULE_TARGET_NAME} ${MODULE_SOURCES} ${_all_headers})
        endif()
    endif()

    # Create alias target for consistent namespacing
    add_library(helios::module::${MODULE_NAME} ALIAS ${MODULE_TARGET_NAME})

    # Apply standard Helios configurations (only for non-INTERFACE libraries)
    if(NOT MODULE_HEADER_ONLY)
        # C++ standard
        helios_target_set_cxx_standard(${MODULE_TARGET_NAME} STANDARD 23)

        # Platform settings
        helios_target_set_platform(${MODULE_TARGET_NAME})

        # Shared library platform settings (for export macros, etc.)
        if(MODULE_SHARED_ONLY OR (NOT MODULE_STATIC_ONLY AND BUILD_SHARED_LIBS))
            helios_target_set_shared_lib_platform(${MODULE_TARGET_NAME})
        endif()

        # Optimization settings
        helios_target_set_optimization(${MODULE_TARGET_NAME})

        # Warning settings
        helios_target_set_warnings(${MODULE_TARGET_NAME})

        # Output directories
        helios_target_set_output_dirs(${MODULE_TARGET_NAME})

        # IDE folder organization
        helios_target_set_folder(${MODULE_TARGET_NAME} "${MODULE_FOLDER}")

        # Output name override
        if(MODULE_OUTPUT_NAME)
            set_target_properties(${MODULE_TARGET_NAME} PROPERTIES
                OUTPUT_NAME ${MODULE_OUTPUT_NAME}
            )
        endif()

        # Link-Time Optimization (LTO)
        if(HELIOS_ENABLE_LTO)
            helios_target_enable_lto(${MODULE_TARGET_NAME})
        endif()

        # Precompiled Header
        if(MODULE_PCH)
            if(NOT IS_ABSOLUTE ${MODULE_PCH})
                set(MODULE_PCH "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_PCH}")
            endif()
            if(EXISTS ${MODULE_PCH})
                helios_target_add_pch(${MODULE_TARGET_NAME} ${MODULE_PCH})
            else()
                message(WARNING "helios_add_module(${MODULE_NAME}): PCH file not found: ${MODULE_PCH}")
            endif()
        endif()

        # Unity build
        if(HELIOS_ENABLE_UNITY_BUILD)
            helios_target_enable_unity_build(${MODULE_TARGET_NAME}
                BATCH_SIZE ${CMAKE_UNITY_BUILD_BATCH_SIZE}
            )

            # Exclude PCH files from unity build if present
            if(MODULE_PCH)
                get_filename_component(_pch_name ${MODULE_PCH} NAME)
                get_filename_component(_pch_base ${MODULE_PCH} NAME_WE)
                set_source_files_properties(
                    ${MODULE_PCH}
                    "src/${_pch_base}.cpp"
                    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON
                )
            endif()
        endif()
    endif()

    # Dependencies - always link to core
    target_link_libraries(${MODULE_TARGET_NAME} ${_target_scope} helios::core)

    # Link module dependencies
    if(MODULE_DEPENDENCIES)
        target_link_libraries(${MODULE_TARGET_NAME} ${_target_scope} ${MODULE_DEPENDENCIES})
    endif()

    if(MODULE_PUBLIC_DEPENDENCIES)
        target_link_libraries(${MODULE_TARGET_NAME} ${_target_scope} ${MODULE_PUBLIC_DEPENDENCIES})
    endif()

    if(MODULE_PRIVATE_DEPENDENCIES)
        target_link_libraries(${MODULE_TARGET_NAME} PRIVATE ${MODULE_PRIVATE_DEPENDENCIES})
    endif()

    # Include directories
    target_include_directories(${MODULE_TARGET_NAME}
        ${_target_scope}
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    # Add src directory as private include (only for non-INTERFACE)
    if(NOT MODULE_HEADER_ONLY)
        target_include_directories(${MODULE_TARGET_NAME}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
    endif()

    # Additional include directories
    if(MODULE_INCLUDE_DIRECTORIES)
        target_include_directories(${MODULE_TARGET_NAME} ${_target_scope} ${MODULE_INCLUDE_DIRECTORIES})
    endif()

    # Third-party includes as SYSTEM to suppress warnings
    if(PROJECT_SOURCE_DIR)
        target_include_directories(${MODULE_TARGET_NAME}
            SYSTEM ${_target_scope}
                ${PROJECT_SOURCE_DIR}/third-party
        )
    endif()

    # Compile definitions
    if(MODULE_COMPILE_DEFINITIONS)
        target_compile_definitions(${MODULE_TARGET_NAME} ${_target_scope} ${MODULE_COMPILE_DEFINITIONS})
    endif()

    # Compile options
    if(MODULE_COMPILE_OPTIONS)
        target_compile_options(${MODULE_TARGET_NAME} ${_target_scope} ${MODULE_COMPILE_OPTIONS})
    endif()

    # Handle dynamic library copying for shared libraries
    if(NOT MODULE_HEADER_ONLY)
        get_target_property(_target_type ${MODULE_TARGET_NAME} TYPE)
        if(_target_type STREQUAL "SHARED_LIBRARY")
            # Automatically copy the shared library to dependents' output directories
            # This will be handled by targets that link to this module
            set_target_properties(${MODULE_TARGET_NAME} PROPERTIES
                HELIOS_IS_SHARED_MODULE TRUE
            )
        endif()
    endif()

    # Store module metadata as target properties
    set_target_properties(${MODULE_TARGET_NAME} PROPERTIES
        HELIOS_MODULE_NAME "${MODULE_NAME}"
        HELIOS_MODULE_VERSION "${MODULE_VERSION}"
    )

    # Add to global list of modules
    set_property(GLOBAL APPEND PROPERTY HELIOS_MODULES ${MODULE_TARGET_NAME})

    # Status message
    message(STATUS "Helios Module: ${MODULE_NAME} v${MODULE_VERSION} (${MODULE_TARGET_NAME})")
endfunction()

# Function: helios_link_module
#
# Links a module to a target and automatically handles dynamic library copying.
# This is a convenience wrapper around target_link_libraries that also copies
# shared library dependencies to the target's output directory.
#
# Usage:
#   helios_link_module(TARGET my_target MODULES module1 module2... [VISIBILITY PUBLIC|PRIVATE|INTERFACE])
#
function(helios_link_module)
    cmake_parse_arguments(ARG "" "TARGET;VISIBILITY" "MODULES" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "helios_link_module: TARGET is required")
    endif()

    if(NOT ARG_MODULES)
        message(FATAL_ERROR "helios_link_module: MODULES is required")
    endif()

    if(NOT ARG_VISIBILITY)
        set(ARG_VISIBILITY PUBLIC)
    endif()

    # Link all modules
    target_link_libraries(${ARG_TARGET} ${ARG_VISIBILITY} ${ARG_MODULES})

    # Check if any modules are shared libraries and copy them
    foreach(_module ${ARG_MODULES})
        if(TARGET ${_module})
            get_target_property(_is_shared ${_module} HELIOS_IS_SHARED_MODULE)
            if(_is_shared)
                # Copy the shared library to the target's output directory
                helios_copy_runtime_dependencies(${ARG_TARGET} LIBRARIES ${_module})
            endif()
        endif()
    endforeach()
endfunction()

# Function: helios_print_modules
#
# Prints all registered Helios modules.
# Useful for debugging and verification.
#
function(helios_print_modules)
    get_property(_modules GLOBAL PROPERTY HELIOS_MODULES)

    message(STATUS "========== Helios Engine Modules ==========")
    if(_modules)
        list(REMOVE_DUPLICATES _modules)
        list(SORT _modules)
        foreach(_module ${_modules})
            get_target_property(_name ${_module} HELIOS_MODULE_NAME)
            get_target_property(_version ${_module} HELIOS_MODULE_VERSION)
            if(_name AND _version)
                message(STATUS "  ✓ ${_name} v${_version} (${_module})")
            else()
                message(STATUS "  ✓ ${_module}")
            endif()
        endforeach()
    else()
        message(STATUS "  No modules registered")
    endif()
    message(STATUS "===========================================")
endfunction()

# Function: helios_get_module_list
#
# Retrieves the list of all registered module targets.
#
# Usage:
#   helios_get_module_list(OUTPUT_VAR)
#
function(helios_get_module_list OUTPUT_VAR)
    get_property(_modules GLOBAL PROPERTY HELIOS_MODULES)
    if(_modules)
        list(REMOVE_DUPLICATES _modules)
        list(SORT _modules)
    endif()
    set(${OUTPUT_VAR} ${_modules} PARENT_SCOPE)
endfunction()
