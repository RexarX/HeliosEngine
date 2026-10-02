set(_helios_memory_uses)
if(HELIOS_MEMORY_USE_MIMALLOC)
  list(APPEND _helios_memory_uses mimalloc
      PRIVATE helios::lib::mimalloc::static
  )
endif()

add_library(helios_memory)

target_sources(helios_memory PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
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
    include/helios/memory/temporary_storage.hpp
    include/helios/memory/temporary_storage_helpers.hpp
    include/helios/memory/treiber_stack.hpp
)

target_sources(helios_memory PRIVATE
    src/details/accumulate_peak.hpp
    src/details/deferred_region_ebr.hpp
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
    src/temporary_storage.cpp
    src/treiber_stack.cpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_memory PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/memory/memory.h
  )
endif()

helios_apply_conventions(helios_memory)
helios_target_set_folder(helios_memory "Helios/Modules")
helios_target_set_output_dirs(helios_memory)
helios_target_precompile_headers(helios_memory src/pch.hpp)
helios_target_cpp_module_sources(helios_memory modules/helios.memory.cppm)

if(HELIOS_MEMORY_USE_MIMALLOC)
  helios_require_dependency(mimalloc)
  target_link_libraries(helios_memory PRIVATE helios::lib::mimalloc::static)
endif()

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_memory
      SOURCES
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
          tests/module_import.cpp
          tests/pool_allocator.cpp
          tests/ref_counted.cpp
          tests/stack_allocator.cpp
          tests/temporary_storage.cpp
          tests/temporary_storage_helpers.cpp
          tests/treiber_stack.cpp

      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()

if(HELIOS_MEMORY_OVERRIDE_GLOBAL_ALLOC)
  add_library(helios_memory_global_alloc OBJECT src/global_alloc.cpp)
  add_library(helios::memory::global_alloc ALIAS helios_memory_global_alloc)

  helios_target_set_cxx_standard(helios_memory_global_alloc STANDARD 23)
  helios_target_set_platform(helios_memory_global_alloc)
  helios_target_set_warnings(helios_memory_global_alloc)
  helios_target_set_optimization(helios_memory_global_alloc)
  helios_target_enable_sanitizers(helios_memory_global_alloc)
  if(COMMAND helios_target_apply_linker)
    helios_target_apply_linker(helios_memory_global_alloc)
  endif()
  helios_target_set_folder(helios_memory_global_alloc "Modules/memory")

  target_include_directories(helios_memory_global_alloc PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

  target_compile_definitions(helios_memory_global_alloc PRIVATE
      $<$<BOOL:HELIOS_MEMORY_USE_MIMALLOC>:HELIOS_MEMORY_USE_MIMALLOC>
      $<$<BOOL:HELIOS_MEMORY_ENABLE_PROFILE>:HELIOS_MEMORY_ENABLE_PROFILE>
      $<$<AND:$<BOOL:HELIOS_MEMORY_ENABLE_PROFILE>,$<TARGET_EXISTS:helios::profile>>:HELIOS_PROFILE_AVAILABLE>
  )

  target_link_libraries(helios_memory_global_alloc PRIVATE
      $<$<AND:$<BOOL:HELIOS_MEMORY_ENABLE_PROFILE>,$<TARGET_EXISTS:helios::profile>>:helios::profile>
  )

  if(HELIOS_MEMORY_USE_MIMALLOC)
    target_link_libraries(helios_memory_global_alloc PRIVATE
        helios::lib::mimalloc::static
    )
  endif()

  helios_target_consume_cxx_modules(helios_memory_global_alloc)

  # Inject global operator new/delete for non-test consumers only. Test
  # executables use the system allocator to avoid mimalloc/LTO static-init
  # issues on some platforms (notably macOS release + AppleClang).
  set_target_properties(helios_memory PROPERTIES INTERFACE_LINK_LIBRARIES_DIRECT
      "$<$<NOT:$<BOOL:$<TARGET_PROPERTY:HELIOS_TEST_TARGET>>>:helios_memory_global_alloc>"
  )
  target_compile_definitions(helios_memory PUBLIC
      "$<$<COMPILE_LANGUAGE:CXX>:HELIOS_MEMORY_OVERRIDE_GLOBAL_ALLOC>"
  )
endif()
