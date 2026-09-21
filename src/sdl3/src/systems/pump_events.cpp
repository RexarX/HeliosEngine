#include <pch.hpp>

#include <helios/sdl3/systems/pump_events.hpp>

#include <helios/app/builtin/app_exit.hpp>
#include <helios/assert.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/lifetime.hpp>
#include <helios/window/clipboard.hpp>
#include <helios/window/monitor.hpp>
#include <helios/window/settings.hpp>

#include <SDL3/SDL_events.h>

namespace helios::sdl3 {

namespace {

bool ShouldRequestNestedPump(const SDL_Event& event) {
  switch (event.type) {
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_MOVED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
      return true;
    default:
      return false;
  }
}

void RequestNestedFramePump(Context& context) {
  if (!context.in_event_poll || context.frame_pump == nullptr ||
      context.in_nested_pump) {
    return;
  }

  context.in_nested_pump = true;
  context.frame_pump(context.frame_pump_user_data);
  context.in_nested_pump = false;
}

void HandleQuitEvent(ecs::World& world) {
  if (!world.HasMessage<app::AppExit>()) [[unlikely]] {
    return;
  }

  world.WriteMessages<app::AppExit>().Write(app::AppExit::Success());
}

void DispatchEvent(EventDispatcher& dispatcher, Context& context,
                   const SDL_Event& event) {
  if (event.type == SDL_EVENT_QUIT) {
    HandleQuitEvent(*context.world);
  }

  dispatcher.Dispatch(event, *context.world);
  if (ShouldRequestNestedPump(event)) {
    RequestNestedFramePump(context);
  }
}

void DrainPolledEvents(EventDispatcher& dispatcher, Context& context) {
  SDL_Event event{};
  while (SDL_PollEvent(&event)) {
    DispatchEvent(dispatcher, context, event);
  }
}

}  // namespace

void PumpEvents::operator()(ecs::Res<Context> context,
                            ecs::Res<EventDispatcher> dispatcher,
                            ecs::Res<const window::Settings> settings) const {
  if (!Initialized() || context->world == nullptr) [[unlikely]] {
    return;
  }

  if (context->in_event_poll) [[unlikely]] {
    return;
  }

  context->in_event_poll = true;

  if (settings->event_mode == window::EventMode::kWaitTimeout) {
    const double timeout = settings->event_wait_timeout;
    HELIOS_ASSERT(std::isfinite(timeout) && timeout > 0.0,
                  "event_wait_timeout must be finite and greater than zero, "
                  "got {}!",
                  timeout);
    const auto timeout_ms = static_cast<Sint32>(timeout * 1000.0 + 0.5);
    SDL_Event event{};
    if (SDL_WaitEventTimeout(&event, timeout_ms)) {
      DispatchEvent(*dispatcher, *context, event);
    }
  }

  DrainPolledEvents(*dispatcher, *context);

  context->in_event_poll = false;
}

}  // namespace helios::sdl3
