#pragma once

#include <helios/ecs/resource/params.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/window/resources.hpp>

#include <string_view>

namespace helios::glfw {

/// @brief Polls or waits for GLFW events on the main thread.
/// @details Uses `glfwPollEvents` by default. `EventMode::kWaitTimeout` calls
/// `glfwWaitEventsTimeout` so editors can idle; infinite `glfwWaitEvents` is
/// not used because GLFW joysticks are not OS events.
struct PollEvents {
  static constexpr std::string_view kName = "helios::glfw::PollEvents";

  void operator()(ecs::Res<Context> context, ecs::Res<NativeWindows> native,
                  ecs::Res<const window::Settings> settings) const;
};

}  // namespace helios::glfw
