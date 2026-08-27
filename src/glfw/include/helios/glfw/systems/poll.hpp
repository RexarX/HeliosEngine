#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.glfw;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>
#include <helios/window/resources.hpp>

#include <string_view>
#endif
#include <helios/glfw/state.hpp>

HELIOS_MODULE_EXPORT
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
#endif  // HELIOS_MODULE_CONSUMER_SHIM
