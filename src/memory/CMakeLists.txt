option(HELIOS_MEMORY_ENABLE_PROFILE "Enable profiling in the memory module if profile module is available" ON)

helios_module(
    NAME memory
    VERSION 0.1.0
    DESCRIPTION "Memory module"

    HEADERS
        include/helios/memory/common.hpp
        include/helios/memory/aligned_alloc.hpp
        include/helios/memory/allocator_traits.hpp
        include/helios/memory/arena_allocator.hpp
        include/helios/memory/details/profile.hpp
        include/helios/memory/frame_allocator.hpp
        include/helios/memory/free_list_allocator.hpp
        include/helios/memory/pool_allocator.hpp
        include/helios/memory/ref_counted.hpp
        include/helios/memory/stack_allocator.hpp

    SOURCES
        src/arena_allocator.cpp
        src/free_list_allocator.cpp
        src/pool_allocator.cpp
        src/stack_allocator.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC core

    OPTIONAL_DEPENDS
        PUBLIC profile

    COMPILE_DEFINITIONS
        PUBLIC
            $<$<BOOL:HELIOS_MEMORY_ENABLE_PROFILE>:HELIOS_MEMORY_ENABLE_PROFILE>

    TEST_SOURCES
        tests/aligned_alloc.cpp
        tests/allocator_traits.cpp
        tests/arena_allocator.cpp
        tests/common.cpp
        tests/frame_allocator.cpp
        tests/free_list_allocator.cpp
        tests/main.cpp
        tests/pool_allocator.cpp
        tests/ref_counted.cpp
        tests/stack_allocator.cpp
)
