# Test utilities for Helios Engine

include(TargetUtils)

function(helios_add_test)
    set(options "")
    set(oneValueArgs TARGET MODULE_NAME TYPE)
    set(multiValueArgs SOURCES DEPENDENCIES)
    cmake_parse_arguments(HELIOS_TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT HELIOS_TEST_TARGET)
        message(FATAL_ERROR "helios_add_test: TARGET is required")
    endif()

    if(NOT HELIOS_TEST_MODULE_NAME)
        message(FATAL_ERROR "helios_add_test: MODULE_NAME is required")
    endif()

    if(NOT HELIOS_TEST_TYPE)
        message(FATAL_ERROR "helios_add_test: TYPE is required (unit or integration)")
    endif()

    if(NOT HELIOS_TEST_SOURCES)
        message(FATAL_ERROR "helios_add_test: SOURCES is required")
    endif()

    # Validate TEST_TYPE
    if(NOT HELIOS_TEST_TYPE STREQUAL "unit" AND NOT HELIOS_TEST_TYPE STREQUAL "integration")
        message(FATAL_ERROR "helios_add_test: TYPE must be 'unit' or 'integration'")
    endif()

    # Create test executable
    add_executable(${HELIOS_TEST_TARGET} ${HELIOS_TEST_SOURCES})

    helios_target_set_cxx_standard(${HELIOS_TEST_TARGET} STANDARD 23)
    helios_target_set_optimization(${HELIOS_TEST_TARGET})
    #helios_target_set_warnings(${HELIOS_TEST_TARGET})
    helios_target_set_output_dirs(${HELIOS_TEST_TARGET} CUSTOM_FOLDER tests)
    helios_target_set_folder(${HELIOS_TEST_TARGET} "Helios/Tests")

    # Enable LTO if requested
    if(HELIOS_ENABLE_LTO)
        helios_target_enable_lto(${HELIOS_TEST_TARGET})
    endif()

    # Link doctest
    target_link_libraries(${HELIOS_TEST_TARGET} PRIVATE
        helios::doctest
    )

    # Link additional dependencies if provided
    if(HELIOS_TEST_DEPENDENCIES)
        target_link_libraries(${HELIOS_TEST_TARGET} PRIVATE
            ${HELIOS_TEST_DEPENDENCIES}
        )
    endif()

    # Additional compile definitions to match the module PCH
    target_compile_definitions(${HELIOS_TEST_TARGET} PRIVATE
        $<$<PLATFORM_ID:Windows>:NOMINMAX>
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
        $<$<PLATFORM_ID:Windows>:UNICODE>
        $<$<PLATFORM_ID:Windows>:_UNICODE>
    )

    # Create PCH for test target using the same header as the module
    # Use the absolute path to the module's PCH header
    if(TARGET helios_${HELIOS_TEST_MODULE_NAME})
        # Construct the path to the module's PCH header
        set(_pch_path "${HELIOS_ROOT_DIR}/src/${HELIOS_TEST_MODULE_NAME}/include/helios/${HELIOS_TEST_MODULE_NAME}_pch.hpp")
        if(EXISTS ${_pch_path})
            target_precompile_headers(${HELIOS_TEST_TARGET} PRIVATE ${_pch_path})
        endif()
    endif()

    # Add to CTest
    add_test(NAME ${HELIOS_TEST_TARGET} COMMAND ${HELIOS_TEST_TARGET})

    # Set test labels for filtering
    set_tests_properties(${HELIOS_TEST_TARGET} PROPERTIES
        LABELS "${HELIOS_TEST_MODULE_NAME};${HELIOS_TEST_TYPE}"
    )

    # Register with our custom targets
    register_test_target(${HELIOS_TEST_MODULE_NAME} ${HELIOS_TEST_TYPE} ${HELIOS_TEST_TARGET})
endfunction()

# Internal function to register test with build targets
function(register_test_target MODULE_NAME TEST_TYPE TARGET_NAME)
    # Add to global targets
    add_dependencies(all_tests ${TARGET_NAME})

    if(TEST_TYPE STREQUAL "unit")
        add_dependencies(unit_tests ${TARGET_NAME})
        add_dependencies(${MODULE_NAME}_unit_tests ${TARGET_NAME})
    elseif(TEST_TYPE STREQUAL "integration")
        add_dependencies(integration_tests ${TARGET_NAME})
        add_dependencies(${MODULE_NAME}_integration_tests ${TARGET_NAME})
    endif()

    # Add to module-specific target
    add_dependencies(${MODULE_NAME}_tests ${TARGET_NAME})
endfunction()
