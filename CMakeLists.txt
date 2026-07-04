cmake_minimum_required(VERSION 3.25)

project(HeliosEngine
    VERSION 0.1.0
    DESCRIPTION "ECS based data-oriented game engine framework"
    HOMEPAGE_URL "https://github.com/RexarX/HeliosEngine"
    LANGUAGES CXX
)

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
  message(STATUS "Setting build type to 'Release' as none was specified.")
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "" FORCE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(HELIOS_DEVELOPER_MODE "Enable developer-focused build features" OFF)
if(DEFINED DEVELOPER_MODE)
  set(HELIOS_DEVELOPER_MODE ${DEVELOPER_MODE} CACHE BOOL "Enable developer-focused build features" FORCE)
endif()

option(HELIOS_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
if(DEFINED BUILD_TESTING)
  set(HELIOS_BUILD_TESTS ${BUILD_TESTING} CACHE BOOL "Build tests" FORCE)
else()
  set(BUILD_TESTING ${HELIOS_BUILD_TESTS} CACHE BOOL "Build tests")
endif()
option(HELIOS_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(HELIOS_BUILD_DOCS "Build API documentation (Doxygen)" OFF)
option(HELIOS_ENABLE_UNITY_BUILD "Enable Unity/Jumbo builds" OFF)
option(HELIOS_ENABLE_LTO "Enable Link Time Optimization" ON)
option(HELIOS_ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(HELIOS_ENABLE_INSTALL "Enable installation rules" OFF)
option(HELIOS_BUILD_ALL_MODULES "Build every discovered module regardless of DEFAULT value" OFF)
set(HELIOS_EXTRA_MODULE_DIRS "" CACHE STRING
    "Semicolon-separated list of extra directories to scan for Helios modules")

# Include utilities
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helpers")
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(ClangArchiver)
helios_configure_clang_archiver()

if(HELIOS_ENABLE_UNITY_BUILD)
  set(CMAKE_UNITY_BUILD ON)
  set(CMAKE_UNITY_BUILD_BATCH_SIZE 16 CACHE STRING "Unity build batch size")
endif()

set(HELIOS_ROOT_DIR ${PROJECT_SOURCE_DIR})

if(HELIOS_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT HELIOS_IPO_SUPPORTED OUTPUT error)
  if(HELIOS_IPO_SUPPORTED)
    set(HELIOS_IPO_SUPPORTED "${HELIOS_IPO_SUPPORTED}" CACHE INTERNAL "IPO/LTO support result")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    message(STATUS "Link-Time Optimization (LTO) enabled for Release and RelWithDebInfo builds")
  else()
    message(STATUS "IPO/LTO not supported: ${error}")
    set(HELIOS_ENABLE_LTO OFF)
  endif()
  set(HELIOS_IPO_CACHED TRUE CACHE INTERNAL "IPO/LTO support has been checked globally")
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled for multi-config generators")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled on Linux")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo" OR CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  message(STATUS "Position-Independent Code (PIC) enabled for ${CMAKE_BUILD_TYPE} builds")
endif()

# Include module utilities
include(cmake/ModuleUtils.cmake)

# Include sanitizer support
include(cmake/Sanitizers.cmake)

# Initialize test infrastructure
if(HELIOS_BUILD_TESTS)
  include(CTest)
  include(cmake/TestUtils.cmake)
  helios_init_test_infrastructure()
endif()

# CMake 4.0+ compat
if(CMAKE_VERSION VERSION_GREATER_EQUAL "4.0")
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum CMake policy version" FORCE)
endif()

helios_print_sanitizer_status()

# Third-party libraries
add_subdirectory(third-party)

# ============================================================================
# Dependency System Initialization
# ============================================================================

include(cmake/Dependencies.cmake)

# ============================================================================
# Module Discovery and Building
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "Configuring Helios Modules...")
message(STATUS "========================================")

if(HELIOS_BUILD_EXAMPLES)
  helios_add_extra_module_dirs("${CMAKE_CURRENT_SOURCE_DIR}/examples/custom_module")
endif()

# Pass 1: Discover and register all modules (two-pass: registration scan + ALL_MODULES)
helios_discover_modules(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
helios_discover_extra_module_dirs()

# Pass 1C-E: Resolve and validate
helios_resolve_module_dependencies()
helios_validate_module_dependencies()

# Pass 2: Build enabled modules in topological order
helios_build_discovered_modules()

# Print summaries
helios_print_modules()
helios_print_module_build_options()
helios_print_dependency_summary()

# ============================================================================
# Testing
# ============================================================================

if(HELIOS_BUILD_TESTS)
  helios_print_test_commands()
endif()

# ============================================================================
# Examples
# ============================================================================

if(HELIOS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ============================================================================
# Documentation
# ============================================================================

if(HELIOS_BUILD_DOCS)
  add_subdirectory(docs)
endif()

# ============================================================================
# Installation
# ============================================================================

if(HELIOS_ENABLE_INSTALL)
  include(cmake/Install.cmake)
endif()
