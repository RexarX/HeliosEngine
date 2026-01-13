# Helios Engine Module Utilities
#
# Provides consistent CMake functions for declaring, registering, and
# discovering engine modules. All modules follow a standardized structure
# and configuration with automatic build option generation.
#
# Key Features:
# - Automatic HELIOS_BUILD_{NAME}_MODULE option creation
# - Module dependency resolution with build option awareness
# - Auto-discovery of modules in the modules directory
# - Simplified interface for common use cases

include_guard(GLOBAL)

include(Sanitizers)

# ============================================================================
# Module Registration and Build Options
# ============================================================================

# Function: helios_register_module
#
# Registers a module and creates a build option for it. This should be called
# before helios_add_module() to set up the build option.
#
# Usage:
#   helios_register_module(
#     NAME <name>                     # Required: Module name (e.g., "window")
#     DESCRIPTION <desc>              # Optional: Option description
#     DEFAULT <ON|OFF>                # Optional: Default value (default: ON)
#     DEPENDS <modules...>            # Optional: Required modules (will force-enable them)
#     OPTIONAL_DEPENDS <modules...>   # Optional: Optional module dependencies
#     EXTERNAL_DEPENDS <packages...>  # Optional: External package dependencies
#   )
#
# This function:
# - Creates option: HELIOS_BUILD_{NAME}_MODULE
# - Validates dependencies and enables required modules
# - Stores module metadata for later use
#
# Example:
#   helios_register_module(
#     NAME window
#     DESCRIPTION "Window management module"
#     DEFAULT ON
#     EXTERNAL_DEPENDS glfw
#   )
#
#   helios_register_module(
#     NAME renderer
#     DESCRIPTION "Rendering module"
#     DEPENDS window
#   )
#
function(helios_register_module)
    set(options "")
    set(oneValueArgs NAME DESCRIPTION DEFAULT)
    set(multiValueArgs DEPENDS OPTIONAL_DEPENDS EXTERNAL_DEPENDS)

    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT MODULE_NAME)
        message(FATAL_ERROR "helios_register_module: NAME is required")
    endif()

    # Normalize name to uppercase for option
    string(TOUPPER "${MODULE_NAME}" _upper_name)
    set(_option_name "HELIOS_BUILD_${_upper_name}_MODULE")

    # Set defaults
    if(NOT DEFINED MODULE_DEFAULT)
        set(MODULE_DEFAULT ON)
    endif()

    if(NOT MODULE_DESCRIPTION)
        set(MODULE_DESCRIPTION "Build the ${MODULE_NAME} module")
    endif()

    # Create the build option
    option(${_option_name} "${MODULE_DESCRIPTION}" ${MODULE_DEFAULT})

    # Store module metadata as cache variables for later retrieval
    set(HELIOS_MODULE_${_upper_name}_REGISTERED TRUE CACHE INTERNAL "Module ${MODULE_NAME} is registered")
    set(HELIOS_MODULE_${_upper_name}_DEPENDS "${MODULE_DEPENDS}" CACHE INTERNAL "Dependencies for ${MODULE_NAME}")
    set(HELIOS_MODULE_${_upper_name}_OPTIONAL_DEPENDS "${MODULE_OPTIONAL_DEPENDS}" CACHE INTERNAL "Optional dependencies for ${MODULE_NAME}")
    set(HELIOS_MODULE_${_upper_name}_EXTERNAL_DEPENDS "${MODULE_EXTERNAL_DEPENDS}" CACHE INTERNAL "External dependencies for ${MODULE_NAME}")

    # Add to list of registered modules
    set_property(GLOBAL APPEND PROPERTY HELIOS_REGISTERED_MODULES ${MODULE_NAME})

    message(STATUS "Registered module: ${MODULE_NAME} (${_option_name}=${${_option_name}})")
endfunction()

