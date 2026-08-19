#pragma once

#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/window/details/native_state.hpp>
#include <helios/window/messages.hpp>
#include <helios/window/params.hpp>

#include <string_view>

namespace helios::sdl3::window {

/// @brief Applies dirty window properties to native SDL windows.
struct ApplyChanges {
  static constexpr std::string_view kName =
      "helios::sdl3::window::ApplyChanges";

  void operator()(
      ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
      ::helios::window::Windows windows,
      ::helios::window::AppearanceWriters appearance,
      ecs::MessageWriter<::helios::window::PosChangedMsg> pos_changed) const;
};

}  // namespace helios::sdl3::window
