#include <pch.hpp>

#include <helios/sdl3/window/systems/destroy.hpp>

#include <helios/app/application.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world_view.hpp>
#include <helios/sdl3/window/details/native_state.hpp>
#include <helios/sdl3/window/details/window_map.hpp>
#include <helios/window/components.hpp>
#include <helios/window/messages.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/params.hpp>
#include <helios/window/resources.hpp>

#include <SDL3/SDL.h>

namespace helios::sdl3::window {

void DestroyClosedWindows::operator()(
    ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
    ecs::Res<WindowMap> window_map,
    ecs::Res<const ::helios::window::Settings> settings,
    ::helios::window::Windows windows, ecs::WorldView world_view,
    ecs::Commands commands,
    ecs::MessageWriter<::helios::window::ClosedMsg> closed,
    ecs::MessageWriter<app::AppExit> app_exit) const {
  if (!context->initialized) [[unlikely]] {
    return;
  }

  for (auto&& [entity, window] : windows.query.WithEntity()) {
    if (!window.close_requested) [[likely]] {
      continue;
    }

    const NativeWindows::Entry* entry = native->TryGet(entity);
    if (entry == nullptr) [[unlikely]] {
      continue;
    }

    const bool was_primary =
        world_view.HasComponent<::helios::window::Primary>(entity);
    const bool is_last_native_window = native->Size() == 1;

    SDL_StopTextInput(entry->native.window);
    SDL_DestroyWindow(entry->native.window);
    native->Erase(entity);
    window_map->Erase(entity);
    closed.Write({.entity = entity});
    commands.Entity(entity)
        .TryRemoveComponents<::helios::window::NativeHandleComponent>()
        .TryDestroy();

    if (::helios::window::ShouldRequestExitOnClose(
            settings->exit_triggers, was_primary, is_last_native_window)) {
      app_exit.Write(app::AppExit::Success());
    }
  }
}

}  // namespace helios::sdl3::window