# Function: helios_module_enabled
#
# Checks if a module is enabled via its build option.
#
# Usage:
#   helios_module_enabled(NAME result_var)
#
# Returns TRUE if HELIOS_BUILD_{NAME}_MODULE is ON, FALSE otherwise.
#
function(helios_module_enabled NAME OUTPUT_VAR)
    string(TOUPPER "${NAME}" _upper_name)
    set(_option_name "HELIOS_BUILD_${_upper_name}_MODULE")

    if(DEFINED ${_option_name} AND ${_option_name})
        set(${OUTPUT_VAR} TRUE PARENT_SCOPE)
    else()
        set(${OUTPUT_VAR} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Function: helios_resolve_module_dependencies
#
# Resolves and enables required module dependencies.
# Should be called after all modules are registered but before building.
#
# Usage:
#   helios_resolve_module_dependencies()
#
function(helios_resolve_module_dependencies)
    get_property(_modules GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)

    if(NOT _modules)
        return()
    endif()

    # Iterate until no more changes (transitive dependency resolution)
    set(_changed TRUE)
    set(_iterations 0)
    while(_changed AND _iterations LESS 100)
        set(_changed FALSE)
        math(EXPR _iterations "${_iterations} + 1")

        foreach(_module ${_modules})
            helios_module_enabled(${_module} _is_enabled)

            if(_is_enabled)
                string(TOUPPER "${_module}" _upper_name)
                set(_deps ${HELIOS_MODULE_${_upper_name}_DEPENDS})

                foreach(_dep ${_deps})
                    helios_module_enabled(${_dep} _dep_enabled)

                    if(NOT _dep_enabled)
                        # Enable the required dependency
                        string(TOUPPER "${_dep}" _dep_upper)
                        set(_dep_option "HELIOS_BUILD_${_dep_upper}_MODULE")

                        message(STATUS "Enabling module '${_dep}' (required by '${_module}')")
                        set(${_dep_option} ON CACHE BOOL "Build the ${_dep} module" FORCE)
                        set(_changed TRUE)
                    endif()
                endforeach()
            endif()
        endforeach()
    endwhile()

    if(_iterations GREATER_EQUAL 100)
        message(WARNING "Module dependency resolution reached iteration limit. Possible circular dependency.")
    endif()
endfunction()

# Function: helios_validate_module_dependencies
#
# Validates that all required dependencies for enabled modules are available.
# Call this after dependency resolution to check external dependencies.
#
# Usage:
#   helios_validate_module_dependencies()
#
function(helios_validate_module_dependencies)
    get_property(_modules GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)

    foreach(_module ${_modules})
        helios_module_enabled(${_module} _is_enabled)

        if(_is_enabled)
            string(TOUPPER "${_module}" _upper_name)

            # Check module dependencies
            set(_deps ${HELIOS_MODULE_${_upper_name}_DEPENDS})
            foreach(_dep ${_deps})
                helios_module_enabled(${_dep} _dep_enabled)
                if(NOT _dep_enabled)
                    message(FATAL_ERROR
                        "Module '${_module}' requires module '${_dep}', but it is disabled. "
                        "Enable it with -DHELIOS_BUILD_${_upper_dep}_MODULE=ON")
                endif()
            endforeach()

            # Check external dependencies
            set(_ext_deps ${HELIOS_MODULE_${_upper_name}_EXTERNAL_DEPENDS})
            foreach(_ext_dep ${_ext_deps})
                # Check if the external dependency target exists
                # This is a soft check - the actual find will happen in the module
                if(NOT TARGET ${_ext_dep} AND NOT TARGET helios::${_ext_dep})
                    message(STATUS "Module '${_module}' requires external package '${_ext_dep}'")
                endif()
            endforeach()
        endif()
    endforeach()
endfunction()

# ============================================================================
# Module Discovery
# ============================================================================

# Function: helios_discover_modules
#
# Automatically discovers and includes modules from a directory.
# Each subdirectory with a CMakeLists.txt is treated as a potential module.
#
# Usage:
#   helios_discover_modules([DIRECTORY <path>] [EXCLUDE <names...>])
#
# Arguments:
#   DIRECTORY   - Directory to scan (default: CMAKE_CURRENT_SOURCE_DIR)
#   EXCLUDE     - List of module names to exclude from discovery
#
# The function expects each module directory to contain:
#   - CMakeLists.txt (required)
#   - Module.cmake (optional, for registration before add_subdirectory)
#
# Example directory structure:
#   modules/
#     ├── window/
#     │   ├── Module.cmake        # Optional: calls helios_register_module()
#     │   ├── CMakeLists.txt      # Calls helios_add_module()
#     │   ├── include/
#     │   └── src/
#     └── renderer/
#         ├── Module.cmake
#         ├── CMakeLists.txt
#         ├── include/
#         └── src/
#
function(helios_discover_modules)
    set(options "")
    set(oneValueArgs DIRECTORY)
    set(multiValueArgs EXCLUDE)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_DIRECTORY)
        set(ARG_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    # Get all subdirectories
    file(GLOB _children RELATIVE "${ARG_DIRECTORY}" "${ARG_DIRECTORY}/*")

    set(_discovered_modules)

    foreach(_child ${_children})
        set(_child_path "${ARG_DIRECTORY}/${_child}")

        # Skip if not a directory
        if(NOT IS_DIRECTORY "${_child_path}")
            continue()
        endif()

        # Skip if excluded
        if("${_child}" IN_LIST ARG_EXCLUDE)
            message(STATUS "Skipping excluded module: ${_child}")
            continue()
        endif()

        # Skip if no CMakeLists.txt
        if(NOT EXISTS "${_child_path}/CMakeLists.txt")
            continue()
        endif()

        # Include Module.cmake for registration if it exists
        if(EXISTS "${_child_path}/Module.cmake")
            include("${_child_path}/Module.cmake")
        endif()

        list(APPEND _discovered_modules ${_child})
    endforeach()

    # Store discovered modules for later processing
    set_property(GLOBAL PROPERTY HELIOS_DISCOVERED_MODULES ${_discovered_modules})

    message(STATUS "Discovered ${_discovered_modules} modules in ${ARG_DIRECTORY}")
endfunction()

# Function: helios_build_discovered_modules
#
# Builds all discovered modules that are enabled via their build options.
# Should be called after helios_discover_modules() and helios_resolve_module_dependencies().
#
# Usage:
#   helios_build_discovered_modules([DIRECTORY <path>])
#
function(helios_build_discovered_modules)
    set(options "")
    set(oneValueArgs DIRECTORY)
    set(multiValueArgs "")

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_DIRECTORY)
        set(ARG_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    get_property(_discovered GLOBAL PROPERTY HELIOS_DISCOVERED_MODULES)

    if(NOT _discovered)
        message(STATUS "No modules discovered to build")
        return()
    endif()

    foreach(_module ${_discovered})
        helios_module_enabled(${_module} _is_enabled)

        if(_is_enabled)
            message(STATUS "Building module: ${_module}")
            add_subdirectory("${ARG_DIRECTORY}/${_module}")
        else()
            message(STATUS "Skipping disabled module: ${_module}")
        endif()
    endforeach()
endfunction()

# ============================================================================
# Module Creation
# ============================================================================

# Function: helios_add_module
#
# Creates a standardized Helios engine module with consistent configuration.
# This is the main function for defining a module's build configuration.
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
#     MODULE_DEPENDENCIES <modules...>     # Other Helios modules (uses helios::module::name)
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
#     NO_CORE_LINK                         # Don't link to helios::core automatically
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
# - Links to helios::core by default (unless NO_CORE_LINK is specified)
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
#     HEADERS include/helios/window/window.hpp
#     PUBLIC_DEPENDENCIES glfw
#     MODULE_DEPENDENCIES input
#     PCH include/helios/window/window_pch.hpp
#   )
#
function(helios_add_module)
    # Parse arguments
    set(options
        HEADER_ONLY
        INTERFACE
        STATIC_ONLY
        SHARED_ONLY
        NO_CORE_LINK
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
        MODULE_DEPENDENCIES
        COMPILE_DEFINITIONS
        COMPILE_OPTIONS
        INCLUDE_DIRECTORIES
    )

    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Validate required arguments
    if(NOT MODULE_NAME)
        message(FATAL_ERROR "helios_add_module: NAME is required")
    endif()

    # Check if module is enabled (if registered)
    string(TOUPPER "${MODULE_NAME}" _upper_name)
    if(DEFINED HELIOS_BUILD_${_upper_name}_MODULE)
        if(NOT HELIOS_BUILD_${_upper_name}_MODULE)
            message(STATUS "Module ${MODULE_NAME} is disabled, skipping")
            return()
        endif()
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

        # Sanitizer settings
        helios_target_enable_sanitizers(${MODULE_TARGET_NAME})

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

    # Link to core (unless NO_CORE_LINK is specified)
    if(NOT MODULE_NO_CORE_LINK)
        target_link_libraries(${MODULE_TARGET_NAME} ${_target_scope} helios::core)
    endif()

    # Link module dependencies
    if(MODULE_MODULE_DEPENDENCIES)
        foreach(_mod_dep ${MODULE_MODULE_DEPENDENCIES})
            # Check if the dependency module is enabled
            helios_module_enabled(${_mod_dep} _mod_dep_enabled)
            if(_mod_dep_enabled OR TARGET helios::module::${_mod_dep})
                target_link_libraries(${MODULE_TARGET_NAME} ${_target_scope} helios::module::${_mod_dep})
            else()
                message(WARNING "helios_add_module(${MODULE_NAME}): Module dependency '${_mod_dep}' is not enabled")
            endif()
        endforeach()
    endif()

    # Link external dependencies
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

    # Add to global list of built modules
    set_property(GLOBAL APPEND PROPERTY HELIOS_MODULES ${MODULE_TARGET_NAME})

    # Status message
    message(STATUS "Helios Module: ${MODULE_NAME} v${MODULE_VERSION} (${MODULE_TARGET_NAME})")
endfunction()

# ============================================================================
# Simplified Module Definition (Convenience Wrapper)
# ============================================================================

# Function: helios_define_module
#
# A convenience wrapper that combines registration and creation for simple modules.
# Use this for modules with straightforward configuration.
#
# Usage:
#   helios_define_module(
#     NAME <name>
#     [DESCRIPTION <desc>]
#     [DEFAULT <ON|OFF>]
#     [VERSION <version>]
#     SOURCES <files...>
#     [HEADERS <files...>]
#     [DEPENDENCIES <targets...>]
#     [MODULE_DEPENDENCIES <modules...>]
#     [EXTERNAL_DEPENDS <packages...>]
#     [HEADER_ONLY]
#     [STATIC_ONLY]
#     [SHARED_ONLY]
#   )
#
# Example:
#   helios_define_module(
#     NAME input
#     DESCRIPTION "Input handling module"
#     SOURCES src/input.cpp src/keyboard.cpp src/mouse.cpp
#     HEADERS include/helios/input/input.hpp
#   )
#
function(helios_define_module)
    set(options HEADER_ONLY STATIC_ONLY SHARED_ONLY)
    set(oneValueArgs NAME DESCRIPTION DEFAULT VERSION)
    set(multiValueArgs
        SOURCES HEADERS DEPENDENCIES MODULE_DEPENDENCIES
        EXTERNAL_DEPENDS PUBLIC_DEPENDENCIES PRIVATE_DEPENDENCIES
    )

    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT MODULE_NAME)
        message(FATAL_ERROR "helios_define_module: NAME is required")
    endif()

    # Register the module first
    set(_register_args NAME ${MODULE_NAME})

    if(MODULE_DESCRIPTION)
        list(APPEND _register_args DESCRIPTION "${MODULE_DESCRIPTION}")
    endif()

    if(DEFINED MODULE_DEFAULT)
        list(APPEND _register_args DEFAULT ${MODULE_DEFAULT})
    endif()

    if(MODULE_MODULE_DEPENDENCIES)
        list(APPEND _register_args DEPENDS ${MODULE_MODULE_DEPENDENCIES})
    endif()

    if(MODULE_EXTERNAL_DEPENDS)
        list(APPEND _register_args EXTERNAL_DEPENDS ${MODULE_EXTERNAL_DEPENDS})
    endif()

    helios_register_module(${_register_args})

    # Check if enabled before creating
    helios_module_enabled(${MODULE_NAME} _is_enabled)
    if(NOT _is_enabled)
        return()
    endif()

    # Build the add_module arguments
    set(_add_args NAME ${MODULE_NAME})

    if(MODULE_VERSION)
        list(APPEND _add_args VERSION ${MODULE_VERSION})
    endif()

    if(MODULE_SOURCES)
        list(APPEND _add_args SOURCES ${MODULE_SOURCES})
    endif()

    if(MODULE_HEADERS)
        list(APPEND _add_args HEADERS ${MODULE_HEADERS})
    endif()

    if(MODULE_DEPENDENCIES)
        list(APPEND _add_args DEPENDENCIES ${MODULE_DEPENDENCIES})
    endif()

    if(MODULE_PUBLIC_DEPENDENCIES)
        list(APPEND _add_args PUBLIC_DEPENDENCIES ${MODULE_PUBLIC_DEPENDENCIES})
    endif()

    if(MODULE_PRIVATE_DEPENDENCIES)
        list(APPEND _add_args PRIVATE_DEPENDENCIES ${MODULE_PRIVATE_DEPENDENCIES})
    endif()

    if(MODULE_MODULE_DEPENDENCIES)
        list(APPEND _add_args MODULE_DEPENDENCIES ${MODULE_MODULE_DEPENDENCIES})
    endif()

    if(MODULE_HEADER_ONLY)
        list(APPEND _add_args HEADER_ONLY)
    endif()

    if(MODULE_STATIC_ONLY)
        list(APPEND _add_args STATIC_ONLY)
    endif()

    if(MODULE_SHARED_ONLY)
        list(APPEND _add_args SHARED_ONLY)
    endif()

    helios_add_module(${_add_args})
