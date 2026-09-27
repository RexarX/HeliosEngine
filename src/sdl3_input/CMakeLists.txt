helios_module(
    NAME helios_sdl3_input
    ALIAS helios::sdl3_input
    VERSION 0.1.0
    DESCRIPTION "SDL3 backend for the helios_input ECS contract"
    DEFAULT ON
    IMPLEMENTS input

    HEADERS
        include/helios/sdl3/input/systems/apply_cursors.hpp
        include/helios/sdl3/input/systems/apply_raw_mouse.hpp
        include/helios/sdl3/input/systems/init.hpp
        include/helios/sdl3/input/systems/poll_gamepads.hpp
        include/helios/sdl3/input/systems/poll_sensors.hpp
        include/helios/sdl3/input/cursor_cache.hpp
        include/helios/sdl3/input/event_handlers.hpp
        include/helios/sdl3/input/input.hpp
        include/helios/sdl3/input/plugin.hpp
        include/helios/sdl3/input/state.hpp
        src/details/input_map.hpp

    SOURCES
        src/systems/apply_cursors.cpp
        src/systems/apply_raw_mouse.cpp
        src/systems/init.cpp
        src/systems/poll_gamepads.cpp
        src/systems/poll_sensors.cpp
        src/event_handlers.cpp
        src/plugin.cpp

    MODULE_SOURCES
        modules/helios.sdl3.input.cppm

    PCH src/pch.hpp

    TEST_SOURCES
        tests/systems/apply_cursors.cpp
        tests/systems/apply_raw_mouse.cpp
        tests/systems/init.cpp
        tests/systems/poll_gamepads.cpp
        tests/systems/poll_sensors.cpp
        tests/available.hpp
        tests/event_handlers.cpp
        tests/input_map.cpp
        tests/main.cpp
        tests/module_import.cpp
        tests/plugin.cpp

    C_HEADERS
        capi/include/helios/sdl3/input/input.h

    C_TEST_SOURCES
        capi/tests/c_smoke.c

    DEPENDS
        PUBLIC
            helios_app
            $<$<BOOL:${HELIOS_BUILD_C_API}>:capi>
            helios_compiler
            helios_ecs
            helios_log
            helios_window
            helios_input
            helios_platform
            helios_sdl3

    OPTIONAL_DEPENDS
        PUBLIC helios_sdl3_window

    USES
        SDL3 PRIVATE helios::lib::sdl3::sdl3
)

if(TARGET helios_sdl3_input_tests AND UNIX AND HELIOS_ENABLE_SANITIZERS AND
    HELIOS_SANITIZER_ADDRESS)
  set_property(TEST helios_sdl3_input_tests APPEND PROPERTY ENVIRONMENT
      "LSAN_OPTIONS=suppressions=${HELIOS_ROOT_DIR}/cmake/sdl3_lsan.supp:print_suppressions=0")
endif()
