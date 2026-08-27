#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.glfw;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/window/params.hpp>

#include <string_view>
#endif
#include <helios/glfw/state.hpp>

HELIOS_MODULE_EXPORT
namespace helios::glfw {

/// @brief Applies dirty window properties to native windows.
struct ApplyChanges {
  static constexpr std::string_view kName = "helios::glfw::ApplyChanges";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<NativeWindows> native, window::Windows windows,
                  window::AppearanceWriters appearance,
                  ecs::MessageWriter<window::PosChangedMsg> pos_changed) const;
};

}  // namespace helios::glfw
#endif  // HELIOS_MODULE_CONSUMER_SHIM