endfunction()

# ============================================================================
# Module Linking Utilities
# ============================================================================

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
    foreach(_module ${ARG_MODULES})
        # Convert module name to target name if needed
        if(TARGET helios::module::${_module})
            set(_target_name helios::module::${_module})
        elseif(TARGET helios_module_${_module})
            set(_target_name helios_module_${_module})
        elseif(TARGET ${_module})
            set(_target_name ${_module})
        else()
            # Check if module is disabled
            helios_module_enabled(${_module} _is_enabled)
            if(NOT _is_enabled)
                message(WARNING "helios_link_module: Module '${_module}' is disabled, skipping link")
                continue()
            else()
                message(FATAL_ERROR "helios_link_module: Module target '${_module}' not found")
            endif()
        endif()

        target_link_libraries(${ARG_TARGET} ${ARG_VISIBILITY} ${_target_name})

        # Check if module is a shared library and copy it
        if(TARGET ${_target_name})
            get_target_property(_is_shared ${_target_name} HELIOS_IS_SHARED_MODULE)
            if(_is_shared)
                helios_copy_runtime_dependencies(${ARG_TARGET} LIBRARIES ${_target_name})
            endif()
        endif()
    endforeach()
endfunction()

# Function: helios_link_modules_if_enabled
#
# Conditionally links modules only if they are enabled.
# Useful for optional module dependencies.
#
# Usage:
#   helios_link_modules_if_enabled(TARGET my_target MODULES mod1 mod2...)
#
function(helios_link_modules_if_enabled)
    cmake_parse_arguments(ARG "" "TARGET;VISIBILITY" "MODULES" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "helios_link_modules_if_enabled: TARGET is required")
    endif()

    if(NOT ARG_VISIBILITY)
        set(ARG_VISIBILITY PUBLIC)
    endif()

    foreach(_module ${ARG_MODULES})
        helios_module_enabled(${_module} _is_enabled)
        if(_is_enabled AND TARGET helios::module::${_module})
            target_link_libraries(${ARG_TARGET} ${ARG_VISIBILITY} helios::module::${_module})
            message(STATUS "Linked optional module '${_module}' to ${ARG_TARGET}")
        endif()
    endforeach()
