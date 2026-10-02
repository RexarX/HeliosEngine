set(_helios_profile_uses)
if(HELIOS_PROFILE_BUNDLE_TRACY)
  list(APPEND _helios_profile_uses tracy PUBLIC helios::lib::tracy::TracyClient)
endif()

add_library(helios_profile)

target_sources(helios_profile PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/helios/profile/backends/flamegraph.hpp
    $<$<BOOL:${HELIOS_PROFILE_BUNDLE_TRACY}>:include/helios/profile/backends/tracy.hpp>
    include/helios/profile/details/memory_dispatch.hpp
    $<$<BOOL:${HELIOS_PROFILE_BUNDLE_TRACY}>:include/helios/profile/tracy/lock.hpp>
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
)

target_sources(helios_profile PRIVATE
    src/backends/flamegraph.cpp
    $<$<BOOL:${HELIOS_PROFILE_BUNDLE_TRACY}>:src/backends/tracy.cpp>
    src/profiler.cpp
    src/zone.cpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_profile PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/profile/profile.h
  )
endif()

if(HELIOS_PROFILE_BUNDLE_TRACY)
  helios_require_dependency(tracy)
  target_link_libraries(helios_profile PUBLIC helios::lib::tracy::TracyClient)
endif()

helios_require_dependency(concurrentqueue)
target_link_libraries(helios_profile PUBLIC
    helios::lib::concurrentqueue::concurrentqueue
)

target_compile_definitions(helios_profile PRIVATE
    $<$<BOOL:${HELIOS_PROFILE_BUNDLE_TRACY}>:TRACY_ENABLE>
)

helios_apply_conventions(helios_profile)
helios_target_set_folder(helios_profile "Helios/Modules")
helios_target_set_output_dirs(helios_profile)
helios_target_precompile_headers(helios_profile src/pch.hpp)
helios_target_cpp_module_sources(helios_profile modules/helios.profile.cppm)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_profile
      SOURCES
          tests/backends/flamegraph.cpp
          $<$<BOOL:${HELIOS_PROFILE_BUNDLE_TRACY}>:tests/backends/tracy.cpp>
          $<$<BOOL:${HELIOS_PROFILE_BUNDLE_TRACY}>:tests/tracy/lock.cpp>
          tests/main.cpp
          tests/module_import.cpp
          tests/profile.cpp
          tests/zone.cpp

      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()
