#include <pch.hpp>

#include <helios/glfw/systems/poll.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/glfw/details/glfw_sync.hpp>
#include <helios/window/resources.hpp>

#include <GLFW/glfw3.h>

#include <cmath>

namespace helios::glfw {

void PollEvents::operator()(ecs::Res<Context> context,
                            ecs::Res<NativeWindows> native,
                            ecs::Res<const window::Settings> settings) const {
  if (!context->initialized) [[unlikely]] {
    return;
  }

  if (context->in_event_poll) [[unlikely]] {
    return;
  }

  context->in_event_poll = true;
  if (settings->event_mode == window::EventMode::kWaitTimeout) {
    const double timeout = settings->event_wait_timeout;
    HELIOS_ASSERT(std::isfinite(timeout) && timeout > 0.0,
                  "event_wait_timeout must be finite and greater than zero, "
                  "got {}!",
                  timeout);
    glfwWaitEventsTimeout(timeout);
  } else {
    glfwPollEvents();
  }

  if (context->world != nullptr) [[likely]] {
    SyncClipboard(*context->world, *native, *context);
  }

  context->in_event_poll = false;
}

}  // namespace helios::glfw
