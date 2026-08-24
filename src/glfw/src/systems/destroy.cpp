#include <pch.hpp>

#include <helios/glfw/systems/destroy.hpp>

#include <helios/app/builtin/app_exit.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/ecs/world_view.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/window/components.hpp>
#include <helios/window/messages.hpp>
#include <helios/window/native_handle.hpp>
#include <helios/window/params.hpp>
#include <helios/window/resources.hpp>

#include <GLFW/glfw3.h>

namespace helios::glfw {

void DestroyClosedWindows::operator()(
    ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
    ecs::Res<const window::Settings> settings, window::Windows windows,
    ecs::WorldView world_view, ecs::Commands commands,
    ecs::MessageWriter<window::ClosedMsg> closed,
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

    const bool was_primary = world_view.HasComponent<window::Primary>(entity);
    const bool is_last_native_window = native->Size() == 1;

    glfwDestroyWindow(entry->native.window);
    native->Erase(entity);
    closed.Write({.entity = entity});
    commands.Entity(entity)
        .TryRemoveComponents<window::NativeHandleComponent>()
        .TryDestroy();

    if (window::ShouldRequestExitOnClose(settings->exit_triggers, was_primary,
                                         is_last_native_window)) {
      app_exit.Write(app::AppExit::Success());
    }
  }
}

}  // namespace helios::glfw
