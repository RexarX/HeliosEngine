#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/window/resources.hpp>

#include <string_view>
#endif
#include <helios/sdl3/window/state.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

/// @brief Initializes SDL video and registers window event handlers.
struct Init {
  static constexpr std::string_view kName = "helios::sdl3::window::Init";

  void operator()(ecs::Res<Context> context,
                  ecs::Res<::helios::window::Monitors> monitors,
                  ecs::Res<sdl3::EventDispatcher> dispatcher) const;
};

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
