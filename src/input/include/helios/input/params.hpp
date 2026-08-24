#pragma once

#include <helios/ecs/message/params.hpp>
#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/composite_param.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/input/messages.hpp>
#include <helios/input/resources.hpp>

namespace helios::input {

/// @brief Mutable keyboard, mouse, gamepad, joystick, and pen resources.
struct State {
  ecs::Res<Keyboard> keyboard;
  ecs::Res<Mouse> mouse;
  ecs::Res<Gamepads> gamepads;
  ecs::Res<Joysticks> joysticks;
  ecs::Res<Pens> pens;
};

/// @brief Read-only keyboard, mouse, gamepad, joystick, and pen resources.
struct StateView {
  ecs::Res<const Keyboard> keyboard;
  ecs::Res<const Mouse> mouse;
  ecs::Res<const Gamepads> gamepads;
  ecs::Res<const Joysticks> joysticks;
  ecs::Res<const Pens> pens;
};

/// @brief Keyboard key and text input message readers.
struct KeyboardMessages {
  ecs::MessageReader<KeyboardInputMsg> keys;
  ecs::MessageReader<TextInputMsg> text;
};

/// @brief Mouse button, cursor, motion, and wheel message readers.
struct MouseMessages {
  ecs::MessageReader<MouseButtonInputMsg> buttons;
  ecs::MessageReader<CursorMovedMsg> cursor;
  ecs::MessageReader<MouseMotionMsg> motion;
  ecs::MessageReader<MouseWheelMsg> wheel;
};

/// @brief Gamepad connection, button, axis, and extra-device message readers.
struct GamepadMessages {
  ecs::MessageReader<GamepadConnectionMsg> connection;
  ecs::MessageReader<GamepadButtonInputMsg> buttons;
  ecs::MessageReader<GamepadAxisChangedMsg> axes;
  ecs::MessageReader<GamepadRemappedMsg> remapped;
  ecs::MessageReader<GamepadPowerChangedMsg> power;
  ecs::MessageReader<GamepadSensorUpdateMsg> sensors;
  ecs::MessageReader<GamepadTouchpadMsg> touchpad;
};

/// @brief Unmapped joystick connection, button, axis, and hat readers.
struct JoystickMessages {
  ecs::MessageReader<JoystickConnectionMsg> connection;
  ecs::MessageReader<JoystickButtonInputMsg> buttons;
  ecs::MessageReader<JoystickAxisChangedMsg> axes;
  ecs::MessageReader<JoystickHatChangedMsg> hats;
};

/// @brief Pen proximity, touch, button, motion, and axis readers.
struct PenMessages {
  ecs::MessageReader<PenProximityMsg> proximity;
  ecs::MessageReader<PenTouchMsg> touch;
  ecs::MessageReader<PenButtonInputMsg> buttons;
  ecs::MessageReader<PenMovedMsg> moved;
  ecs::MessageReader<PenAxisChangedMsg> axes;
};

/// @brief All input message readers.
struct Messages {
  KeyboardMessages keyboard;
  MouseMessages mouse;
  GamepadMessages gamepad;
  JoystickMessages joystick;
  PenMessages pen;
};

/// @brief Keyboard key and text input message writers.
struct KeyboardWriters {
  ecs::MessageWriter<KeyboardInputMsg> keys;
  ecs::MessageWriter<TextInputMsg> text;
};

/// @brief Mouse button, cursor, motion, and wheel message writers.
struct MouseWriters {
  ecs::MessageWriter<MouseButtonInputMsg> buttons;
  ecs::MessageWriter<CursorMovedMsg> cursor;
  ecs::MessageWriter<MouseMotionMsg> motion;
  ecs::MessageWriter<MouseWheelMsg> wheel;
};

/// @brief Gamepad connection, button, axis, and extra-device message writers.
struct GamepadWriters {
  ecs::MessageWriter<GamepadConnectionMsg> connection;
  ecs::MessageWriter<GamepadButtonInputMsg> buttons;
  ecs::MessageWriter<GamepadAxisChangedMsg> axes;
  ecs::MessageWriter<GamepadRemappedMsg> remapped;
  ecs::MessageWriter<GamepadPowerChangedMsg> power;
  ecs::MessageWriter<GamepadSensorUpdateMsg> sensors;
  ecs::MessageWriter<GamepadTouchpadMsg> touchpad;
};

/// @brief Unmapped joystick connection, button, axis, and hat writers.
struct JoystickWriters {
  ecs::MessageWriter<JoystickConnectionMsg> connection;
  ecs::MessageWriter<JoystickButtonInputMsg> buttons;
  ecs::MessageWriter<JoystickAxisChangedMsg> axes;
  ecs::MessageWriter<JoystickHatChangedMsg> hats;
};

/// @brief Pen proximity, touch, button, motion, and axis writers.
struct PenWriters {
  ecs::MessageWriter<PenProximityMsg> proximity;
  ecs::MessageWriter<PenTouchMsg> touch;
  ecs::MessageWriter<PenButtonInputMsg> buttons;
  ecs::MessageWriter<PenMovedMsg> moved;
  ecs::MessageWriter<PenAxisChangedMsg> axes;
};

/// @brief All input message writers.
struct Writers {
  KeyboardWriters keyboard;
  MouseWriters mouse;
  GamepadWriters gamepad;
  JoystickWriters joystick;
  PenWriters pen;
};

}  // namespace helios::input

