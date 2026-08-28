#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world.hpp>

#include <string_view>
#endif
#include <helios/sdl3/context.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3 {

/// @brief Stores the main world pointer used by the SDL event pump.
struct Init {
  static constexpr std::string_view kName = "helios::sdl3::Init";

  void operator()(ecs::World& world, ecs::Res<Context> context) const;
};

}  // namespace helios::sdl3
#endif  // HELIOS_MODULE_CONSUMER_SHIM
