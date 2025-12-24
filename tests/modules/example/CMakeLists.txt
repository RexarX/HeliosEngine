# Example Module Tests
#
# This file demonstrates how to add tests for a custom module using the
# simplified helios_add_module_test() function.

# Unit tests for the example module
set(EXAMPLE_UNIT_TESTS_SOURCES
    unit/main.cpp
    unit/example_test.cpp
)

helios_add_module_test(
    MODULE_NAME example
    TYPE unit
    SOURCES ${EXAMPLE_UNIT_TESTS_SOURCES}
)

# Integration tests for the example module
set(EXAMPLE_INTEGRATION_TESTS_SOURCES
    integration/main.cpp
    integration/example_integration_test.cpp
)

helios_add_module_test(
    MODULE_NAME example
    TYPE integration
    SOURCES ${EXAMPLE_INTEGRATION_TESTS_SOURCES}
)
