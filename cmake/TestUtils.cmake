# Helios Engine Test Utilities
#
# Provides a unified test infrastructure that works for all modules (engine and user).
# Test targets are created globally and modules automatically integrate when they add tests.
#
# Key Features:
# - Global test targets created once, reused by all modules
# - Automatic module test target creation on demand
# - Co-located tests via TEST_SOURCES in helios_module()
# - Separate tests via helios_add_module_test()
# - Integration tests via helios_add_integration_test()
#
# Usage:
#   1. Call helios_init_test_infrastructure() once in root CMakeLists.txt
#   2. Modules use TEST_SOURCES or helios_add_module_test() to add tests
#   3. Integration tests use helios_add_integration_test()

include_guard(GLOBAL)

include(TargetUtils)
include(Sanitizers)

# ============================================================================
# Global State
# ============================================================================

# Track if infrastructure is initialized
set(HELIOS_TEST_INFRASTRUCTURE_INITIALIZED FALSE CACHE INTERNAL "Test infrastructure initialized")

# ============================================================================
# Test Infrastructure Initialization
# ============================================================================

#[[
    helios_init_test_infrastructure()

    Initializes the global test infrastructure. Creates aggregate test targets
    that all modules can register with. Should be called once in the root
    CMakeLists.txt before any tests are added.

    Creates:
        - all_tests: Build all test executables
        - unit_tests: Build all unit test executables
        - integration_tests: Build all integration test executables

    Also defines global properties for tracking tests.
]]
function(helios_init_test_infrastructure)
  if(HELIOS_TEST_INFRASTRUCTURE_INITIALIZED)
    return()
  endif()

  message(STATUS "[Test] Initializing test infrastructure...")

  # Create global aggregate targets
  if(NOT TARGET all_tests)
    add_custom_target(all_tests
            COMMENT "Build all test executables"
        )
  endif()

  if(NOT TARGET unit_tests)
    add_custom_target(unit_tests
            COMMENT "Build all unit test executables"
        )
  endif()

  if(NOT TARGET integration_tests)
    add_custom_target(integration_tests
            COMMENT "Build all integration test executables"
        )
  endif()

  # Define global properties for tracking
  define_property(GLOBAL PROPERTY HELIOS_ALL_TESTS
        BRIEF_DOCS "All test targets"
        FULL_DOCS "List of all test target names")

  define_property(GLOBAL PROPERTY HELIOS_UNIT_TESTS
        BRIEF_DOCS "Unit test targets"
        FULL_DOCS "List of unit test target names")

  define_property(GLOBAL PROPERTY HELIOS_INTEGRATION_TESTS
        BRIEF_DOCS "Integration test targets"
        FULL_DOCS "List of integration test target names")

  define_property(GLOBAL PROPERTY HELIOS_TEST_MODULES
        BRIEF_DOCS "Modules with tests"
        FULL_DOCS "List of module names that have tests")

  set(HELIOS_TEST_INFRASTRUCTURE_INITIALIZED TRUE CACHE INTERNAL "Test infrastructure initialized")

  message(STATUS "[Test] Infrastructure ready (targets: all_tests, unit_tests, integration_tests)")
endfunction()

# ============================================================================
# Module Test Target Management
# ============================================================================

#[[
    _helios_ensure_module_test_targets(<module_name>)

    Internal function to create module-specific test targets on demand.
    Creates:
        - <module>_tests: All tests for this module
        - <module>_unit_tests: Unit tests for this module
        - <module>_integration_tests: Integration tests for this module
]]
function(_helios_ensure_module_test_targets MODULE_NAME)
  if(NOT TARGET ${MODULE_NAME}_tests)
    add_custom_target(${MODULE_NAME}_tests
            COMMENT "Build all tests for ${MODULE_NAME} module"
        )
    # Track this module as having tests
    set_property(GLOBAL APPEND PROPERTY HELIOS_TEST_MODULES ${MODULE_NAME})
  endif()

  if(NOT TARGET ${MODULE_NAME}_unit_tests)
    add_custom_target(${MODULE_NAME}_unit_tests
            COMMENT "Build unit tests for ${MODULE_NAME} module"
        )
  endif()

  if(NOT TARGET ${MODULE_NAME}_integration_tests)
    add_custom_target(${MODULE_NAME}_integration_tests
            COMMENT "Build integration tests for ${MODULE_NAME} module"
        )
  endif()
