# Installation configuration for Helios Engine
# Dynamically handles all built modules

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# ============================================================================
# Helper Functions
# ============================================================================

# Check if a target is from CPM (downloaded dependency)
function(_is_cpm_target target result_var)
    if(NOT TARGET ${target})
        set(${result_var} FALSE PARENT_SCOPE)
        return()
    endif()

    # Get the source directory of the target
    get_target_property(_source_dir ${target} SOURCE_DIR)
    if(_source_dir)
        # Check if it's in the CPM cache
        if(_source_dir MATCHES "\.cpm_cache" OR _source_dir MATCHES "_deps")
            set(${result_var} TRUE PARENT_SCOPE)
            return()
        endif()
    endif()

    set(${result_var} FALSE PARENT_SCOPE)
endfunction()

# ============================================================================
# Install Built Targets
# ============================================================================

set(_HELIOS_INSTALL_TARGETS "")
set(_HELIOS_INSTALL_HEADERS "")

# Core module (always present)
if(TARGET helios_core)
    list(APPEND _HELIOS_INSTALL_TARGETS helios_core)
    list(APPEND _HELIOS_INSTALL_HEADERS "${PROJECT_SOURCE_DIR}/src/core/include/helios")
endif()

# Dynamically detect and install all modules
if(NOT HELIOS_BUILD_CORE_ONLY)
    # Get all registered modules from ModuleUtils
    get_property(_all_modules GLOBAL PROPERTY HELIOS_MODULES)

    if(_all_modules)
        foreach(_module_target ${_all_modules})
            # Add module target for installation
            if(TARGET ${_module_target})
                list(APPEND _HELIOS_INSTALL_TARGETS ${_module_target})

                # Get module name from target property
                get_target_property(_module_name ${_module_target} HELIOS_MODULE_NAME)

                if(_module_name)
                    # Add module headers (assuming standard structure)
                    set(_module_header_dir "${PROJECT_SOURCE_DIR}/src/modules/${_module_name}/include/helios")
                    if(EXISTS "${_module_header_dir}")
                        list(APPEND _HELIOS_INSTALL_HEADERS "${_module_header_dir}")
                    endif()
                endif()
            endif()
        endforeach()
    endif()
endif()

# Third-party header-only libraries
if(TARGET ctti)
    list(APPEND _HELIOS_INSTALL_TARGETS ctti)
endif()

if(TARGET concurrentqueue)
    list(APPEND _HELIOS_INSTALL_TARGETS concurrentqueue)
endif()

if(TARGET stb)
    list(APPEND _HELIOS_INSTALL_TARGETS stb)
endif()

# Filter out CPM targets - they shouldn't be installed
set(_FILTERED_INSTALL_TARGETS "")
foreach(_target ${_HELIOS_INSTALL_TARGETS})
    _is_cpm_target(${_target} _is_cpm)
    if(NOT _is_cpm)
        list(APPEND _FILTERED_INSTALL_TARGETS ${_target})
    else()
        message(STATUS "Excluding CPM target from installation: ${_target}")
    endif()
endforeach()

# Install all built targets (excluding CPM dependencies)
if(_FILTERED_INSTALL_TARGETS)
    install(TARGETS ${_FILTERED_INSTALL_TARGETS}
        EXPORT HeliosEngineTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

# ============================================================================
# Install Headers
# ============================================================================

# Install module headers
foreach(_header_dir ${_HELIOS_INSTALL_HEADERS})
    if(EXISTS "${_header_dir}")
        install(DIRECTORY "${_header_dir}"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.inl"
            PATTERN "*.h"
        )
    endif()
endforeach()

# Install third-party headers
if(EXISTS "${PROJECT_SOURCE_DIR}/third-party/ctti")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/third-party/ctti"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/third-party/concurrentqueue")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/third-party/concurrentqueue"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
endif()

if(EXISTS "${PROJECT_SOURCE_DIR}/third-party/stb")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/third-party/stb"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
    )
endif()

# ============================================================================
# Export Targets
# ============================================================================

install(EXPORT HeliosEngineTargets
    FILE HeliosEngineTargets.cmake
    NAMESPACE helios::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HeliosEngine
)

# ============================================================================
# Create Package Config Files
# ============================================================================

# Check if config template exists
set(_CONFIG_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/HeliosEngineConfig.cmake.in")
if(NOT EXISTS "${_CONFIG_TEMPLATE}")
    message(WARNING "HeliosEngineConfig.cmake.in not found, skipping config file generation")
    return()
endif()

# Configure package config file
configure_package_config_file(
    "${_CONFIG_TEMPLATE}"
    "${CMAKE_CURRENT_BINARY_DIR}/HeliosEngineConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HeliosEngine
    PATH_VARS
        CMAKE_INSTALL_INCLUDEDIR
        CMAKE_INSTALL_LIBDIR
)

# Create version file
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/HeliosEngineConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# ============================================================================
# Install Config Files
# ============================================================================

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/HeliosEngineConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/HeliosEngineConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HeliosEngine
)

# ============================================================================
# Install CMake Helper Modules (optional)
# ============================================================================

# Install helper modules that might be useful for consumers
set(_CMAKE_HELPERS
    "${CMAKE_CURRENT_LIST_DIR}/TargetUtils.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/SimdUtils.cmake"
    "${CMAKE_CURRENT_LIST_DIR}/ModuleUtils.cmake"
)

foreach(_helper ${_CMAKE_HELPERS})
    if(EXISTS "${_helper}")
        install(FILES "${_helper}"
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/HeliosEngine/Modules
        )
    endif()
endforeach()

# ============================================================================
# Install Documentation
# ============================================================================

# Install license
if(EXISTS "${PROJECT_SOURCE_DIR}/LICENSE")
    install(FILES "${PROJECT_SOURCE_DIR}/LICENSE"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/HeliosEngine
    )
endif()

# Install README
if(EXISTS "${PROJECT_SOURCE_DIR}/README.md")
    install(FILES "${PROJECT_SOURCE_DIR}/README.md"
        DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/HeliosEngine
    )
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "")
message(STATUS "========== Helios Engine Installation Configuration ==========")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Build mode: ${HELIOS_BUILD_CORE_ONLY}")
if(HELIOS_BUILD_CORE_ONLY)
    message(STATUS "Installing: Core only")
else()
    message(STATUS "Installing: Core + Modules")
endif()
message(STATUS "Install targets: ${_FILTERED_INSTALL_TARGETS}")
if(_HELIOS_INSTALL_TARGETS)
    list(LENGTH _HELIOS_INSTALL_TARGETS _total_targets)
    list(LENGTH _FILTERED_INSTALL_TARGETS _filtered_targets)
    math(EXPR _excluded_targets "${_total_targets} - ${_filtered_targets}")
    if(_excluded_targets GREATER 0)
        message(STATUS "Excluded CPM targets: ${_excluded_targets}")
    endif()
endif()
message(STATUS "Binary directory: ${CMAKE_INSTALL_FULL_BINDIR}")
message(STATUS "Library directory: ${CMAKE_INSTALL_FULL_LIBDIR}")
message(STATUS "Include directory: ${CMAKE_INSTALL_FULL_INCLUDEDIR}")
message(STATUS "CMake config directory: ${CMAKE_INSTALL_FULL_LIBDIR}/cmake/HeliosEngine")
message(STATUS "===============================================================")
message(STATUS "")
