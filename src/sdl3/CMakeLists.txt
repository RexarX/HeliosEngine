helios_module(
    NAME sdl3
    VERSION 0.1.0
    DESCRIPTION "SDL3 process-global runtime (lifetime, event pump)"

    HEADERS
        include/helios/sdl3/details/context.hpp
        include/helios/sdl3/details/event_dispatcher.hpp
        include/helios/sdl3/details/lifetime.hpp
        include/helios/sdl3/systems/init.hpp
        include/helios/sdl3/systems/pump_events.hpp
        include/helios/sdl3/systems/shutdown.hpp
        include/helios/sdl3/plugin.hpp
        include/helios/sdl3/sdl3.hpp

    SOURCES
        src/details/lifetime.cpp
        src/systems/init.cpp
        src/systems/pump_events.cpp
        src/systems/shutdown.cpp
        src/plugin.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC app
        PUBLIC ecs
        PUBLIC log
        PUBLIC window

    USES
        SDL3 PRIVATE helios::lib::sdl3::sdl3

    TEST_SOURCES
        tests/available.hpp
        tests/event_dispatcher.cpp
        tests/lifetime.cpp
        tests/main.cpp
        tests/plugin.cpp
        tests/systems.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  include(CMakeDependentOption)
  cmake_dependent_option(
      HELIOS_SDL3_FORCE_WAYLAND
      "Force Wayland video driver (disable X11 fallback)"
      OFF
      "CMAKE_SYSTEM_NAME STREQUAL Linux;NOT HELIOS_SDL3_FORCE_X11"
      OFF
  )
  cmake_dependent_option(
      HELIOS_SDL3_FORCE_X11
      "Force X11 video driver (disable Wayland probing)"
      OFF
      "CMAKE_SYSTEM_NAME STREQUAL Linux;NOT HELIOS_SDL3_FORCE_WAYLAND"
      OFF
  )

  if(HELIOS_SDL3_FORCE_WAYLAND AND HELIOS_SDL3_FORCE_X11)
    message(FATAL_ERROR
        "sdl3: HELIOS_SDL3_FORCE_WAYLAND and HELIOS_SDL3_FORCE_X11 are mutually exclusive")
  endif()

  if(HELIOS_SDL3_FORCE_WAYLAND)
    target_compile_definitions(helios_module_sdl3 PUBLIC HELIOS_SDL3_FORCE_WAYLAND)
  elseif(HELIOS_SDL3_FORCE_X11)
    target_compile_definitions(helios_module_sdl3 PUBLIC HELIOS_SDL3_FORCE_X11)
  endif()
endif()

if(TARGET helios_sdl3_tests AND UNIX AND HELIOS_ENABLE_SANITIZERS AND
    HELIOS_SANITIZER_ADDRESS)
  set_property(TEST helios_sdl3_tests APPEND PROPERTY ENVIRONMENT
      "LSAN_OPTIONS=suppressions=${HELIOS_ROOT_DIR}/cmake/sdl3_lsan.supp:print_suppressions=0")
endif()
