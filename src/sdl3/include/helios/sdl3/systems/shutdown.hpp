#pragma once

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/context.hpp>

#include <string_view>

namespace helios::sdl3 {

/// @brief Clears SDL runtime pointers owned by `Context`.
struct Shutdown {
  static constexpr std::string_view kName = "helios::sdl3::Shutdown";

  void operator()(ecs::Res<Context> context) const;
};

}  // namespace helios::sdl3
