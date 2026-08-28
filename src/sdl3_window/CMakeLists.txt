helios_module(
    NAME sdl3_window
    VERSION 0.1.0
    DESCRIPTION "SDL3 backend for the window ECS contract"
    DEFAULT ON
    IMPLEMENTS window

    HEADERS

        include/helios/sdl3/window/systems/apply.hpp
        include/helios/sdl3/window/systems/create.hpp
        include/helios/sdl3/window/systems/destroy.hpp
        include/helios/sdl3/window/systems/init.hpp
        include/helios/sdl3/window/systems/poll.hpp
        include/helios/sdl3/window/systems/shutdown.hpp
        include/helios/sdl3/window/event_handlers.hpp
        include/helios/sdl3/window/native_handle.hpp
        include/helios/sdl3/window/plugin.hpp
        include/helios/sdl3/window/state.hpp
        include/helios/sdl3/window/sync.hpp
        include/helios/sdl3/window/window.hpp
        include/helios/sdl3/window/window_map.hpp

    SOURCES
        src/systems/apply.cpp
        src/systems/create.cpp
        src/systems/destroy.cpp
        src/systems/init.cpp
        src/systems/poll.cpp
        src/systems/shutdown.cpp
        src/event_handlers.cpp
        src/native_handle.cpp
        src/plugin.cpp
        src/sync.cpp

    MODULE_SOURCES
        modules/helios.sdl3.window.cppm

    PCH src/pch.hpp

    DEPENDS
        PUBLIC app
        PUBLIC compiler
        PUBLIC ecs
        PUBLIC memory
        PUBLIC log
        PUBLIC window
        PUBLIC sdl3

    OPTIONAL_DEPENDS
        PUBLIC input

    USES
        SDL3 PRIVATE helios::lib::sdl3::sdl3

    TEST_SOURCES
        tests/systems/apply.cpp
        tests/systems/create.cpp
        tests/systems/destroy.cpp
        tests/systems/init.cpp
        tests/systems/poll.cpp
        tests/systems/shutdown.cpp
        tests/available.hpp
        tests/event_handlers.cpp
        tests/main.cpp
        tests/module_import.cpp
        tests/native_handle.cpp
        tests/native_windows.cpp
        tests/plugin.cpp
        tests/sync.cpp
        tests/window_map.cpp
)

if(TARGET helios_sdl3_window_tests AND UNIX AND HELIOS_ENABLE_SANITIZERS AND
    HELIOS_SANITIZER_ADDRESS)
  set_property(TEST helios_sdl3_window_tests APPEND PROPERTY ENVIRONMENT
      "LSAN_OPTIONS=suppressions=${HELIOS_ROOT_DIR}/cmake/sdl3_lsan.supp:print_suppressions=0")
endif()