endfunction()

# ============================================================================
# Module Information and Debugging
# ============================================================================

# Function: helios_print_modules
#
# Prints all registered and built Helios modules.
# Useful for debugging and verification.
#
function(helios_print_modules)
    get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
    get_property(_built GLOBAL PROPERTY HELIOS_MODULES)

    message(STATUS "")
    message(STATUS "========== Helios Engine Modules ==========")

    if(_registered)
        list(REMOVE_DUPLICATES _registered)
        list(SORT _registered)
        message(STATUS "Registered modules:")
        foreach(_module ${_registered})
            helios_module_enabled(${_module} _is_enabled)
            string(TOUPPER "${_module}" _upper_name)
            if(_is_enabled)
                message(STATUS "  ✓ ${_module} [ENABLED]")
            else()
                message(STATUS "  ✗ ${_module} [DISABLED]")
            endif()
        endforeach()
    else()
        message(STATUS "No modules registered")
    endif()

    message(STATUS "")

    if(_built)
        list(REMOVE_DUPLICATES _built)
        list(SORT _built)
        message(STATUS "Built modules:")
        foreach(_target ${_built})
            get_target_property(_name ${_target} HELIOS_MODULE_NAME)
            get_target_property(_version ${_target} HELIOS_MODULE_VERSION)
            if(_name AND _version)
                message(STATUS "  ✓ ${_name} v${_version} (${_target})")
            else()
                message(STATUS "  ✓ ${_target}")
            endif()
        endforeach()
    else()
        message(STATUS "No modules built")
    endif()

    message(STATUS "===========================================")
    message(STATUS "")
