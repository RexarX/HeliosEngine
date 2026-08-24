helios_module(
    NAME sdl3_input
    VERSION 0.1.0
    DESCRIPTION "SDL3 backend for the input ECS contract"
    DEFAULT ON
    IMPLEMENTS input

    HEADERS
        include/helios/sdl3/input/details/cursor_cache.hpp
        include/helios/sdl3/input/details/event_handlers.hpp
        include/helios/sdl3/input/details/input_map.hpp
        include/helios/sdl3/input/details/input_state.hpp
        include/helios/sdl3/input/systems/apply_cursors.hpp
        include/helios/sdl3/input/systems/apply_raw_mouse.hpp
        include/helios/sdl3/input/systems/init.hpp
        include/helios/sdl3/input/systems/poll_gamepads.hpp
        include/helios/sdl3/input/input.hpp
        include/helios/sdl3/input/plugin.hpp

    SOURCES
        src/details/event_handlers.cpp
        src/systems/apply_cursors.cpp
        src/systems/apply_raw_mouse.cpp
        src/systems/init.cpp
        src/systems/poll_gamepads.cpp
        src/plugin.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC app
        PUBLIC ecs
        PUBLIC log
        PUBLIC window
        PUBLIC input
        PUBLIC sdl3

    OPTIONAL_DEPENDS
        PUBLIC sdl3_window

    USES
        SDL3 PRIVATE helios::lib::sdl3::sdl3

    TEST_SOURCES
        tests/available.hpp
        tests/event_handlers.cpp
        tests/input_map.cpp
        tests/main.cpp
        tests/plugin.cpp
        tests/systems.cpp
)

if(TARGET helios_sdl3_input_tests AND UNIX AND HELIOS_ENABLE_SANITIZERS AND
    HELIOS_SANITIZER_ADDRESS)
  set_property(TEST helios_sdl3_input_tests APPEND PROPERTY ENVIRONMENT
      "LSAN_OPTIONS=suppressions=${HELIOS_ROOT_DIR}/cmake/sdl3_lsan.supp:print_suppressions=0")
endif()
