#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/glfw/glfw.hpp>
#include <helios/input/input.hpp>
#include <helios/log/log.hpp>
#include <helios/window/window.hpp>

#include <helios/profile/backends/tracy.hpp>
#include <helios/profile/profile.hpp>

#include <format>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;
namespace hwindow = helios::window;
namespace hinput = helios::input;
namespace hglfw = helios::glfw;

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

struct LogInput {
  void operator()(hinput::KeyboardMessages keyboard_messages,
                  hinput::MouseMessages mouse_messages,
                  hinput::GamepadMessages gamepad_messages) const {
    for (const auto msg : keyboard_messages.keys) {
      hlog::Info("{} | {} | {}", msg->key, msg->state, msg->modifiers);
    }

    for (const auto msg : mouse_messages.buttons) {
      hlog::Info("{} | {} | {}", msg->button, msg->state, msg->modifiers);
    }

    for (const auto msg : mouse_messages.cursor) {
      hlog::Info("Mouse moved: x={}, y={}", msg->x, msg->y);
    }

    for (const auto msg : mouse_messages.motion) {
      hlog::Info("Mouse motion (delta): x={}, y={}", msg->delta_x,
                 msg->delta_y);
    }

    for (const auto msg : mouse_messages.wheel) {
      hlog::Info("Mouse wheel: x={}, y={}", msg->x, msg->y);
    }

    for (const auto msg : mouse_messages.wheel) {
      hlog::Info("Mouse wheel: x={}, y={}", msg->x, msg->y);
    }

    for (const auto msg : gamepad_messages.buttons) {
      hlog::Info("Gamepad: {} | {} | {}", msg->id, msg->button, msg->state);
    }

    for (const auto msg : gamepad_messages.axes) {
      hlog::Info("Gamepad: {} | {} | {}", msg->id, msg->axis, msg->value);
    }

    for (const auto msg : gamepad_messages.connection) {
      hlog::Info("Gamepad: {} | {} | connected={}", msg->id, msg->name,
                 msg->connected);
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
  auto& profiler = helios::profile::Profiler::Instance();
  profiler.AddBackend<helios::profile::TracyBackend>();
  profiler.Finalize();
  std::this_thread::sleep_for(std::chrono::milliseconds(300));

  happ::App app;
  app.AddPluginGroups(hglfw::WindowInputPlugin{}.Configure(
      hinput::Plugin{{.raw_mouse_motion = true}}));
  app.AddSystem(happ::kStartup, SpawnWindowStartup{});
  app.AddSystems(happ::kUpdate, ChangeCursor{}, LogInput{},
                 RequestCloseOnEscape{});
  return std::to_underlying(app.Run());
}
