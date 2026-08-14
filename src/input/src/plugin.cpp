#include <pch.hpp>

#include <helios/input/plugin.hpp>

#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>
#include <helios/input/messages.hpp>
#include <helios/input/resources.hpp>
#include <helios/input/systems.hpp>

namespace helios::input {

void Plugin::Build(app::App& app) {
  app.TryInsertResources(std::move(settings), Keyboard{}, Mouse{}, Gamepads{});
  app.AddMessages<KeyboardInputMsg, TextInputMsg, MouseButtonInputMsg,
                  CursorMovedMsg, MouseMotionMsg, MouseWheelMsg,
                  GamepadConnectionMsg, GamepadButtonInputMsg,
                  GamepadAxisChangedMsg>();
  auto clear_system = app.AddSystem(app::kFirst, ClearInputState{});
  app.AddSystems(app::kFirst, UpdateKeyboardState{}, UpdateMouseState{},
                 UpdateGamepadState{})
      .After(clear_system);
}

}  // namespace helios::input
