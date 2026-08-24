#include <pch.hpp>

#include <helios/glfw/plugin.hpp>

#include <helios/app/application.hpp>
#include <helios/app/frame_order.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/glfw/systems/apply.hpp>
#include <helios/glfw/systems/create.hpp>
#include <helios/glfw/systems/destroy.hpp>
#include <helios/glfw/systems/init.hpp>
#include <helios/glfw/systems/poll.hpp>
#include <helios/glfw/systems/shutdown.hpp>
#include <helios/window/plugin.hpp>
#include <helios/window/schedules.hpp>

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
#include <helios/glfw/systems/input.hpp>
#include <helios/input/messages.hpp>
#endif

namespace helios::glfw {

void Plugin::Build(app::App& app) {
  window::RegisterEventsSchedule(app.GetMainSubApp().GetScheduler());
  auto& world = app.GetWorld();
  world.WriteResource<app::MainFrameOrder>().InsertBefore(app::kUpdateStage,
                                                          window::kWindowStage);

  app.TryInsertResources(Context{}, NativeWindows{});
  app.AddSystem(app::kMainStartup, Init{});
  app.AddSystems(window::kEvents, CreateNativeWindows{}, PollEvents{},
                 ApplyChanges{}, DestroyClosedWindows{})
      .Sequence();
  app.AddSystem(app::kShutdown, Shutdown{});
}

void Plugin::Finish(app::App& app) {
  auto& world = app.GetWorld();
  auto& context = world.WriteResource<Context>();
#ifdef HELIOS_MODULE_INPUT_AVAILABLE
  // WindowInputPlugin builds glfw before input; detect input after all Builds.
  // HELIOS_MODULE_INPUT_AVAILABLE only means the module is linked.
  // WindowPlugin apps must not run systems that require input::Settings.
  context.input_enabled = world.HasMessage<input::KeyboardInputMsg>();
  if (context.input_enabled) {
    app.TryInsertResources(GamepadCache{}, CursorCache{});
    app.AddSystems(window::kEvents, ApplyGamepadMappings{}, PollGamepads{},
                   ApplyGamepadOutputs{}, ApplyCursors{}, ApplyRawMouseMotion{})
        .Sequence()
        .After<PollEvents>()
        .Before<ApplyChanges>();
  }
#endif
  context.frame_pump_user_data = &app;
  // Size/pos/scale callbacks invoke this while glfwPollEvents is on the stack
  // (OS modal resize/move). FramePumpOrder is Update only so it cannot re-enter
  // kEvents. WM_PAINT/refresh must not trigger it.
  context.frame_pump = [](void* user_data) {
    auto& application = *static_cast<app::App*>(user_data);
    application.RunFrameOrder(
        application.GetWorld().ReadResource<app::FramePumpOrder>());
  };
}

void Plugin::Destroy(app::App& app) {
  auto& world = app.GetWorld();
  if (auto* context = world.TryWriteResource<Context>(); context != nullptr)
      [[likely]] {
    context->frame_pump = nullptr;
    context->frame_pump_user_data = nullptr;

    if (context->initialized) [[likely]] {
      if (auto* native = world.TryWriteResource<NativeWindows>();
          native != nullptr) [[likely]] {
        Shutdown{}(ecs::Res<Context>(*context),
                   ecs::Res<NativeWindows>(*native));
      }
    }
  }
}

}  // namespace helios::glfw
