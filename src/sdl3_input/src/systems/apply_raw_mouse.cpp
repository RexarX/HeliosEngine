#include <pch.hpp>

#include <helios/sdl3/input/systems/apply_raw_mouse.hpp>

#include <helios/ecs/resource/params.hpp>

#include <SDL3/SDL.h>

namespace helios::sdl3::input {

void ApplyRawMouseMotion::operator()(
    ecs::Res<Context> context,
    ecs::Res<const helios::input::Settings> settings) const {
  if (!context->input_enabled) [[unlikely]] {
    return;
  }

  if (context->raw_mouse_hint_set &&
      context->last_raw_mouse_motion == settings->raw_mouse_motion) {
    return;
  }

  SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE,
              settings->raw_mouse_motion ? "0" : "1");
  context->raw_mouse_hint_set = true;
  context->last_raw_mouse_motion = settings->raw_mouse_motion;
}

}  // namespace helios::sdl3::input