endfunction()

#[[
    _helios_register_test_target(<target> <module_name> <type>)

    Internal function to register a test target with all appropriate aggregate targets.
    Arguments:
        target      - The test executable target name
        module_name - The module this test belongs to
        type        - Test type: "unit" or "integration"
]]
function(_helios_register_test_target TARGET_NAME MODULE_NAME TEST_TYPE)
  # Ensure infrastructure exists
  if(NOT HELIOS_TEST_INFRASTRUCTURE_INITIALIZED)
    helios_init_test_infrastructure()
  endif()

  # Ensure module targets exist
  _helios_ensure_module_test_targets(${MODULE_NAME})

  # Add to global all_tests
  if(TARGET all_tests)
    add_dependencies(all_tests ${TARGET_NAME})
  endif()

  # Add to type-specific global target
  if(TEST_TYPE STREQUAL "unit")
    if(TARGET unit_tests)
      add_dependencies(unit_tests ${TARGET_NAME})
    endif()
    if(TARGET ${MODULE_NAME}_unit_tests)
      add_dependencies(${MODULE_NAME}_unit_tests ${TARGET_NAME})
    endif()
    set_property(GLOBAL APPEND PROPERTY HELIOS_UNIT_TESTS ${TARGET_NAME})
  elseif(TEST_TYPE STREQUAL "integration")
    if(TARGET integration_tests)
      add_dependencies(integration_tests ${TARGET_NAME})
    endif()
    if(TARGET ${MODULE_NAME}_integration_tests)
      add_dependencies(${MODULE_NAME}_integration_tests ${TARGET_NAME})
    endif()
    set_property(GLOBAL APPEND PROPERTY HELIOS_INTEGRATION_TESTS ${TARGET_NAME})
  endif()

  # Add to module-specific target
  if(TARGET ${MODULE_NAME}_tests)
    add_dependencies(${MODULE_NAME}_tests ${TARGET_NAME})
  endif()

  # Track in global list
  set_property(GLOBAL APPEND PROPERTY HELIOS_ALL_TESTS ${TARGET_NAME})
endfunction()

# ============================================================================
# Internal Helper Functions
# ============================================================================

#[[
    _helios_configure_test_target(<target>)

    Internal function to apply standard test configurations to a target.
]]
function(_helios_configure_test_target TARGET_NAME)
  # C++ standard
  helios_target_set_cxx_standard(${TARGET_NAME} STANDARD 23)

  # Optimization
  helios_target_set_optimization(${TARGET_NAME})

  # Output directory
  helios_target_set_output_dirs(${TARGET_NAME} CUSTOM_FOLDER tests)

  # IDE folder (will be overridden if needed)
  set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Helios/Tests")

  # LTO
  if(HELIOS_ENABLE_LTO)
    helios_target_enable_lto(${TARGET_NAME})
  endif()

  # Sanitizers
  helios_target_enable_sanitizers(${TARGET_NAME})

  # Platform definitions
  target_compile_definitions(${TARGET_NAME} PRIVATE
        $<$<PLATFORM_ID:Windows>:NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE>
    )
endfunction()

#[[
    _helios_link_test_framework(<target>)

    Internal function to link the test framework to a target.
]]
function(_helios_link_test_framework TARGET_NAME)
  if(TARGET helios::doctest::doctest)
    target_link_libraries(${TARGET_NAME} PRIVATE helios::doctest::doctest)
  elseif(TARGET doctest::doctest)
    target_link_libraries(${TARGET_NAME} PRIVATE doctest::doctest)
  elseif(TARGET doctest)
    target_link_libraries(${TARGET_NAME} PRIVATE doctest)
  else()
    message(WARNING "[Test] doctest target not found for ${TARGET_NAME}. "
                        "Ensure helios_require_dependency(doctest) is called.")
  endif()
