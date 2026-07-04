option(HELIOS_PROFILE_BUNDLE_TRACY "Bundle Tracy profiler" ON)

helios_module(
    NAME profile
    VERSION 0.1.0
    DESCRIPTION "Profiling module with composable backends"
    DEFAULT OFF

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
        concurrentqueue PUBLIC helios::lib::concurrentqueue::concurrentqueue

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
  target_link_libraries(helios_module_profile PUBLIC helios::lib::tracy::TracyClient)
  if(TARGET helios_profile_tests)
    target_link_libraries(helios_profile_tests PRIVATE helios::lib::tracy::TracyClient)
  endif()
endif()
