helios_module(
    NAME window
    VERSION 0.1.0
    DESCRIPTION "Window ECS contract module"

    HEADERS
        include/helios/window/components.hpp
        include/helios/window/messages.hpp
        include/helios/window/native_handle.hpp
        include/helios/window/params.hpp
        include/helios/window/plugin.hpp
        include/helios/window/properties.hpp
        include/helios/window/resources.hpp
        include/helios/window/schedules.hpp
        include/helios/window/window.hpp

    SOURCES
        src/plugin.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC app
        PUBLIC ecs

    TEST_SOURCES
        tests/components.cpp
        tests/main.cpp
        tests/native_handle.cpp
        tests/plugin.cpp
        tests/properties.cpp
        tests/resources.cpp
        tests/schedules.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(helios_module_window PUBLIC
      HELIOS_PLATFORM_LINUX_X11 HELIOS_PLATFORM_LINUX_WAYLAND
  )
endif()