namespace helios::ecs {

template <>
struct SystemParamTraits<input::State>
    : CompositeSystemParam<input::State, Res<input::Keyboard>,
                           Res<input::Mouse>, Res<input::Gamepads>,
                           Res<input::Joysticks>, Res<input::Pens>> {};

template <>
struct SystemParamTraits<input::StateView>
    : CompositeSystemParam<input::StateView, Res<const input::Keyboard>,
                           Res<const input::Mouse>, Res<const input::Gamepads>,
                           Res<const input::Joysticks>,
                           Res<const input::Pens>> {};

template <>
struct SystemParamTraits<input::KeyboardMessages>
    : CompositeSystemParam<input::KeyboardMessages,
                           MessageReader<input::KeyboardInputMsg>,
                           MessageReader<input::TextInputMsg>> {};

template <>
struct SystemParamTraits<input::MouseMessages>
    : CompositeSystemParam<input::MouseMessages,
                           MessageReader<input::MouseButtonInputMsg>,
                           MessageReader<input::CursorMovedMsg>,
                           MessageReader<input::MouseMotionMsg>,
                           MessageReader<input::MouseWheelMsg>> {};

template <>
struct SystemParamTraits<input::GamepadMessages>
    : CompositeSystemParam<input::GamepadMessages,
                           MessageReader<input::GamepadConnectionMsg>,
                           MessageReader<input::GamepadButtonInputMsg>,
                           MessageReader<input::GamepadAxisChangedMsg>,
                           MessageReader<input::GamepadRemappedMsg>,
                           MessageReader<input::GamepadPowerChangedMsg>,
                           MessageReader<input::GamepadSensorUpdateMsg>,
                           MessageReader<input::GamepadTouchpadMsg>> {};

template <>
struct SystemParamTraits<input::JoystickMessages>
    : CompositeSystemParam<input::JoystickMessages,
                           MessageReader<input::JoystickConnectionMsg>,
                           MessageReader<input::JoystickButtonInputMsg>,
                           MessageReader<input::JoystickAxisChangedMsg>,
                           MessageReader<input::JoystickHatChangedMsg>> {};

template <>
struct SystemParamTraits<input::PenMessages>
    : CompositeSystemParam<input::PenMessages,
                           MessageReader<input::PenProximityMsg>,
                           MessageReader<input::PenTouchMsg>,
                           MessageReader<input::PenButtonInputMsg>,
                           MessageReader<input::PenMovedMsg>,
                           MessageReader<input::PenAxisChangedMsg>> {};

template <>
struct SystemParamTraits<input::Messages>
    : CompositeSystemParam<input::Messages, input::KeyboardMessages,
                           input::MouseMessages, input::GamepadMessages,
                           input::JoystickMessages, input::PenMessages> {};

template <>
struct SystemParamTraits<input::KeyboardWriters>
    : CompositeSystemParam<input::KeyboardWriters,
                           MessageWriter<input::KeyboardInputMsg>,
                           MessageWriter<input::TextInputMsg>> {};

template <>
struct SystemParamTraits<input::MouseWriters>
    : CompositeSystemParam<input::MouseWriters,
                           MessageWriter<input::MouseButtonInputMsg>,
                           MessageWriter<input::CursorMovedMsg>,
                           MessageWriter<input::MouseMotionMsg>,
                           MessageWriter<input::MouseWheelMsg>> {};

template <>
struct SystemParamTraits<input::GamepadWriters>
    : CompositeSystemParam<input::GamepadWriters,
                           MessageWriter<input::GamepadConnectionMsg>,
                           MessageWriter<input::GamepadButtonInputMsg>,
                           MessageWriter<input::GamepadAxisChangedMsg>,
                           MessageWriter<input::GamepadRemappedMsg>,
                           MessageWriter<input::GamepadPowerChangedMsg>,
                           MessageWriter<input::GamepadSensorUpdateMsg>,
                           MessageWriter<input::GamepadTouchpadMsg>> {};

template <>
struct SystemParamTraits<input::JoystickWriters>
    : CompositeSystemParam<input::JoystickWriters,
                           MessageWriter<input::JoystickConnectionMsg>,
                           MessageWriter<input::JoystickButtonInputMsg>,
                           MessageWriter<input::JoystickAxisChangedMsg>,
                           MessageWriter<input::JoystickHatChangedMsg>> {};

template <>
struct SystemParamTraits<input::PenWriters>
    : CompositeSystemParam<input::PenWriters,
                           MessageWriter<input::PenProximityMsg>,
                           MessageWriter<input::PenTouchMsg>,
                           MessageWriter<input::PenButtonInputMsg>,
                           MessageWriter<input::PenMovedMsg>,
                           MessageWriter<input::PenAxisChangedMsg>> {};

template <>
struct SystemParamTraits<input::Writers>
    : CompositeSystemParam<input::Writers, input::KeyboardWriters,
                           input::MouseWriters, input::GamepadWriters,
                           input::JoystickWriters, input::PenWriters> {};

}  // namespace helios::ecs
