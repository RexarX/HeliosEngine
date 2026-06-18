helios_module(
    NAME compiler
    VERSION 0.1.0
    DESCRIPTION "Compiler engine module providing compiler specific functionality"

    HEADERS
        include/helios/compiler/compiler.hpp

    TEST_SOURCES
        tests/compiler.cpp
        tests/main.cpp
)
