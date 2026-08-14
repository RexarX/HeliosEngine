#pragma once

#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/window/params.hpp>

#include <string_view>

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
