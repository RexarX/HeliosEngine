helios_module(
    NAME helios_platform
    ALIAS helios::platform
    VERSION 0.1.0
    DESCRIPTION "Platform module providing helios_platform specific functionality"

    HEADERS
        include/helios/platform/detail/platform.h
        include/helios/platform/platform.hpp

    MODULE_SOURCES
        modules/helios.platform.cppm

    TEST_SOURCES
        tests/main.cpp
        tests/module_import.cpp
        tests/platform.cpp

    C_HEADERS
        include/helios/platform/detail/platform.h
        capi/include/helios/platform/platform.h

    C_TEST_SOURCES
        capi/tests/c_smoke.c

    DEPENDS
        PUBLIC helios_compiler
)
