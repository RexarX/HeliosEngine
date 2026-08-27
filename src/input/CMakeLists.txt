helios_module(
    NAME input
    VERSION 0.1.0
    DESCRIPTION "Input ECS contract (keyboard, mouse, cursor, gamepad, pen)"

    HEADERS
        include/helios/input/axis.hpp
        include/helios/input/button_input.hpp
        include/helios/input/components.hpp
        include/helios/input/gamepad.hpp
        include/helios/input/input.hpp
        include/helios/input/joystick.hpp
        include/helios/input/keyboard.hpp
        include/helios/input/messages.hpp
        include/helios/input/mouse.hpp
        include/helios/input/params.hpp
        include/helios/input/pen.hpp
        include/helios/input/plugin.hpp
        include/helios/input/resources.hpp
        include/helios/input/systems.hpp

    SOURCES
        src/plugin.cpp
        src/systems.cpp

    MODULE_SOURCES
        modules/helios.input.cppm

    PCH
        src/pch.hpp

    DEPENDS
        PUBLIC app
        PUBLIC compiler
        PUBLIC ecs
        PUBLIC log
        PUBLIC memory

    TEST_SOURCES
        tests/axis.cpp
        tests/button_input.cpp
        tests/components.cpp
        tests/gamepad.cpp
        tests/joystick.cpp
        tests/main.cpp
        tests/module_import.cpp
        tests/pen.cpp
        tests/plugin.cpp
        tests/resources.cpp
        tests/systems.cpp
)
