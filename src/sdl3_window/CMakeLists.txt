helios_module(
    NAME sdl3_window
    VERSION 0.1.0
    DESCRIPTION "SDL3 backend for the window ECS contract"
    DEFAULT ON
    IMPLEMENTS window

    HEADERS
        include/helios/sdl3/window/details/native_state.hpp
        include/helios/sdl3/window/details/native_handle.hpp
        include/helios/sdl3/window/details/event_handlers.hpp
        include/helios/sdl3/window/details/sdl_sync.hpp
        include/helios/sdl3/window/details/window_map.hpp
        include/helios/sdl3/window/systems/apply.hpp
        include/helios/sdl3/window/systems/create.hpp
        include/helios/sdl3/window/systems/destroy.hpp
        include/helios/sdl3/window/systems/init.hpp
        include/helios/sdl3/window/systems/poll.hpp
        include/helios/sdl3/window/systems/shutdown.hpp
        include/helios/sdl3/window/window.hpp
        include/helios/sdl3/window/plugin.hpp

    SOURCES
        src/details/event_handlers.cpp
        src/details/sdl_sync.cpp
        src/details/native_handle.cpp
        src/systems/apply.cpp
        src/systems/create.cpp
        src/systems/destroy.cpp
        src/systems/init.cpp
        src/systems/poll.cpp
        src/systems/shutdown.cpp
        src/plugin.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC app
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
        tests/available.hpp
        tests/event_handlers.cpp
        tests/main.cpp
        tests/native_handle.cpp
        tests/native_windows.cpp
        tests/plugin.cpp
        tests/sdl_sync.cpp
        tests/systems.cpp
        tests/window_map.cpp
)
