#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/input/input.hpp>
#include <helios/log/log.hpp>
#include <helios/sdl3/input/input.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/window/window.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;
namespace hwindow = helios::window;
namespace hinput = helios::input;
namespace hsdl3 = helios::sdl3;

namespace {

struct SpawnWindowStartup {
  void operator()(hecs::Commands commands) const {
    commands.Spawn()
        .AddBundle(hwindow::PrimaryWindow{})
        .AddComponents(hinput::Cursor{});
  }
};

struct ChangeCursor {
  void operator()(hecs::Res<const hinput::Keyboard> keyboard,
                  hecs::Query<hinput::Cursor&> cursors) const {
    if (keyboard->keys.JustPressed(hinput::Key::kC)) {
      for (auto&& [cursor] : cursors) {
        const auto next = static_cast<hinput::CursorIcon>(
            (std::to_underlying(cursor.icon) + 1) %
            std::to_underlying(hinput::CursorIcon::kCount));
        cursor.SetIcon(next);
        hlog::Info("Cursor icon -> {}", next);
      }
    }
  }
};

struct LogKeyboardInput {
  void operator()(hinput::KeyboardMessages keyboard_messages) const {
    for (const auto msg : keyboard_messages.keys) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : keyboard_messages.text) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : keyboard_messages.editing) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : keyboard_messages.candidates) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : keyboard_messages.connection) {
      hlog::Info("{}", *msg);
    }
  };
};

struct LogMouseInput {
  void operator()(hinput::MouseMessages mouse_messages) const {
    for (const auto msg : mouse_messages.buttons) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : mouse_messages.cursor) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : mouse_messages.motion) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : mouse_messages.wheel) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : mouse_messages.connection) {
      hlog::Info("{}", *msg);
    }
  }
};

struct LogGamepadInput {
  void operator()(hinput::GamepadMessages gamepad_messages) const {
    for (const auto msg : gamepad_messages.buttons) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : gamepad_messages.axes) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : gamepad_messages.connection) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : gamepad_messages.touchpad) {
      hlog::Info("{}", *msg);
    }
  }
};

struct LogJoystickInput {
  void operator()(hinput::JoystickMessages joystick_messages) const {
    for (const auto msg : joystick_messages.connection) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : joystick_messages.buttons) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : joystick_messages.axes) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : joystick_messages.hats) {
      hlog::Info("{}", *msg);
    }
  }
};

struct LogPenInput {
  void operator()(hinput::PenMessages pen_messages) const {
    for (const auto msg : pen_messages.proximity) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : pen_messages.touch) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : pen_messages.buttons) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : pen_messages.moved) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : pen_messages.axes) {
      hlog::Info("{}", *msg);
    }
  }
};

struct LogTouchInput {
  void operator()(hinput::TouchMessages touch_messages) const {
    for (const auto msg : touch_messages.fingers) {
      hlog::Info("{}", *msg);
    }
  }
};

struct LogSensorInput {
  void operator()(hinput::SensorMessages sensor_messages) const {
    for (const auto msg : sensor_messages.connection) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : sensor_messages.samples) {
      hlog::Info("{}", *msg);
    }
  }
};

struct RequestCloseOnEscape {
  void operator()(
      hecs::Query<hwindow::Window&, hecs::With<hwindow::Primary>> query,
      hecs::Res<const hinput::Keyboard> keyboard) const {
    if (!keyboard->keys.JustPressed(hinput::Key::kEscape)) {
      return;
    }

    for (auto&& [window] : query) {
      window.RequestClose();
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPluginGroups(hsdl3::input::InputPlugin{{.raw_mouse_motion = true}},
                      hsdl3::window::WindowPlugin{});
  app.AddSystem(happ::kStartup, SpawnWindowStartup{});
  app.AddSystems(happ::kUpdate, ChangeCursor{}, LogKeyboardInput{},
                 LogMouseInput{}, LogGamepadInput{}, LogJoystickInput{},
                 LogPenInput{}, LogTouchInput{}, LogSensorInput{},
                 RequestCloseOnEscape{});
  return std::to_underlying(app.Run());
}
