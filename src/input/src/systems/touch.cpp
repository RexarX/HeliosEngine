#include <pch.hpp>

#include <helios/input/systems/touch.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/input/params.hpp>
#include <helios/input/touch.hpp>

namespace helios::input {

namespace {

void ApplyTouchPosition(TouchFinger& finger, double x, double y) noexcept {
  if (finger.has_position) {
    finger.delta_x += x - finger.position_x;
    finger.delta_y += y - finger.position_y;
  }
  finger.position_x = x;
  finger.position_y = y;
  finger.has_position = true;
}

}  // namespace

void UpdateTouchState::operator()(ecs::Res<Touches> touches,
                                  TouchMessages messages) const {
  for (const auto msg : messages.fingers) {
    TouchFinger* finger = touches->TryGet(msg->id);
    if (finger == nullptr) [[unlikely]] {
      continue;
    }

    finger->id = msg->id;
    finger->pressure = msg->pressure;
    finger->device_type = msg->device_type;
    switch (msg->phase) {
      using enum TouchPhase;
      case kStarted:
        finger->down = true;
        ApplyTouchPosition(*finger, msg->x, msg->y);
        break;
      case kMoved:
        ApplyTouchPosition(*finger, msg->x, msg->y);
        break;
      case kEnded:
      case kCanceled:
        finger->down = false;
        ApplyTouchPosition(*finger, msg->x, msg->y);
        break;
    }
  }
}

}  // namespace helios::input
