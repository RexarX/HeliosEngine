#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.glfw;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world.hpp>
#include <helios/window/resources.hpp>

#include <string_view>
#endif
#include <helios/glfw/state.hpp>

HELIOS_MODULE_EXPORT
namespace helios::glfw {

/// @brief Initializes GLFW on the main thread.
struct Init {
  static constexpr std::string_view kName = "helios::glfw::Init";

  void operator()(ecs::World& world, ecs::Res<Context> context,
                  ecs::Res<window::Monitors> monitors) const;
};

}  // namespace helios::glfw
#endif  // HELIOS_MODULE_CONSUMER_SHIM
