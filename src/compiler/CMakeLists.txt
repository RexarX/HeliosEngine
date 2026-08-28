helios_module(
    NAME compiler
    VERSION 0.1.0
    DESCRIPTION "Compiler engine module providing compiler specific functionality"

    HEADERS
        include/helios/config.hpp
        include/helios/compiler/compiler.hpp

    MODULE_SOURCES
        modules/helios.compiler.cppm

    TEST_SOURCES
        tests/compiler.cpp
        tests/main.cpp
        tests/module_import.cpp
)
