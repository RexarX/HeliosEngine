#include <pch.hpp>

#include <helios/sdl3/plugin.hpp>

#include <helios/app/application.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/context.hpp>
#include <helios/sdl3/details/event_dispatcher.hpp>
#include <helios/sdl3/systems/init.hpp>
#include <helios/sdl3/systems/pump_events.hpp>
#include <helios/sdl3/systems/shutdown.hpp>
#include <helios/window/schedules.hpp>

namespace helios::sdl3 {

void Plugin::Build(app::App& app) {
  window::RegisterEventsSchedule(app.GetMainSubApp().GetScheduler());
  auto& order = app.GetWorld().WriteResource<app::MainFrameOrder>();
  if (!order.Contains(window::kWindowStage)) {
    order.InsertBefore(app::kUpdateStage, window::kWindowStage);
  }

  app.TryInsertResources(Context{}, EventDispatcher{});
  app.AddSystem(app::kMainStartup, Init{}).InSet(kStartupSet);
  app.AddSystem(window::kEvents, PumpEvents{}).InSet(kEventPumpSet);
  app.AddSystem(app::kShutdown, Shutdown{}).InSet(kShutdownSet);
}

void Plugin::Finish(app::App& app) {
  auto& context = app.GetWorld().WriteResource<Context>();
  context.frame_pump_user_data = &app;
  context.frame_pump = [](void* user_data) {
    auto& application = *static_cast<app::App*>(user_data);
    application.RunFrameOrder(
        application.GetWorld().ReadResource<app::FramePumpOrder>());
  };
}

void Plugin::Destroy(app::App& app) {
  if (auto* context = app.GetWorld().TryWriteResource<Context>();
      context != nullptr) [[likely]] {
    Shutdown{}(ecs::Res<Context>(*context));
  }
}

}  // namespace helios::sdl3
