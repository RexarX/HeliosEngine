#pragma once

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world.hpp>
#include <helios/sdl3/details/context.hpp>

#include <string_view>

namespace helios::sdl3 {

/// @brief Stores the main world pointer used by the SDL event pump.
struct Init {
  static constexpr std::string_view kName = "helios::sdl3::Init";

  void operator()(ecs::World& world, ecs::Res<Context> context) const;
};

}  // namespace helios::sdl3
