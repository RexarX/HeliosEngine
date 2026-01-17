cmake_minimum_required(VERSION 3.25)

# Early Conan toolchain detection (before project())
# HELIOS_USE_CONAN can be set via command line: -DHELIOS_USE_CONAN=ON/OFF
# If not set, we auto-detect based on toolchain file existence

# Check if HELIOS_USE_CONAN is explicitly set via command line
if(DEFINED CACHE{HELIOS_USE_CONAN})
    # User explicitly set HELIOS_USE_CONAN, respect their choice
    message(STATUS "HELIOS_USE_CONAN explicitly set to: ${HELIOS_USE_CONAN}")
elseif(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    # No toolchain specified, try to auto-detect Conan toolchain
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" _platform_lower)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    string(TOLOWER "${CMAKE_BUILD_TYPE}" _build_type_lower)

    # Nested structure: build/<build_type>/<platform>/conan_toolchain.cmake
    set(_CONAN_TOOLCHAIN "${CMAKE_SOURCE_DIR}/build/${_build_type_lower}/${_platform_lower}/conan_toolchain.cmake")

    if(EXISTS "${_CONAN_TOOLCHAIN}")
        set(CMAKE_TOOLCHAIN_FILE "${_CONAN_TOOLCHAIN}" CACHE FILEPATH "Conan toolchain file" FORCE)
        message(STATUS "Auto-detected Conan toolchain: ${_CONAN_TOOLCHAIN}")
        set(HELIOS_USE_CONAN TRUE CACHE BOOL "Using Conan for dependencies" FORCE)
    else()
        # No Conan toolchain found, will use system packages and CPM
        set(HELIOS_USE_CONAN FALSE CACHE BOOL "Using Conan for dependencies" FORCE)
    endif()
endif()

# Verify Conan toolchain exists if CMAKE_TOOLCHAIN_FILE is set
if(DEFINED CMAKE_TOOLCHAIN_FILE AND CMAKE_TOOLCHAIN_FILE MATCHES "conan_toolchain.cmake")
    if(EXISTS "${CMAKE_TOOLCHAIN_FILE}")
        message(STATUS "Using Conan toolchain: ${CMAKE_TOOLCHAIN_FILE}")
        # Only set HELIOS_USE_CONAN if not already explicitly set
        if(NOT DEFINED CACHE{HELIOS_USE_CONAN})
            set(HELIOS_USE_CONAN TRUE CACHE BOOL "Using Conan for dependencies" FORCE)
        endif()
    else()
        message(WARNING "CMAKE_TOOLCHAIN_FILE is set to Conan toolchain but file doesn't exist: ${CMAKE_TOOLCHAIN_FILE}")
        set(HELIOS_USE_CONAN FALSE CACHE BOOL "Using Conan for dependencies" FORCE)
    endif()
elseif(NOT DEFINED CACHE{HELIOS_USE_CONAN})
    # No Conan toolchain and HELIOS_USE_CONAN not set, default to OFF
    set(HELIOS_USE_CONAN FALSE CACHE BOOL "Using Conan for dependencies" FORCE)
endif()

project(HeliosEngine
    VERSION 0.1.0
    DESCRIPTION "ECS based data-oriented game engine framework"
    HOMEPAGE_URL "https://github.com/RexarX/HeliosEngine"
    LANGUAGES CXX
)

# Set default build type to Release if not specified
# This is placed after project() to ensure command-line arguments are preserved
# even if CMake clears the cache mid-run due to compiler changes
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
    message(STATUS "Setting build type to 'Release' as none was specified.")
endif()

# Set archiver tools for Clang to support LTO
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        # On Windows with clang-cl, use llvm-lib (MSVC-compatible) or MSVC's lib.exe
        # llvm-lib understands MSVC-style flags (/out:, /machine:, etc.)
        # llvm-ar does NOT work with MSVC-style flags
        find_program(LLVM_LIB_EXECUTABLE
            NAMES llvm-lib llvm-lib.exe
            HINTS
                "$ENV{VCINSTALLDIR}/Tools/Llvm/x64/bin"
                "$ENV{VCINSTALLDIR}/Tools/Llvm/bin"
                "C:/Program Files/LLVM/bin"
                "C:/Program Files (x86)/LLVM/bin"
        )
        find_program(MSVC_LIB_EXECUTABLE NAMES lib lib.exe)

        if(LLVM_LIB_EXECUTABLE)
            set(CMAKE_AR "${LLVM_LIB_EXECUTABLE}" CACHE FILEPATH "Archiver" FORCE)
            message(STATUS "Using LLVM archiver for clang-cl: ${LLVM_LIB_EXECUTABLE}")
        elseif(MSVC_LIB_EXECUTABLE)
            set(CMAKE_AR "${MSVC_LIB_EXECUTABLE}" CACHE FILEPATH "Archiver" FORCE)
            message(STATUS "Using MSVC archiver for clang-cl: ${MSVC_LIB_EXECUTABLE}")
        else()
            message(WARNING "No MSVC-compatible archiver found for clang-cl. Build may fail.")
            message(WARNING "Please ensure llvm-lib or lib.exe is in PATH.")
        endif()
        # No ranlib needed on Windows - set to empty to avoid issues
        set(CMAKE_RANLIB "" CACHE FILEPATH "Ranlib" FORCE)
    else()
        # On Unix-like systems with Clang, use llvm-ar and llvm-ranlib
        find_program(LLVM_AR_EXECUTABLE llvm-ar)
        find_program(LLVM_RANLIB_EXECUTABLE llvm-ranlib)
        if(LLVM_AR_EXECUTABLE)
            set(CMAKE_AR "${LLVM_AR_EXECUTABLE}" CACHE FILEPATH "Archiver" FORCE)
        endif()
        if(LLVM_RANLIB_EXECUTABLE)
            set(CMAKE_RANLIB "${LLVM_RANLIB_EXECUTABLE}" CACHE FILEPATH "Ranlib" FORCE)
        endif()
    endif()
endif()

# Only generate specific configs for multi-config generators
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "" FORCE)
endif()

