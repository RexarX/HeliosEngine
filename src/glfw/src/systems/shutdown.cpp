#include <pch.hpp>

#include <helios/glfw/systems/shutdown.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/glfw/state.hpp>
#include <helios/window/components.hpp>
#include <helios/window/native_handle.hpp>

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
#include <helios/glfw/systems/input.hpp>
#endif

#include <GLFW/glfw3.h>

namespace helios::glfw {

void Shutdown::operator()(ecs::Res<Context> context,
                          ecs::Res<NativeWindows> native) const {
  if (!context->initialized) [[unlikely]] {
    return;
  }

  UnregisterMonitorCallback();

  for (auto& entry : native->entries) {
    if (entry.native.window != nullptr) [[likely]] {
      context->world->TryRemoveComponents<window::NativeHandleComponent>(
          entry.entity);
      glfwDestroyWindow(entry.native.window);
      entry.native.window = nullptr;
    }
  }

  native->entries.clear();

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
  if (context->world != nullptr) [[likely]] {
    if (auto* cache = context->world->TryWriteResource<CursorCache>();
        cache != nullptr) {
      DestroyCursorCache(*cache);
    }
  }
#endif

  glfwTerminate();
  context->initialized = false;
  context->world = nullptr;
}

}  // namespace helios::glfw
