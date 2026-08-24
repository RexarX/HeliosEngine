#pragma once

#include <helios/ecs/resource/params.hpp>
#include <helios/input/resources.hpp>
#include <helios/sdl3/input/details/input_state.hpp>

#include <string_view>

namespace helios::sdl3::input {

/// @brief Applies the raw-mouse-motion setting to SDL relative mouse scaling.
struct ApplyRawMouseMotion {
  static constexpr std::string_view kName =
      "helios::sdl3::input::ApplyRawMouseMotion";

  void operator()(ecs::Res<Context> context,
                  ecs::Res<const helios::input::Settings> settings) const;
};

}  // namespace helios::sdl3::input
