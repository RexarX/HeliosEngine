helios_module(
    NAME helios_capi
    ALIAS helios::capi
    VERSION 0.1.0
    DESCRIPTION "Shared C API linkage, error codes, and the last-error buffer"

    MODULE_SOURCES
        modules/helios.capi.cppm

    C_HEADERS
        capi/include/helios/capi/capi.h
        capi/include/helios/capi/decl.h
        capi/include/helios/capi/error.h

    C_SOURCES
        src/error.cpp

    C_TEST_SOURCES
        tests/c_smoke.c

    DEPENDS
        PUBLIC helios_platform
)
