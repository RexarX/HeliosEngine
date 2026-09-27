helios_module(
    NAME helios_window
    ALIAS helios::window
    VERSION 0.1.0
    DESCRIPTION "Window ECS contract module"

    HEADERS
        include/helios/window/clipboard.hpp
        include/helios/window/ids.hpp
        include/helios/window/monitor.hpp
        include/helios/window/native_handle.hpp
        include/helios/window/params.hpp
        include/helios/window/plugin.hpp
        include/helios/window/properties.hpp
        include/helios/window/schedules.hpp
        include/helios/window/settings.hpp
        include/helios/window/window.hpp

    SOURCES
        src/plugin.cpp

    MODULE_SOURCES
        modules/helios.window.cppm

    PCH src/pch.hpp

    TEST_SOURCES
        tests/main.cpp
        tests/module_import.cpp
        tests/native_handle.cpp
        tests/params.cpp
        tests/plugin.cpp
        tests/properties.cpp
        tests/schedules.cpp
        tests/settings.cpp
        tests/systems/sync_frame_limiter.cpp

    C_HEADERS
        capi/include/helios/window/window.h

    C_TEST_SOURCES
        capi/tests/c_smoke.c

    DEPENDS
        PUBLIC
            helios_app
            $<$<BOOL:${HELIOS_BUILD_C_API}>:helios_capi>
            helios_compiler
            helios_ecs
            helios_memory
            helios_platform
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(helios_window PUBLIC
      HELIOS_PLATFORM_LINUX_X11 HELIOS_PLATFORM_LINUX_WAYLAND
  )
endif()
