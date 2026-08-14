#pragma once

#include <helios/ecs/message/reader.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/system/composite_param.hpp>
#include <helios/ecs/system/param_traits.hpp>
#include <helios/input/messages.hpp>
#include <helios/input/resources.hpp>

namespace helios::input {

/// @brief Mutable keyboard, mouse, and gamepad resources.
struct State {
  ecs::Res<Keyboard> keyboard;
  ecs::Res<Mouse> mouse;
  ecs::Res<Gamepads> gamepads;
};

/// @brief Read-only keyboard, mouse, and gamepad resources.
struct StateView {
  ecs::Res<const Keyboard> keyboard;
  ecs::Res<const Mouse> mouse;
  ecs::Res<const Gamepads> gamepads;
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

/// @brief Gamepad connection, button, and axis message readers.
struct GamepadMessages {
  ecs::MessageReader<GamepadConnectionMsg> connection;
  ecs::MessageReader<GamepadButtonInputMsg> buttons;
  ecs::MessageReader<GamepadAxisChangedMsg> axes;
};

/// @brief All input message readers.
struct Messages {
  KeyboardMessages keyboard;
  MouseMessages mouse;
  GamepadMessages gamepad;
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

/// @brief Gamepad connection, button, and axis message writers.
struct GamepadWriters {
  ecs::MessageWriter<GamepadConnectionMsg> connection;
  ecs::MessageWriter<GamepadButtonInputMsg> buttons;
  ecs::MessageWriter<GamepadAxisChangedMsg> axes;
};

/// @brief All input message writers.
struct Writers {
  KeyboardWriters keyboard;
  MouseWriters mouse;
  GamepadWriters gamepad;
};

}  // namespace helios::input

namespace helios::ecs {

template <>
struct SystemParamTraits<helios::input::State>
    : CompositeSystemParam<helios::input::State, Res<helios::input::Keyboard>,
                           Res<helios::input::Mouse>,
                           Res<helios::input::Gamepads>> {};

template <>
struct SystemParamTraits<helios::input::StateView>
    : CompositeSystemParam<
          helios::input::StateView, Res<const helios::input::Keyboard>,
          Res<const helios::input::Mouse>, Res<const helios::input::Gamepads>> {
};

template <>
struct SystemParamTraits<helios::input::KeyboardMessages>
    : CompositeSystemParam<helios::input::KeyboardMessages,
                           MessageReader<helios::input::KeyboardInputMsg>,
                           MessageReader<helios::input::TextInputMsg>> {};

template <>
struct SystemParamTraits<helios::input::MouseMessages>
    : CompositeSystemParam<helios::input::MouseMessages,
                           MessageReader<helios::input::MouseButtonInputMsg>,
                           MessageReader<helios::input::CursorMovedMsg>,
                           MessageReader<helios::input::MouseMotionMsg>,
                           MessageReader<helios::input::MouseWheelMsg>> {};

template <>
struct SystemParamTraits<helios::input::GamepadMessages>
    : CompositeSystemParam<
          helios::input::GamepadMessages,
          MessageReader<helios::input::GamepadConnectionMsg>,
          MessageReader<helios::input::GamepadButtonInputMsg>,
          MessageReader<helios::input::GamepadAxisChangedMsg>> {};

template <>
struct SystemParamTraits<helios::input::Messages>
    : CompositeSystemParam<
          helios::input::Messages, helios::input::KeyboardMessages,
          helios::input::MouseMessages, helios::input::GamepadMessages> {};

template <>
struct SystemParamTraits<helios::input::KeyboardWriters>
    : CompositeSystemParam<helios::input::KeyboardWriters,
                           MessageWriter<helios::input::KeyboardInputMsg>,
                           MessageWriter<helios::input::TextInputMsg>> {};

template <>
struct SystemParamTraits<helios::input::MouseWriters>
    : CompositeSystemParam<helios::input::MouseWriters,
                           MessageWriter<helios::input::MouseButtonInputMsg>,
                           MessageWriter<helios::input::CursorMovedMsg>,
                           MessageWriter<helios::input::MouseMotionMsg>,
                           MessageWriter<helios::input::MouseWheelMsg>> {};

template <>
struct SystemParamTraits<helios::input::GamepadWriters>
    : CompositeSystemParam<
          helios::input::GamepadWriters,
          MessageWriter<helios::input::GamepadConnectionMsg>,
          MessageWriter<helios::input::GamepadButtonInputMsg>,
          MessageWriter<helios::input::GamepadAxisChangedMsg>> {};

template <>
struct SystemParamTraits<helios::input::Writers>
    : CompositeSystemParam<
          helios::input::Writers, helios::input::KeyboardWriters,
          helios::input::MouseWriters, helios::input::GamepadWriters> {};

}  // namespace helios::ecs
