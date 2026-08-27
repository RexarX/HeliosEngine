#include <pch.hpp>

#include <helios/sdl3/window/systems/poll.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/sdl3/context.hpp>
#include <helios/sdl3/window/state.hpp>
#include <helios/sdl3/window/sync.hpp>

namespace helios::sdl3::window {

void PollEvents::operator()(ecs::Res<Context> context,
                            ecs::Res<NativeWindows> native,
                            ecs::Res<sdl3::Context> sdl_context) const {
  if (!context->initialized || sdl_context->world == nullptr) [[unlikely]] {
    return;
  }

  SyncClipboard(*sdl_context->world, *native, *context);
}

}  // namespace helios::sdl3::window