endfunction()

# ============================================================================
# Public Test Functions
# ============================================================================

#[[
    helios_add_module_test(
        MODULE_NAME <name>
        [TYPE <unit|integration>]
        SOURCES <files...>
        [DEPENDENCIES <targets...>]
        [COMPILE_DEFINITIONS <defs...>]
    )

    Creates a test executable for a module.

    Arguments:
        MODULE_NAME   - Required. Module name being tested
        TYPE          - Optional. Test type: "unit" (default) or "integration"
        SOURCES       - Required. Test source files
        DEPENDENCIES  - Optional. Additional link dependencies
        COMPILE_DEFINITIONS - Optional. Additional compile definitions

    Creates:
        - Test executable: helios_<module>_<type>
        - CTest test with labels: <module>, <type>
        - Auto-registers with global and module test targets

    Example:
        helios_add_module_test(
            MODULE_NAME core
            TYPE unit
            SOURCES
                unit/main.cpp
                unit/core_test.cpp
            DEPENDENCIES helios::module::core
        )
]]
function(helios_add_module_test)
  set(options "")
  set(oneValueArgs MODULE_NAME TYPE)
  set(multiValueArgs SOURCES DEPENDENCIES COMPILE_DEFINITIONS)
  cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Validate arguments
  if(NOT TEST_MODULE_NAME)
    message(FATAL_ERROR "helios_add_module_test: MODULE_NAME is required")
  endif()

  if(NOT TEST_SOURCES)
    message(FATAL_ERROR "helios_add_module_test: SOURCES is required")
  endif()

  # Default to unit test
  if(NOT TEST_TYPE)
    set(TEST_TYPE "unit")
  endif()

  # Validate type
  if(NOT TEST_TYPE STREQUAL "unit" AND NOT TEST_TYPE STREQUAL "integration")
    message(FATAL_ERROR "helios_add_module_test: TYPE must be 'unit' or 'integration', got '${TEST_TYPE}'")
  endif()

  # Check if tests are enabled globally
  if(NOT HELIOS_BUILD_TESTS)
    return()
  endif()

  # Check if module is enabled
  string(TOUPPER "${TEST_MODULE_NAME}" _upper_name)
  if(DEFINED HELIOS_BUILD_${_upper_name}_MODULE AND NOT HELIOS_BUILD_${_upper_name}_MODULE)
    message(STATUS "[Test] Skipping ${TEST_TYPE} tests for disabled module: ${TEST_MODULE_NAME}")
    return()
  endif()

  # Check per-module test option
  set(_module_test_option "HELIOS_MODULE_${_upper_name}_BUILD_TESTS")
  if(DEFINED ${_module_test_option} AND NOT ${_module_test_option})
    message(STATUS "[Test] Skipping ${TEST_TYPE} tests for ${TEST_MODULE_NAME} (${_module_test_option}=OFF)")
    return()
  endif()

  # Generate target name
  set(_target_name "helios_${TEST_MODULE_NAME}_${TEST_TYPE}")

  # Create test executable
  add_executable(${_target_name} ${TEST_SOURCES})

  # Configure
  _helios_configure_test_target(${_target_name})

  # Set module-specific folder
  helios_target_set_folder(${_target_name} "Helios/Tests/${TEST_MODULE_NAME}")

  # Link test framework
  _helios_link_test_framework(${_target_name})

  # Link the module being tested (if target exists)
  if(TARGET helios::module::${TEST_MODULE_NAME})
    target_link_libraries(${_target_name} PRIVATE helios::module::${TEST_MODULE_NAME})
  endif()

  # Link additional dependencies
  if(TEST_DEPENDENCIES)
    target_link_libraries(${_target_name} PRIVATE ${TEST_DEPENDENCIES})
  endif()

  # Add compile definitions
  if(TEST_COMPILE_DEFINITIONS)
    target_compile_definitions(${_target_name} PRIVATE ${TEST_COMPILE_DEFINITIONS})
  endif()

  # Try to reuse module's PCH
  set(_pch_path "${HELIOS_ROOT_DIR}/src/${TEST_MODULE_NAME}/include/helios/${TEST_MODULE_NAME}_pch.hpp")
  if(EXISTS "${_pch_path}")
    target_precompile_headers(${_target_name} PRIVATE "${_pch_path}")
  endif()

  # Register with CTest
  add_test(NAME ${_target_name} COMMAND ${_target_name})
  set_tests_properties(${_target_name} PROPERTIES
        LABELS "${TEST_MODULE_NAME};${TEST_TYPE}"
    )

  # Register with aggregate targets
  _helios_register_test_target(${_target_name} ${TEST_MODULE_NAME} ${TEST_TYPE})

  message(STATUS "[Test] Added ${TEST_TYPE} test: ${_target_name} (module: ${TEST_MODULE_NAME})")
