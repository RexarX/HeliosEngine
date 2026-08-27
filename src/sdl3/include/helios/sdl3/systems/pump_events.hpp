#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/window/resources.hpp>

#include <string_view>
#endif
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/event_dispatcher.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3 {

/// @brief Pumps the SDL event queue on the main thread.
struct PumpEvents {
  static constexpr std::string_view kName = "helios::sdl3::PumpEvents";

  void operator()(ecs::Res<Context> context,
                  ecs::Res<EventDispatcher> dispatcher,
                  ecs::Res<const window::Settings> settings) const;
};

}  // namespace helios::sdl3
#endif  // HELIOS_MODULE_CONSUMER_SHIM
