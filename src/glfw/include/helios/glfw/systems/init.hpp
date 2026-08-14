#pragma once

#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/world.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/window/resources.hpp>

#include <string_view>

namespace helios::glfw {

/// @brief Initializes GLFW on the main thread.
struct Init {
  static constexpr std::string_view kName = "helios::glfw::Init";

  void operator()(ecs::World& world, ecs::Res<Context> context,
                  ecs::Res<window::Monitors> monitors) const;
};

}  // namespace helios::glfw