endfunction()

#[[
    helios_add_integration_test(
        NAME <name>
        SOURCES <files...>
        [MODULES <module1> <module2> ...]
        [DEPENDENCIES <targets...>]
        [COMPILE_DEFINITIONS <defs...>]
    )

    Creates an integration test that can span multiple modules.

    Arguments:
        NAME          - Required. Unique test name
        SOURCES       - Required. Test source files
        MODULES       - Optional. Helios modules to link
        DEPENDENCIES  - Optional. Additional link dependencies
        COMPILE_DEFINITIONS - Optional. Additional compile definitions

    Creates:
        - Test executable: helios_integration_<name>
        - CTest test with labels: integration, <module1>, <module2>, ...
        - Auto-registers with integration_tests target

    Example:
        helios_add_integration_test(
            NAME ecs_workflow
            SOURCES
                integration/main.cpp
                integration/ecs_workflow_test.cpp
            MODULES core ecs window
        )
]]
function(helios_add_integration_test)
  set(options "")
  set(oneValueArgs NAME MODULE_NAME)
  set(multiValueArgs SOURCES MODULES DEPENDENCIES COMPILE_DEFINITIONS)
  cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Support MODULE_NAME for backwards compatibility
  if(NOT TEST_NAME AND TEST_MODULE_NAME)
    set(TEST_NAME "${TEST_MODULE_NAME}")
    list(APPEND TEST_MODULES ${TEST_MODULE_NAME})
  endif()

  # Validate arguments
  if(NOT TEST_NAME)
    message(FATAL_ERROR "helios_add_integration_test: NAME is required")
  endif()

  if(NOT TEST_SOURCES)
    message(FATAL_ERROR "helios_add_integration_test: SOURCES is required")
  endif()

  # Check if tests are enabled
  if(NOT HELIOS_BUILD_TESTS)
    return()
  endif()

  # Generate target name
  set(_target_name "helios_integration_${TEST_NAME}")

  # Create test executable
  add_executable(${_target_name} ${TEST_SOURCES})

  # Configure
  _helios_configure_test_target(${_target_name})

  # Set folder
  helios_target_set_folder(${_target_name} "Helios/Tests/Integration")

  # Link test framework
  _helios_link_test_framework(${_target_name})

  # Link modules and collect labels
  set(_test_labels "integration")
  if(TEST_MODULES)
    foreach(_module ${TEST_MODULES})
      if(TARGET helios::module::${_module})
        target_link_libraries(${_target_name} PRIVATE helios::module::${_module})
        list(APPEND _test_labels ${_module})
      else()
        message(WARNING "[Test] Module ${_module} not found for integration test ${TEST_NAME}")
      endif()
    endforeach()
  endif()

  # Link additional dependencies
  if(TEST_DEPENDENCIES)
    target_link_libraries(${_target_name} PRIVATE ${TEST_DEPENDENCIES})
  endif()

  # Add compile definitions
  if(TEST_COMPILE_DEFINITIONS)
    target_compile_definitions(${_target_name} PRIVATE ${TEST_COMPILE_DEFINITIONS})
  endif()

  # Register with CTest
  add_test(NAME ${_target_name} COMMAND ${_target_name})
  set_tests_properties(${_target_name} PROPERTIES
        LABELS "${_test_labels}"
    )

  # Register with aggregate targets (use first module or "integration" as module name)
  if(TEST_MODULES)
    list(GET TEST_MODULES 0 _primary_module)
  else()
    set(_primary_module "integration")
  endif()
  _helios_register_test_target(${_target_name} ${_primary_module} "integration")

  message(STATUS "[Test] Added integration test: ${_target_name}")
