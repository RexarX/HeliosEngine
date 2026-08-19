#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/world.hpp>

#include <SDL3/SDL_events.h>

#include <string_view>
#include <vector>

namespace helios::sdl3 {

/// @brief Ordered SDL event handlers invoked from `PumpEvents`.
struct EventDispatcher {
  static constexpr std::string_view kName = "helios::sdl3::EventDispatcher";

  using Handler = void (*)(const SDL_Event& event, ecs::World& world);

  std::vector<Handler> handlers;

  /**
   * @brief Registers a handler invoked after earlier handlers each event.
   * @warning `handler` must not be null
   * @param handler Callback receiving the `SDL_Event` and main world
   */
  void Register(Handler handler) {
    HELIOS_ASSERT(handler != nullptr, "SDL event handler must not be null!");
    handlers.push_back(handler);
  }

  /**
   * @brief Dispatches one event to all registered handlers.
   * @param event SDL event
   * @param world ECS world to pass to handlers
   */
  void Dispatch(const SDL_Event& event, ecs::World& world) const {
    for (const Handler handler : handlers) {
      handler(event, world);
    }
  }
};

}  // namespace helios::sdl3
