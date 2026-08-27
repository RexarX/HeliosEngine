#include <pch.hpp>

#include <helios/input/systems/pen.hpp>

#include <details/apply_button.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/params.hpp>
#include <helios/input/pen.hpp>

namespace helios::input {

using details::ApplyButtonState;

namespace {

void ApplyPenPosition(Pen& pen, double x, double y) noexcept {
  if (pen.has_position) {
    pen.delta_x += x - pen.position_x;
    pen.delta_y += y - pen.position_y;
  }
  pen.position_x = x;
  pen.position_y = y;
  pen.has_position = true;
}

}  // namespace

void UpdatePenState::operator()(ecs::Res<Pens> pens,
                                PenMessages messages) const {
  for (const auto msg : messages.proximity) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr) [[unlikely]] {
      continue;
    }

    pen->Reset();
    pen->in_proximity = msg->in_proximity;
    if (msg->in_proximity) {
      pen->id = msg->id;
      pen->device_type = msg->device_type;
    }
  }

  for (const auto msg : messages.moved) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr || !pen->in_proximity) [[unlikely]] {
      continue;
    }
    ApplyPenPosition(*pen, msg->x, msg->y);
  }

  for (const auto msg : messages.axes) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr || !pen->in_proximity) [[unlikely]] {
      continue;
    }
    pen->axes.Set(msg->axis, msg->value);
    ApplyPenPosition(*pen, msg->x, msg->y);
  }

  for (const auto msg : messages.touch) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr || !pen->in_proximity) [[unlikely]] {
      continue;
    }
    pen->down = msg->down;
    pen->eraser = msg->eraser;
    ApplyPenPosition(*pen, msg->x, msg->y);
  }

  for (const auto msg : messages.buttons) {
    Pen* pen = pens->TryGet(msg->id);
    if (pen == nullptr || !pen->in_proximity) [[unlikely]] {
      continue;
    }
    ApplyButtonState(pen->buttons, msg->button, msg->state);
  }
}

}  // namespace helios::input