endfunction()

# ============================================================================
# Utility Functions
# ============================================================================

#[[
    helios_print_test_summary()

    Prints a summary of all registered tests.
]]
function(helios_print_test_summary)
  get_property(_all_tests GLOBAL PROPERTY HELIOS_ALL_TESTS)
  get_property(_unit_tests GLOBAL PROPERTY HELIOS_UNIT_TESTS)
  get_property(_integration_tests GLOBAL PROPERTY HELIOS_INTEGRATION_TESTS)
  get_property(_test_modules GLOBAL PROPERTY HELIOS_TEST_MODULES)

  message(STATUS "")
  message(STATUS "============ Helios Test Summary ============")

  if(_all_tests)
    list(LENGTH _all_tests _total_count)
    message(STATUS "Total tests: ${_total_count}")
  else()
    message(STATUS "Total tests: 0")
  endif()

  if(_unit_tests)
    list(LENGTH _unit_tests _unit_count)
    message(STATUS "  Unit tests: ${_unit_count}")
  endif()

  if(_integration_tests)
    list(LENGTH _integration_tests _integration_count)
    message(STATUS "  Integration tests: ${_integration_count}")
  endif()

  if(_test_modules)
    list(REMOVE_DUPLICATES _test_modules)
    list(SORT _test_modules)
    message(STATUS "")
    message(STATUS "Modules with tests:")
    foreach(_module ${_test_modules})
      message(STATUS "  • ${_module}")
    endforeach()
  endif()

  message(STATUS "=============================================")
  message(STATUS "")
endfunction()

#[[
    helios_print_test_commands()

    Prints helpful CTest commands.
]]
function(helios_print_test_commands)
  message(STATUS "")
  message(STATUS "========== Test Commands ==========")
  message(STATUS "")
  message(STATUS "Run all tests:")
  message(STATUS "  ctest")
  message(STATUS "  ctest --parallel \$(nproc)")
  message(STATUS "")
  message(STATUS "Run by type:")
  message(STATUS "  ctest -L unit")
  message(STATUS "  ctest -L integration")
  message(STATUS "")
  message(STATUS "Run by module:")
  message(STATUS "  ctest -L core")
  message(STATUS "  ctest -L window")
  message(STATUS "")
  message(STATUS "Build test targets:")
  message(STATUS "  cmake --build . --target all_tests")
  message(STATUS "  cmake --build . --target unit_tests")
  message(STATUS "  cmake --build . --target core_tests")
  message(STATUS "  cmake --build . --target core_unit_tests")
  message(STATUS "")
  message(STATUS "Useful ctest options:")
  message(STATUS "  --output-on-failure")
  message(STATUS "  --verbose")
  message(STATUS "  --stop-on-failure")
  message(STATUS "===================================")
  message(STATUS "")
endfunction()

#[[
    helios_get_all_test_targets(<output_var>)

    Gets a list of all test target names.
]]
function(helios_get_all_test_targets OUTPUT_VAR)
  get_property(_tests GLOBAL PROPERTY HELIOS_ALL_TESTS)
  set(${OUTPUT_VAR} "${_tests}" PARENT_SCOPE)
endfunction()

#[[
    helios_get_module_test_targets(<module_name> <output_var>)

    Gets a list of test targets for a specific module.
]]
function(helios_get_module_test_targets MODULE_NAME OUTPUT_VAR)
  get_property(_all_tests GLOBAL PROPERTY HELIOS_ALL_TESTS)
  set(_module_tests "")

  foreach(_test ${_all_tests})
    if(_test MATCHES "^helios_${MODULE_NAME}_")
      list(APPEND _module_tests ${_test})
    endif()
  endforeach()

  set(${OUTPUT_VAR} "${_module_tests}" PARENT_SCOPE)
endfunction()
