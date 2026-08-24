helios_module(
    NAME glfw
    VERSION 0.1.0
    DESCRIPTION "GLFW backend for the window ECS contract"
    DEFAULT OFF
    IMPLEMENTS window input

    HEADERS
        include/helios/glfw/details/glfw_state.hpp
        include/helios/glfw/details/glfw_sync.hpp
        include/helios/glfw/details/input_map.hpp
        include/helios/glfw/details/native_handle.hpp
        include/helios/glfw/systems/apply.hpp
        include/helios/glfw/systems/create.hpp
        include/helios/glfw/systems/destroy.hpp
        include/helios/glfw/systems/init.hpp
        include/helios/glfw/systems/input.hpp
        include/helios/glfw/systems/poll.hpp
        include/helios/glfw/systems/shutdown.hpp
        include/helios/glfw/glfw.hpp
        include/helios/glfw/plugin.hpp

    SOURCES
        src/details/glfw_sync.cpp
        src/systems/apply.cpp
        src/systems/create.cpp
        src/systems/destroy.cpp
        src/systems/init.cpp
        src/systems/input.cpp
        src/systems/poll.cpp
        src/systems/shutdown.cpp
        src/callbacks.cpp
        src/native_handle.cpp
        src/plugin.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC app
        PUBLIC ecs
        PUBLIC memory
        PUBLIC log
        PUBLIC window

    OPTIONAL_DEPENDS
        PUBLIC input

    USES
        glfw PRIVATE helios::lib::glfw::glfw

    TEST_SOURCES
        tests/glfw_sync.cpp
        tests/input_map.cpp
        tests/main.cpp
        tests/native_handle.cpp
        tests/native_windows.cpp
        tests/plugin.cpp
        tests/systems.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  include(CMakeDependentOption)
  cmake_dependent_option(
      HELIOS_GLFW_FORCE_WAYLAND
      "Force Wayland backend (disable X11 fallback)"
      OFF
      "CMAKE_SYSTEM_NAME STREQUAL Linux;NOT HELIOS_GLFW_FORCE_X11"
      OFF
  )
  cmake_dependent_option(
      HELIOS_GLFW_FORCE_X11
      "Force X11 backend (disable Wayland probing)"
      OFF
      "CMAKE_SYSTEM_NAME STREQUAL Linux;NOT HELIOS_GLFW_FORCE_WAYLAND"
      OFF
  )

  if(HELIOS_GLFW_FORCE_WAYLAND AND HELIOS_GLFW_FORCE_X11)
    message(FATAL_ERROR
        "glfw: HELIOS_GLFW_FORCE_WAYLAND and HELIOS_GLFW_FORCE_X11 are mutually exclusive")
  endif()

  if(HELIOS_GLFW_FORCE_WAYLAND)
    target_compile_definitions(helios_module_glfw PUBLIC HELIOS_GLFW_FORCE_WAYLAND)
  elseif(HELIOS_GLFW_FORCE_X11)
    target_compile_definitions(helios_module_glfw PUBLIC HELIOS_GLFW_FORCE_X11)
  else()
    target_compile_definitions(helios_module_glfw PUBLIC HELIOS_GLFW_PREFER_WAYLAND)
  endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_compile_definitions(helios_module_glfw PUBLIC HELIOS_GLFW_WIN32_SUPPORT)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_compile_definitions(helios_module_glfw PUBLIC HELIOS_GLFW_COCOA_SUPPORT)
endif()
