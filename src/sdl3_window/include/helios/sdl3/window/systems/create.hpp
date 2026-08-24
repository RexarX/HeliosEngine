#pragma once

#include <helios/ecs/query/params.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/details/context.hpp>
#include <helios/sdl3/window/details/native_state.hpp>
#include <helios/sdl3/window/details/window_map.hpp>
#include <helios/window/components.hpp>
#include <helios/window/params.hpp>

#include <string_view>

namespace helios::sdl3::window {

/// @brief Creates native SDL windows for new `::helios::window::Window`
/// entities.
struct CreateNativeWindows {
  static constexpr std::string_view kName =
      "helios::sdl3::window::CreateNativeWindows";

  void operator()(ecs::Res<const Context> context,
                  ecs::Res<NativeWindows> native,
                  ecs::Res<WindowMap> window_map,
                  ecs::Res<const sdl3::Context> sdl_context,
                  ecs::Query<::helios::window::Window&,
                             ecs::Without<::helios::window::CreationFailed>>
                      windows,
                  ::helios::window::CreationWriters writers) const;
};

}  // namespace helios::sdl3::window
