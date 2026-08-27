#include <pch.hpp>

#include <helios/sdl3/window/systems/shutdown.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/lifetime.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/window_map.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/properties.hpp>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_video.h>

namespace helios::sdl3::window {

void Shutdown::operator()(ecs::Res<Context> context,
                          ecs::Res<NativeWindows> native,
                          ecs::Res<WindowMap> window_map,
                          ecs::Res<sdl3::Context> sdl_context) const {
  if (!context->initialized) [[unlikely]] {
    return;
  }

  for (auto& entry : native->entries) {
    if (entry.native.window != nullptr) [[likely]] {
      if (sdl_context->world != nullptr) [[likely]] {
        sdl_context->world
            ->TryRemoveComponents<::helios::window::NativeHandleComponent>(
                entry.entity);
      }
      SDL_StopTextInput(entry.native.window);
      SDL_DestroyWindow(entry.native.window);
      entry.native.window = nullptr;
    }
    window_map->Erase(entry.entity);
  }

  native->entries.clear();
  window_map->entries.clear();

  sdl3::Release(SDL_INIT_VIDEO);
  context->initialized = false;
}

}  // namespace helios::sdl3::window
