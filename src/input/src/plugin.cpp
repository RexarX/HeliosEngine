#include <pch.hpp>

#include <helios/input/plugin.hpp>

#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/pen.hpp>
#include <helios/input/sensor.hpp>
#include <helios/input/systems/clear.hpp>
#include <helios/input/systems/gamepad.hpp>
#include <helios/input/systems/joystick.hpp>
#include <helios/input/systems/keyboard.hpp>
#include <helios/input/systems/mouse.hpp>
#include <helios/input/systems/pen.hpp>
#include <helios/input/systems/sensor.hpp>
#include <helios/input/systems/touch.hpp>
#include <helios/input/touch.hpp>

namespace helios::input {

void Plugin::Build(app::App& app) {
  app.TryInsertResources(settings, Keyboard{}, Mouse{}, Gamepads{}, Joysticks{},
                         Pens{}, Touches{}, Sensors{}, GamepadMappings{});
  app.AddMessages<
      KeyboardInputMsg, TextInputMsg, TextEditingMsg, TextEditingCandidatesMsg,
      KeyboardConnectionMsg, MouseButtonInputMsg, CursorMovedMsg,
      MouseMotionMsg, MouseWheelMsg, MouseConnectionMsg, GamepadConnectionMsg,
      GamepadButtonInputMsg, GamepadAxisChangedMsg, GamepadRemappedMsg,
      GamepadPowerChangedMsg, GamepadSensorUpdateMsg, GamepadTouchpadMsg,
      JoystickConnectionMsg, JoystickButtonInputMsg, JoystickAxisChangedMsg,
      JoystickHatChangedMsg, PenProximityMsg, PenTouchMsg, PenButtonInputMsg,
      PenMovedMsg, PenAxisChangedMsg, TouchInputMsg, SensorConnectionMsg,
      SensorUpdateMsg>();
  auto clear_system = app.AddSystem(app::kFirst, ClearInputState{});
  app.AddSystems(app::kFirst, UpdateKeyboardState{}, UpdateMouseState{},
                 UpdateGamepadState{}, UpdateJoystickState{}, UpdatePenState{},
                 UpdateTouchState{}, UpdateSensorState{})
      .After(clear_system);
}

}  // namespace helios::input
