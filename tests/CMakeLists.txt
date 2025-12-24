# Test suite root configuration for Helios Engine

enable_testing()

# Create custom build targets for different test categories
add_custom_target(all_tests)
add_custom_target(unit_tests)
add_custom_target(integration_tests)

# Module-specific test targets
add_custom_target(core_tests)

# Module + type specific targets
add_custom_target(core_unit_tests)
add_custom_target(core_integration_tests)

# Function to create module test targets dynamically
function(create_module_test_targets MODULE_NAME)
    # Create targets for this module if they don't exist
    if(NOT TARGET ${MODULE_NAME}_tests)
        add_custom_target(${MODULE_NAME}_tests)
    endif()
    if(NOT TARGET ${MODULE_NAME}_unit_tests)
        add_custom_target(${MODULE_NAME}_unit_tests)
    endif()
    if(NOT TARGET ${MODULE_NAME}_integration_tests)
        add_custom_target(${MODULE_NAME}_integration_tests)
    endif()
endfunction()

# Help target for test commands
add_custom_target(test_help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== HeliosEngine Test Commands ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Run all tests:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ctest                           # All tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  ctest --parallel 4              # All tests in parallel"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Run by test type:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ctest -L unit                   # All unit tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  ctest -L integration            # All integration tests"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Run by module:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ctest -L core                   # All core tests"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Run specific test executable:"
    COMMAND ${CMAKE_COMMAND} -E echo "  ctest -R helios_core_unit       # Core unit tests executable"
    COMMAND ${CMAKE_COMMAND} -E echo "  ctest -R helios_runtime_integration # Runtime integration tests executable"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Build specific test targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target core_unit_tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target unit_tests"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target integration_tests"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Useful options:"
    COMMAND ${CMAKE_COMMAND} -E echo "  --verbose                       # Verbose output"
    COMMAND ${CMAKE_COMMAND} -E echo "  --output-on-failure             # Show output only on failure"
    COMMAND ${CMAKE_COMMAND} -E echo "  --stop-on-failure               # Stop on first failure"
    COMMAND ${CMAKE_COMMAND} -E echo "  --timeout <seconds>             # Set test timeout"
    COMMAND ${CMAKE_COMMAND} -E echo ""
)

# Add module test subdirectories
# Core tests (always added if tests are enabled)
create_module_test_targets(core)
add_subdirectory(core)

# Module tests (only if not building core-only)
if(NOT HELIOS_BUILD_CORE_ONLY)
    add_subdirectory(modules)
endif()
