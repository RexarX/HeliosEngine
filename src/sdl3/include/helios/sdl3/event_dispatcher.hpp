#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/world.hpp>

#include <string_view>
#include <vector>
#endif
#include <helios/assert.hpp>

HELIOS_MODULE_EXPORT union SDL_Event;

HELIOS_MODULE_EXPORT
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
#endif  // HELIOS_MODULE_CONSUMER_SHIM
