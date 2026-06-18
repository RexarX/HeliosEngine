helios_module(
    NAME utils
    VERSION 0.1.0
    DESCRIPTION "Utility module providing common helpers and utilities"

    HEADERS
        include/helios/utils/common_traits.hpp
        include/helios/utils/defer.hpp
        include/helios/utils/dynamic_library.hpp
        include/helios/utils/fast_pimpl.hpp
        include/helios/utils/filesystem.hpp
        include/helios/utils/functional_adapters.hpp
        include/helios/utils/hash.hpp
        include/helios/utils/macro.hpp
        include/helios/utils/random.hpp
        include/helios/utils/string_hash.hpp
        include/helios/utils/timer.hpp
        include/helios/utils/type_info.hpp

    SOURCES
        src/dynamic_library.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC compiler
        PUBLIC platform

    TEST_SOURCES
        tests/common_traits.cpp
        tests/defer.cpp
        tests/dynamic_library.cpp
        tests/fast_pimpl.cpp
        tests/filesystem.cpp
        tests/functional_adapters.cpp
        tests/hash.cpp
        tests/macro.cpp
        tests/main.cpp
        tests/random.cpp
        tests/string_hash.cpp
        tests/timer.cpp
        tests/type_info.cpp
)