endfunction()

# Function: helios_get_module_list
#
# Retrieves the list of all built module targets.
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

# Function: helios_get_enabled_modules
#
# Retrieves the list of all enabled module names.
#
# Usage:
#   helios_get_enabled_modules(OUTPUT_VAR)
#
function(helios_get_enabled_modules OUTPUT_VAR)
    get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
    set(_enabled)

    foreach(_module ${_registered})
        helios_module_enabled(${_module} _is_enabled)
        if(_is_enabled)
            list(APPEND _enabled ${_module})
        endif()
    endforeach()

    set(${OUTPUT_VAR} ${_enabled} PARENT_SCOPE)
endfunction()

# Function: helios_print_module_build_options
#
# Prints all module build options for user reference.
#
function(helios_print_module_build_options)
    get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)

    message(STATUS "")
    message(STATUS "========== Module Build Options ==========")

    if(_registered)
        list(SORT _registered)
        foreach(_module ${_registered})
            string(TOUPPER "${_module}" _upper_name)
            set(_option "HELIOS_BUILD_${_upper_name}_MODULE")
            message(STATUS "  -D${_option}=ON|OFF  (current: ${${_option}})")
        endforeach()
    else()
        message(STATUS "No modules registered")
    endif()

    message(STATUS "==========================================")
    message(STATUS "")
endfunction()
