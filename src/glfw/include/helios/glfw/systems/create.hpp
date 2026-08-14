#pragma once

#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/window/components.hpp>
#include <helios/window/params.hpp>

#include <string_view>

namespace helios::glfw {

/// @brief Creates native windows for new `window::Window` entities.
struct CreateNativeWindows {
  static constexpr std::string_view kName = "helios::glfw::CreateNativeWindows";

  void operator()(
      ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
      ecs::Query<window::Window&, ecs::Without<window::CreationFailed>> windows,
      window::CreationWriters writers) const;
};

}  // namespace helios::glfw
