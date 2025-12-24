# Tests for engine modules

message(STATUS "Configuring Module Tests...")

# Function to create module test targets dynamically
function(create_module_test_targets MODULE_NAME)
    # Create targets for this module if they don't exist
    if(NOT TARGET ${MODULE_NAME}_tests)
        add_custom_target(${MODULE_NAME}_tests)
        add_dependencies(all_tests ${MODULE_NAME}_tests)
    endif()
    if(NOT TARGET ${MODULE_NAME}_unit_tests)
        add_custom_target(${MODULE_NAME}_unit_tests)
        add_dependencies(unit_tests ${MODULE_NAME}_unit_tests)
    endif()
    if(NOT TARGET ${MODULE_NAME}_integration_tests)
        add_custom_target(${MODULE_NAME}_integration_tests)
        add_dependencies(integration_tests ${MODULE_NAME}_integration_tests)
    endif()
endfunction()

# Automatically discover and add tests for all modules
# This iterates through all directories in src/modules/ and checks if:
# 1. The module is enabled via HELIOS_BUILD_{MODULE_NAME}_MODULE
# 2. A corresponding test directory exists in tests/modules/{module_name}

# Get list of all module directories
file(GLOB MODULE_DIRS RELATIVE ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/modules/*)

foreach(MODULE_DIR ${MODULE_DIRS})
    if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/src/modules/${MODULE_DIR})
        # Convert module name to uppercase for option check
        string(TOUPPER "${MODULE_DIR}" MODULE_UPPER)

        # Check if module is enabled
        if(HELIOS_BUILD_${MODULE_UPPER}_MODULE)
            # Check if test directory exists for this module
            set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${MODULE_DIR})
            if(EXISTS ${TEST_DIR} AND IS_DIRECTORY ${TEST_DIR})
                message(STATUS "  Adding tests for module: ${MODULE_DIR}")
                # Create test targets for this module
                create_module_test_targets("module_${MODULE_DIR}")
                add_subdirectory(${MODULE_DIR})
            else()
                message(STATUS "  Module '${MODULE_DIR}' is enabled but has no tests directory")
            endif()
        endif()
    endif()
endforeach()

# Print instructions for adding module tests
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/example)
    message(STATUS "")
    message(STATUS "========================================")
    message(STATUS "To add tests for a custom module:")
    message(STATUS "")
    message(STATUS "1. Create test directory:")
    message(STATUS "   mkdir -p tests/modules/your_module/{unit,integration}")
    message(STATUS "")
    message(STATUS "2. Create CMakeLists.txt:")
    message(STATUS "   tests/modules/your_module/CMakeLists.txt")
    message(STATUS "")
    message(STATUS "3. Add your tests using helios_add_module_test():")
    message(STATUS "")
    message(STATUS "   # Unit tests")
    message(STATUS "   helios_add_module_test(")
    message(STATUS "       MODULE_NAME your_module")
    message(STATUS "       TYPE unit")
    message(STATUS "       SOURCES")
    message(STATUS "           unit/main.cpp")
    message(STATUS "           unit/your_test.cpp")
    message(STATUS "   )")
    message(STATUS "")
    message(STATUS "   # Integration tests (optional)")
    message(STATUS "   helios_add_module_test(")
    message(STATUS "       MODULE_NAME your_module")
    message(STATUS "       TYPE integration")
    message(STATUS "       SOURCES")
    message(STATUS "           integration/main.cpp")
    message(STATUS "           integration/integration_test.cpp")
    message(STATUS "   )")
    message(STATUS "")
    message(STATUS "See tests/core/CMakeLists.txt for examples")
    message(STATUS "See cmake/TestUtils.cmake for function documentation")
    message(STATUS "========================================")
    message(STATUS "")
endif()
