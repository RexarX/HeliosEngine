helios_module(
    NAME container
    VERSION 0.1.0
    DESCRIPTION "Container data structures module"

    HEADERS
        include/helios/container/container.hpp
        include/helios/container/details/callable_buffer_common.hpp
        include/helios/container/details/typed_buffer_common.hpp
        include/helios/container/callable_buffer.hpp
        include/helios/container/callable_buffer_array.hpp
        include/helios/container/multi_type_map.hpp
        include/helios/container/sparse_set.hpp
        include/helios/container/static_string.hpp
        include/helios/container/typed_buffer.hpp
        include/helios/container/typed_buffer_array.hpp

    DEPENDS
        PUBLIC core
        PUBLIC utils

    USES
        FlatMap PUBLIC helios::lib::flat_map

    TEST_SOURCES
        tests/callable_buffer.cpp
        tests/callable_buffer_array.cpp
        tests/main.cpp
        tests/multi_type_map.cpp
        tests/sparse_set.cpp
        tests/static_string.cpp
        tests/typed_buffer.cpp
        tests/typed_buffer_array.cpp
)
