#include <pch.hpp>

#include <helios/sdl3/input/plugin.hpp>

#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/messages.hpp>
#include <helios/sdl3/details/lifetime.hpp>
#include <helios/sdl3/input/details/cursor_cache.hpp>
#include <helios/sdl3/input/details/input_state.hpp>
#include <helios/sdl3/input/systems/apply_cursors.hpp>
#include <helios/sdl3/input/systems/apply_raw_mouse.hpp>
#include <helios/sdl3/input/systems/init.hpp>
#include <helios/sdl3/input/systems/poll_gamepads.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/window/schedules.hpp>

#include <SDL3/SDL.h>

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/plugin.hpp>
#endif

namespace helios::sdl3::input {

void Plugin::Build(app::App& app) {
  app.TryInsertResources(Context{});

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
  app.ConfigureSet(app::kMainStartup, window::kStartupSet);
  app.ConfigureSet(::helios::window::kEvents, window::kApplySet);
#endif

  auto init = app.AddSystem(app::kMainStartup, Init{})
                  .InSet(kStartupSet)
                  .AfterSet(sdl3::kStartupSet);
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
  init.AfterSet(window::kStartupSet);
#endif
}

void Plugin::Finish(app::App& app) {
  auto& world = app.GetWorld();
  auto& context = world.WriteResource<Context>();
  context.input_enabled = world.HasMessage<helios::input::KeyboardInputMsg>();
  if (!context.input_enabled) {
    return;
  }

  app.TryInsertResources(GamepadCache{}, CursorCache{}, PenCache{});
  auto systems =
      app.AddSystems(::helios::window::kEvents, ApplyGamepadMappings{},
                     PollGamepads{}, ApplyGamepadOutputs{}, ApplyCursors{},
                     ApplyRawMouseMotion{})
          .InSet(kApplySet)
          .AfterSet(sdl3::kEventPumpSet)
          .Sequence();
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
  systems.BeforeSet(window::kApplySet);
#endif
}

void Plugin::Destroy(app::App& app) {
  auto& world = app.GetWorld();

  if (auto* context = world.TryWriteResource<Context>(); context != nullptr)
      [[likely]] {
    if (auto* cache = world.TryWriteResource<GamepadCache>(); cache != nullptr)
        [[likely]] {
      DestroyGamepadCache(*cache);
    }
    if (auto* cache = world.TryWriteResource<CursorCache>(); cache != nullptr)
        [[likely]] {
      DestroyCursorCache(*cache);
    }
    if (context->gamepad_subsystem_retained) {
      Release(SDL_INIT_GAMEPAD);
      context->gamepad_subsystem_retained = false;
    }
  }
}

}  // namespace helios::sdl3::input
