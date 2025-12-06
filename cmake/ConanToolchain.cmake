# Wrapper to detect and load Conan toolchain if available
#
# This file helps CMake find and load the Conan-generated toolchain file
# when it exists, enabling seamless integration with Conan dependencies.

include_guard(GLOBAL)

# Define possible Conan toolchain locations relative to project
# Updated for nested structure: build/<build_type>/<platform>/
set(_CONAN_TOOLCHAIN_PATHS
    # Standard Conan 2.x locations in binary directory
    "${CMAKE_BINARY_DIR}/conan_toolchain.cmake"
    "${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake"

    # Nested structure: build/<build_type>/<platform>/
    "${PROJECT_SOURCE_DIR}/build/debug/linux/conan_toolchain.cmake"
    "${PROJECT_SOURCE_DIR}/build/debug/windows/conan_toolchain.cmake"
    "${PROJECT_SOURCE_DIR}/build/debug/macos/conan_toolchain.cmake"
    "${PROJECT_SOURCE_DIR}/build/release/linux/conan_toolchain.cmake"
    "${PROJECT_SOURCE_DIR}/build/release/windows/conan_toolchain.cmake"
    "${PROJECT_SOURCE_DIR}/build/release/macos/conan_toolchain.cmake"
    "${PROJECT_SOURCE_DIR}/build/relwithdebinfo/linux/conan_toolchain.cmake"
    "${PROJECT_SOURCE_DIR}/build/relwithdebinfo/windows/conan_toolchain.cmake"
    "${PROJECT_SOURCE_DIR}/build/relwithdebinfo/macos/conan_toolchain.cmake"

    # Legacy Conan 1.x locations (for compatibility)
    "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake"
)

# Try to find Conan toolchain
set(CONAN_TOOLCHAIN_FILE "")
foreach(_path ${_CONAN_TOOLCHAIN_PATHS})
    if(EXISTS "${_path}")
        set(CONAN_TOOLCHAIN_FILE "${_path}")
        break()
    endif()
endforeach()

# Load Conan toolchain if found
if(CONAN_TOOLCHAIN_FILE)
    message(STATUS "Found Conan toolchain: ${CONAN_TOOLCHAIN_FILE}")

    # Set flag that Conan is being used
    set(HELIOS_USE_CONAN TRUE CACHE BOOL "Using Conan for dependencies" FORCE)

    # Include the toolchain file
    include("${CONAN_TOOLCHAIN_FILE}")

    # For Conan 1.x compatibility
    if(CONAN_TOOLCHAIN_FILE MATCHES "conanbuildinfo.cmake")
        # Call basic setup for Conan 1.x
        if(COMMAND conan_basic_setup)
            conan_basic_setup(TARGETS)
        endif()
    endif()

    message(STATUS "Conan toolchain loaded successfully")
else()
    message(STATUS "Conan toolchain not found - will use system packages or CPM")
    set(HELIOS_USE_CONAN FALSE CACHE BOOL "Using Conan for dependencies" FORCE)
endif()

# Helper function to check if a package is from Conan
function(helios_is_conan_package package_name out_var)
    if(HELIOS_USE_CONAN)
        # Check if the package has Conan-specific properties
        if(TARGET ${package_name})
            get_target_property(_package_type ${package_name} TYPE)
            get_target_property(_package_location ${package_name} IMPORTED_LOCATION)

            # Conan packages are typically IMPORTED and have location in Conan cache
            if(_package_location AND _package_location MATCHES "conan")
                set(${out_var} TRUE PARENT_SCOPE)
                return()
            endif()
        endif()
    endif()

    set(${out_var} FALSE PARENT_SCOPE)
endfunction()

# Print Conan status
if(NOT HELIOS_USE_CONAN)
    message(STATUS "")
    message(STATUS "Conan not detected. To use Conan:")
    message(STATUS "  1. Install dependencies: python scripts/install_deps.py --use-conan")
    message(STATUS "  2. Or let CMake use system packages and CPM fallback")
    message(STATUS "")
endif()
