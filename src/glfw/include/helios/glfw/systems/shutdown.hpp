#pragma once

#include <helios/ecs/resource/param.hpp>
#include <helios/glfw/details/glfw_state.hpp>

#include <string_view>

namespace helios::glfw {

/// @brief Destroys native windows and terminates GLFW.
struct Shutdown {
  static constexpr std::string_view kName = "helios::glfw::Shutdown";

  void operator()(ecs::Res<Context> context,
                  ecs::Res<NativeWindows> native) const;
};

}  // namespace helios::glfw