# Modern CMake best practices
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

# C++23 standard as project-wide requirement
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include utilities
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers")
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(BuildDirHelper)

# Print build directory information
helios_print_build_dir_info()

# Print Conan status with clear messaging
message(STATUS "")
message(STATUS "========================================")
if(HELIOS_USE_CONAN)
    message(STATUS "Conan dependency management: ENABLED")
    helios_get_conan_toolchain_path(_CONAN_TOOLCHAIN)
    if(EXISTS "${_CONAN_TOOLCHAIN}")
        message(STATUS "  Conan toolchain: ${_CONAN_TOOLCHAIN}")
        message(STATUS "  Dependencies will be loaded from Conan")
    else()
        message(WARNING "")
        message(WARNING "Conan enabled but toolchain file not found!")
        message(WARNING "Expected location: ${_CONAN_TOOLCHAIN}")
        message(WARNING "")
        message(WARNING "To install Conan dependencies, run:")
        message(WARNING "  python scripts/install_deps.py --use-conan")
        message(WARNING "")
        message(WARNING "Falling back to system packages and CPM...")
        # Disable Conan if toolchain doesn't exist
        set(HELIOS_USE_CONAN FALSE CACHE BOOL "Using Conan for dependencies" FORCE)
    endif()
else()
    message(STATUS "Conan dependency management: DISABLED")
    message(STATUS "  Will use system packages where available")
    message(STATUS "  CPM will download missing dependencies")
    message(STATUS "")
    message(STATUS "To enable Conan, run:")
    message(STATUS "  python scripts/install_deps.py --use-conan")
    message(STATUS "  python scripts/configure.py")
endif()
message(STATUS "========================================")
message(STATUS "")

# HELIOS_FORCE_CONAN controls dependency search order
# OFF (default): System packages -> Conan -> CPM (prefer what's already installed)
# ON (--use-conan): Conan -> System packages -> CPM (prefer Conan for consistency)
option(HELIOS_FORCE_CONAN "Force Conan packages first, use system only as fallback" OFF)

if(HELIOS_FORCE_CONAN)
    message(STATUS "Forcing Conan: Conan packages will be preferred over system packages")
else()
    message(STATUS "Default mode: System packages will be checked first, Conan used for missing dependencies")
endif()

# Options with better defaults
option(HELIOS_BUILD_CORE_ONLY "Build only the core library (no modules)" OFF)
option(HELIOS_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(HELIOS_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(HELIOS_ENABLE_PROFILING "Enable profiling instrumentation" OFF)
option(HELIOS_ENABLE_UNITY_BUILD "Enable Unity/Jumbo builds" OFF)
option(HELIOS_ENABLE_LTO "Enable Link Time Optimization" ON)
option(HELIOS_ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(HELIOS_ENABLE_INSTALL "Enable installation rules" OFF)

# Unity build configuration
if(HELIOS_ENABLE_UNITY_BUILD)
    set(CMAKE_UNITY_BUILD ON)
    set(CMAKE_UNITY_BUILD_BATCH_SIZE 16 CACHE STRING "Unity build batch size")
endif()

# LTO configuration
if(HELIOS_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT HELIOS_IPO_SUPPORTED OUTPUT error)
    if(HELIOS_IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        message(STATUS "Link-Time Optimization (LTO) enabled for Release and RelWithDebInfo builds")
    else()
        message(STATUS "IPO/LTO not supported: ${error}")
        # Disable HELIOS_ENABLE_LTO so targets don't try to enable it
        set(HELIOS_ENABLE_LTO OFF)
    endif()
endif()

# Set root directory for modules to reference
set(HELIOS_ROOT_DIR ${PROJECT_SOURCE_DIR})

# Include module utilities
include(cmake/ModuleUtils.cmake)

# Include sanitizer support
include(cmake/Sanitizers.cmake)

# For CMake 4.0+ compatibility with older packages that use cmake_minimum_required < 3.5
# This must be set before including Dependencies.cmake which may download old packages
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version" FORCE)
endif()

# Dependency management (system -> vcpkg -> CPM fallback)
include(cmake/Dependencies.cmake)

# Print sanitizer configuration
helios_print_sanitizer_status()

# Third-party libraries
add_subdirectory(third-party)

# Core is always built
add_subdirectory(src/core)

# Modules (only if not core-only build)
if(NOT HELIOS_BUILD_CORE_ONLY)
    add_subdirectory(src/modules)
endif()

# Testing
if(HELIOS_BUILD_TESTS)
    enable_testing()
    include(cmake/TestUtils.cmake)
    add_subdirectory(tests)
endif()

# Examples
if(HELIOS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation rules
if(HELIOS_ENABLE_INSTALL)
    include(cmake/Install.cmake)
endif()
