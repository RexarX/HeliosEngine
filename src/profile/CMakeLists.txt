option(HELIOS_PROFILE_BUNDLE_TRACY "Bundle Tracy profiler" ON)
option(HELIOS_PROFILE_GLOBAL_ALLOC
       "Build optional global operator new/delete profiling hooks"
       ON)

helios_module(
    NAME profile
    VERSION 0.1.0
    DESCRIPTION "Profiling module with composable backends"

    HEADERS
        include/helios/profile/backends/flamegraph.hpp
        $<$<BOOL:HELIOS_PROFILE_BUNDLE_TRACY>:include/helios/profile/backends/tracy.hpp>
        include/helios/profile/details/memory_dispatch.hpp
        $<$<BOOL:HELIOS_PROFILE_BUNDLE_TRACY>:include/helios/profile/tracy/lock.hpp>
        include/helios/profile/backend.hpp
        include/helios/profile/common.hpp
        include/helios/profile/config.hpp
        include/helios/profile/frame.hpp
        include/helios/profile/macros.hpp
        include/helios/profile/memory.hpp
        include/helios/profile/message.hpp
        include/helios/profile/plot.hpp
        include/helios/profile/profile.hpp
        include/helios/profile/profiler.hpp
        include/helios/profile/zone.hpp

    SOURCES
        src/backends/flamegraph.cpp
        $<$<BOOL:HELIOS_PROFILE_BUNDLE_TRACY>:src/backends/tracy.cpp>
        src/profiler.cpp
        src/zone.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC compiler
        PUBLIC container
        PUBLIC core
        PUBLIC platform
        PUBLIC utils

    USES
        concurrentqueue PUBLIC helios::concurrentqueue::concurrentqueue

    COMPILE_DEFINITIONS
        PUBLIC
            $<$<BOOL:HELIOS_PROFILE_BUNDLE_TRACY>:HELIOS_PROFILE_BUNDLE_TRACY>
        PRIVATE
            $<$<BOOL:HELIOS_PROFILE_BUNDLE_TRACY>:TRACY_ENABLE>

    TEST_SOURCES
        tests/backends/flamegraph.cpp
        $<$<BOOL:HELIOS_PROFILE_BUNDLE_TRACY>:tests/backends/tracy.cpp>
        $<$<BOOL:HELIOS_PROFILE_BUNDLE_TRACY>:tests/tracy/lock.cpp>
        tests/main.cpp
        tests/profile.cpp
        tests/zone.cpp
)

if(HELIOS_PROFILE_BUNDLE_TRACY)
  helios_require_dependency(tracy)
  target_link_libraries(helios_module_profile PUBLIC helios::tracy::TracyClient)
  if(TARGET helios_profile_tests)
    target_link_libraries(helios_profile_tests PRIVATE helios::tracy::TracyClient)
  endif()
endif()

if(HELIOS_PROFILE_GLOBAL_ALLOC)
  add_library(helios_profile_global_alloc OBJECT
      src/global_alloc.cpp
  )
  add_library(helios::profile::global_alloc ALIAS helios_profile_global_alloc)

  helios_target_set_cxx_standard(helios_profile_global_alloc STANDARD 23)
  helios_target_set_platform(helios_profile_global_alloc)
  helios_target_set_warnings(helios_profile_global_alloc)
  helios_target_set_optimization(helios_profile_global_alloc)
  helios_target_set_folder(helios_profile_global_alloc "Modules/profile")

  target_link_libraries(helios_profile_global_alloc PUBLIC helios::module::profile)
  target_compile_definitions(helios_profile_global_alloc
      PUBLIC HELIOS_PROFILE_GLOBAL_ALLOC
  )
endif()

function(helios_profile_link_global_alloc TARGET_NAME)
  if(NOT TARGET ${TARGET_NAME})
    message(FATAL_ERROR "helios_profile_link_global_alloc: target '${TARGET_NAME}' not found")
  endif()
  if(NOT TARGET helios_profile_global_alloc)
    message(FATAL_ERROR
        "helios_profile_link_global_alloc: global alloc hooks are disabled "
        "(enable with -DHELIOS_PROFILE_GLOBAL_ALLOC=ON)")
  endif()
  target_link_libraries(${TARGET_NAME} PRIVATE helios_profile_global_alloc)
endfunction()
