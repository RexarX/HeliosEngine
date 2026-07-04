option(HELIOS_MEMORY_ENABLE_PROFILE "Enable profiling in the memory module if profile module is available" ON)
option(HELIOS_MEMORY_USE_MIMALLOC "Use mimalloc for memory module backing allocations" ON)
option(HELIOS_MEMORY_OVERRIDE_GLOBAL_ALLOC
       "Override global operator new/delete when linking the memory module"
       ON)

set(_helios_memory_uses)
if(HELIOS_MEMORY_USE_MIMALLOC)
  list(APPEND _helios_memory_uses mimalloc PRIVATE helios::lib::mimalloc::static)
endif()

helios_module(
    NAME memory
    VERSION 0.1.0
    DESCRIPTION "Memory module"

    HEADERS
        include/helios/memory/details/profile.hpp
        include/helios/memory/aligned_alloc.hpp
        include/helios/memory/allocator_traits.hpp
        include/helios/memory/arena_allocator.hpp
        include/helios/memory/common.hpp
        include/helios/memory/fixed_arena_allocator.hpp
        include/helios/memory/fixed_free_list_allocator.hpp
        include/helios/memory/fixed_pool_allocator.hpp
        include/helios/memory/fixed_stack_allocator.hpp
        include/helios/memory/frame_allocator.hpp
        include/helios/memory/free_list_allocator.hpp
        include/helios/memory/memory.hpp
        include/helios/memory/pool_allocator.hpp
        include/helios/memory/ref_counted.hpp
        include/helios/memory/stack_allocator.hpp
        src/details/accumulate_peak.hpp
        src/details/deferred_region_ebr.hpp

    SOURCES
        src/details/deferred_region_ebr.cpp
        src/aligned_alloc.cpp
        src/arena_allocator.cpp
        src/fixed_arena_allocator.cpp
        src/fixed_free_list_allocator.cpp
        src/fixed_pool_allocator.cpp
        src/fixed_stack_allocator.cpp
        src/free_list_allocator.cpp
        src/pool_allocator.cpp
        src/stack_allocator.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC core
        PUBLIC platform

    OPTIONAL_DEPENDS
        PUBLIC profile

    USES
        ${_helios_memory_uses}

    COMPILE_DEFINITIONS
        PUBLIC
            $<$<BOOL:HELIOS_MEMORY_ENABLE_PROFILE>:HELIOS_MEMORY_ENABLE_PROFILE>
            $<$<BOOL:HELIOS_MEMORY_USE_MIMALLOC>:HELIOS_MEMORY_USE_MIMALLOC>

    TEST_SOURCES
        tests/aligned_alloc.cpp
        tests/allocator_traits.cpp
        tests/arena_allocator.cpp
        tests/common.cpp
        tests/fixed_arena_allocator.cpp
        tests/fixed_free_list_allocator.cpp
        tests/fixed_pool_allocator.cpp
        tests/fixed_stack_allocator.cpp
        tests/frame_allocator.cpp
        tests/free_list_allocator.cpp
        tests/main.cpp
        tests/pool_allocator.cpp
        tests/ref_counted.cpp
        tests/stack_allocator.cpp
)

if(HELIOS_MEMORY_OVERRIDE_GLOBAL_ALLOC)
  add_library(helios_memory_global_alloc OBJECT src/global_alloc.cpp)
  add_library(helios::memory::global_alloc ALIAS helios_memory_global_alloc)

  helios_target_set_cxx_standard(helios_memory_global_alloc STANDARD 23)
  helios_target_set_platform(helios_memory_global_alloc)
  helios_target_set_warnings(helios_memory_global_alloc)
  helios_target_set_optimization(helios_memory_global_alloc)
  helios_target_set_folder(helios_memory_global_alloc "Modules/memory")

  target_include_directories(helios_memory_global_alloc PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_compile_definitions(helios_memory_global_alloc PRIVATE
      $<$<BOOL:HELIOS_MEMORY_USE_MIMALLOC>:HELIOS_MEMORY_USE_MIMALLOC>
      $<$<BOOL:HELIOS_MEMORY_ENABLE_PROFILE>:HELIOS_MEMORY_ENABLE_PROFILE>
      $<$<AND:$<BOOL:HELIOS_MEMORY_ENABLE_PROFILE>,$<TARGET_EXISTS:helios::module::profile>>:HELIOS_MODULE_PROFILE_AVAILABLE>
  )

  target_link_libraries(helios_memory_global_alloc PRIVATE
      $<$<AND:$<BOOL:HELIOS_MEMORY_ENABLE_PROFILE>,$<TARGET_EXISTS:helios::module::profile>>:helios::module::profile>
  )

  if(HELIOS_MEMORY_USE_MIMALLOC)
    target_link_libraries(helios_memory_global_alloc PRIVATE helios::lib::mimalloc::static)
  endif()

  # Inject global operator new/delete for non-test consumers only. Test
  # executables use the system allocator to avoid mimalloc/LTO static-init
  # issues on some platforms (notably macOS release + AppleClang).
  set_target_properties(helios_module_memory PROPERTIES
      INTERFACE_LINK_LIBRARIES_DIRECT
          "$<$<NOT:$<BOOL:$<TARGET_PROPERTY:HELIOS_TEST_TARGET>>>:helios_memory_global_alloc>"
  )
  target_compile_definitions(helios_module_memory PUBLIC
      HELIOS_MEMORY_OVERRIDE_GLOBAL_ALLOC
  )
endif()
