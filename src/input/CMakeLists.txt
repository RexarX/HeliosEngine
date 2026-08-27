helios_module(
    NAME input
    VERSION 0.1.0
    DESCRIPTION "Input ECS contract (keyboard, mouse, cursor, gamepad, pen)"

    HEADERS
        include/helios/input/systems/clear.hpp
        include/helios/input/systems/gamepad.hpp
        include/helios/input/systems/joystick.hpp
        include/helios/input/systems/keyboard.hpp
        include/helios/input/systems/mouse.hpp
        include/helios/input/systems/pen.hpp
        include/helios/input/systems/sensor.hpp
        include/helios/input/systems/touch.hpp
        include/helios/input/axis.hpp
        include/helios/input/button_input.hpp
        include/helios/input/gamepad.hpp
        include/helios/input/ids.hpp
        include/helios/input/input.hpp
        include/helios/input/joystick.hpp
        include/helios/input/keyboard.hpp
        include/helios/input/mouse.hpp
        include/helios/input/params.hpp
        include/helios/input/pen.hpp
        include/helios/input/plugin.hpp
        include/helios/input/sensor.hpp
        include/helios/input/settings.hpp
        include/helios/input/touch.hpp

    SOURCES
        src/systems/clear.cpp
        src/systems/gamepad.cpp
        src/systems/joystick.cpp
        src/systems/keyboard.cpp
        src/systems/mouse.cpp
        src/systems/pen.cpp
        src/systems/sensor.cpp
        src/systems/touch.cpp
        src/plugin.cpp

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
        tests/systems/clear.cpp
        tests/systems/gamepad.cpp
        tests/systems/joystick.cpp
        tests/systems/keyboard.cpp
        tests/systems/mouse.cpp
        tests/systems/pen.cpp
        tests/systems/sensor.cpp
        tests/systems/touch.cpp
        tests/axis.cpp
        tests/button_input.cpp
        tests/gamepad.cpp
        tests/joystick.cpp
        tests/keyboard.cpp
        tests/main.cpp
        tests/module_import.cpp
        tests/mouse.cpp
        tests/params.cpp
        tests/pen.cpp
        tests/plugin.cpp
        tests/sensor.cpp
        tests/touch.cpp
)
