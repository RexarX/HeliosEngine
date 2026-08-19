#include <pch.hpp>

#include <helios/sdl3/window/plugin.hpp>

#include <helios/app/application.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/context.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/sdl3/window/details/native_state.hpp>
#include <helios/sdl3/window/details/window_map.hpp>
#include <helios/sdl3/window/systems/apply.hpp>
#include <helios/sdl3/window/systems/create.hpp>
#include <helios/sdl3/window/systems/destroy.hpp>
#include <helios/sdl3/window/systems/init.hpp>
#include <helios/sdl3/window/systems/poll.hpp>
#include <helios/sdl3/window/systems/shutdown.hpp>
#include <helios/window/plugin.hpp>
#include <helios/window/schedules.hpp>

namespace helios::sdl3::window {

void Plugin::Build(app::App& app) {
  ::helios::window::RegisterEventsSchedule(app.GetMainSubApp().GetScheduler());
  auto& order = app.GetWorld().WriteResource<app::MainFrameOrder>();
  if (!order.Contains(::helios::window::kWindowStage)) {
    order.InsertBefore(app::kUpdateStage, ::helios::window::kWindowStage);
  }

  app.TryInsertResources(Context{}, NativeWindows{}, WindowMap{});
  app.AddSystem(app::kMainStartup, Init{})
      .InSet(kStartupSet)
      .AfterSet(sdl3::kStartupSet);
  app.AddSystem(::helios::window::kEvents, CreateNativeWindows{})
      .InSet(kCreateSet)
      .BeforeSet(sdl3::kEventPumpSet);
  app.AddSystems(::helios::window::kEvents, PollEvents{}, ApplyChanges{},
                 DestroyClosedWindows{})
      .InSet(kApplySet)
      .AfterSet(sdl3::kEventPumpSet)
      .Sequence();
  app.AddSystem(app::kShutdown, Shutdown{})
      .InSet(kShutdownSet)
      .BeforeSet(sdl3::kShutdownSet);
}

void Plugin::Destroy(app::App& app) {
  auto& world = app.GetWorld();
  if (auto* context = world.TryWriteResource<Context>(); context != nullptr)
      [[likely]] {
    if (context->initialized) [[likely]] {
      if (auto* native = world.TryWriteResource<NativeWindows>();
          native != nullptr) [[likely]] {
        if (auto* window_map = world.TryWriteResource<WindowMap>();
            window_map != nullptr) [[likely]] {
          if (auto* sdl_context = world.TryWriteResource<sdl3::Context>();
              sdl_context != nullptr) [[likely]] {
            Shutdown{}(ecs::Res<Context>(*context),
                       ecs::Res<NativeWindows>(*native),
                       ecs::Res<WindowMap>(*window_map),
                       ecs::Res<sdl3::Context>(*sdl_context));
          }
        }
      }
    }
  }
}

}  // namespace helios::sdl3::window
