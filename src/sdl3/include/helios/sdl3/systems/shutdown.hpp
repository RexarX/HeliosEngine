#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>

#include <string_view>
#endif
#include <helios/sdl3/context.hpp>

HELIOS_MODULE_EXPORT
namespace helios::sdl3 {

/// @brief Clears SDL runtime pointers owned by `Context`.
struct Shutdown {
  static constexpr std::string_view kName = "helios::sdl3::Shutdown";

  void operator()(ecs::Res<Context> context) const;
};

}  // namespace helios::sdl3
#endif  // HELIOS_MODULE_CONSUMER_SHIM
