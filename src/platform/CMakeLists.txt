helios_module(
    NAME platform
    VERSION 0.1.0
    DESCRIPTION "Platform module providing platform specific functionality"

    HEADERS
        include/helios/platform/platform.hpp

    MODULE_SOURCES
        modules/helios.platform.cppm

    DEPENDS
        PUBLIC compiler

    TEST_SOURCES
        tests/main.cpp
        tests/module_import.cpp
        tests/platform.cpp
)
