helios_module(
    NAME platform
    VERSION 0.1.0
    DESCRIPTION "Platform module providing platform specific functionality"

    HEADERS
        include/helios/platform/platform.hpp

    TEST_SOURCES
        tests/main.cpp
        tests/platform.cpp
)
