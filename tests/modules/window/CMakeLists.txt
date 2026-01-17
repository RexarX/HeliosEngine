# Window Module Tests
#
# Tests for the cross-platform window management module.
# Note: Some tests may require a display server (X11/Wayland/Win32) to run.

# Unit tests for the window module
set(WINDOW_UNIT_TESTS_SOURCES
    unit/main.cpp
)

helios_add_module_test(
    MODULE_NAME window
    TYPE unit
    SOURCES ${WINDOW_UNIT_TESTS_SOURCES}
)

# Integration tests for the window module
# These tests may require a display server to run
set(WINDOW_INTEGRATION_TESTS_SOURCES
    integration/main.cpp
)

helios_add_module_test(
    MODULE_NAME window
    TYPE integration
    SOURCES ${WINDOW_INTEGRATION_TESTS_SOURCES}
)
