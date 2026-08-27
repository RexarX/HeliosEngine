helios_module(
    NAME window
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

    DEPENDS
        PUBLIC app
        PUBLIC compiler
        PUBLIC ecs
        PUBLIC memory

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
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(helios_module_window PUBLIC
      HELIOS_PLATFORM_LINUX_X11 HELIOS_PLATFORM_LINUX_WAYLAND
  )
endif()
