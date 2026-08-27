#include <pch.hpp>

#include <helios/sdl3/input/event_handlers.hpp>

#include <helios/ecs/world.hpp>
#include <helios/input/messages.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/input/event_handlers.hpp>
#include <helios/sdl3/input/state.hpp>
#include <helios/sdl3/input/systems/poll_gamepads.hpp>

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/window_map.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <helios/sdl3/input/details/input_map.hpp>
#include <optional>
#include <string_view>

namespace helios::sdl3::input {

namespace {

[[nodiscard]] bool InputEnabled(ecs::World& world) noexcept {
  const auto* context = world.TryReadResource<Context>();
  return context != nullptr && context->input_enabled;
}

[[nodiscard]] auto TryGetEntity([[maybe_unused]] ecs::World& world,
                                [[maybe_unused]] SDL_WindowID window_id)
    -> std::optional<ecs::Entity> {
#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
  const auto* window_map = world.TryReadResource<window::WindowMap>();
  if (window_map == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  const window::WindowMap::Entry* entry =
      window_map->TryGetByWindowId(window_id);
  if (entry == nullptr) [[unlikely]] {
    return std::nullopt;
  }

  return entry->entity;
#else
  return std::nullopt;
#endif
}

[[nodiscard]] constexpr uint32_t DecodeUtf8Codepoint(std::string_view text,
                                                     size_t& offset) noexcept {
  if (offset >= text.size()) {
    return 0;
  }

  const auto first = static_cast<unsigned char>(text[offset]);
  if (first < 0x80U) {
    ++offset;
    return first;
  }

  size_t length = 0;
  uint32_t codepoint = 0;
  if ((first & 0xE0U) == 0xC0U) {
    length = 2;
    codepoint = first & 0x1FU;
  } else if ((first & 0xF0U) == 0xE0U) {
    length = 3;
    codepoint = first & 0x0FU;
  } else if ((first & 0xF8U) == 0xF0U) {
    length = 4;
    codepoint = first & 0x07U;
  } else {
    ++offset;
    return 0;
  }

  if (offset + length > text.size()) {
    offset = text.size();
    return 0;
  }

  for (size_t i = 1; i < length; ++i) {
    const auto byte = static_cast<unsigned char>(text[offset + i]);
    if ((byte & 0xC0U) != 0x80U) {
      offset += length;
      return 0;
    }
    codepoint = (codepoint << 6U) | static_cast<uint32_t>(byte & 0x3FU);
  }

  offset += length;
  return codepoint;
}

void HandleKeyboardEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const SDL_KeyboardEvent& key_event = event.key;
  const auto entity = TryGetEntity(world, key_event.windowID);
  if (!entity.has_value()) [[unlikely]] {
    return;
  }

  world.WriteMessages<helios::input::KeyboardInputMsg>().Write({
      .entity = *entity,
      .key = KeyFromSdl(key_event.scancode),
      .state = ButtonStateFromSdl(key_event.down, key_event.repeat),
      .modifiers = ModifiersFromSdl(key_event.mod),
  });
}

void HandleTextInputEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const SDL_TextInputEvent& text_event = event.text;
  const auto entity = TryGetEntity(world, text_event.windowID);
  if (!entity.has_value() || text_event.text == nullptr) [[unlikely]] {
    return;
  }

  const std::string_view text{text_event.text};
  size_t offset = 0;
  auto writer = world.WriteMessages<helios::input::TextInputMsg>();
  while (offset < text.size()) {
    const uint32_t codepoint = DecodeUtf8Codepoint(text, offset);
    if (codepoint == 0) {
      continue;
    }
    writer.Write({.entity = *entity, .codepoint = codepoint});
  }
}

void HandleMouseButtonEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const SDL_MouseButtonEvent& button_event = event.button;
  const auto entity = TryGetEntity(world, button_event.windowID);
  if (!entity.has_value()) [[unlikely]] {
    return;
  }

  const auto mapped = MouseButtonFromSdl(button_event.button);
  if (!mapped.has_value()) {
    return;
  }

  world.WriteMessages<helios::input::MouseButtonInputMsg>().Write({
      .entity = *entity,
      .button = *mapped,
      .state = ButtonStateFromSdl(button_event.down, false),
      .modifiers = ModifiersFromSdl(SDL_GetModState()),
  });
}

void HandleMouseMotionEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const SDL_MouseMotionEvent& motion_event = event.motion;
  const auto entity = TryGetEntity(world, motion_event.windowID);
  if (!entity.has_value()) [[unlikely]] {
    return;
  }

  world.WriteMessages<helios::input::CursorMovedMsg>().Write({
      .entity = *entity,
      .x = static_cast<double>(motion_event.x),
      .y = static_cast<double>(motion_event.y),
  });

  if (motion_event.xrel == 0 && motion_event.yrel == 0) {
    return;
  }

  world.WriteMessages<helios::input::MouseMotionMsg>().Write({
      .entity = *entity,
      .delta_x = static_cast<double>(motion_event.xrel),
      .delta_y = static_cast<double>(motion_event.yrel),
  });
}

void HandleMouseWheelEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const SDL_MouseWheelEvent& wheel_event = event.wheel;
  const auto entity = TryGetEntity(world, wheel_event.windowID);
  if (!entity.has_value()) [[unlikely]] {
    return;
  }

  float scroll_x = wheel_event.x;
  float scroll_y = wheel_event.y;
  if (wheel_event.direction == SDL_MOUSEWHEEL_FLIPPED) {
    scroll_x = -scroll_x;
    scroll_y = -scroll_y;
  }

  world.WriteMessages<helios::input::MouseWheelMsg>().Write({
      .entity = *entity,
      .x = static_cast<double>(scroll_x),
      .y = static_cast<double>(scroll_y),
  });
}

[[nodiscard]] int FindPenSlot(const PenCache& cache,
                              uint32_t instance_id) noexcept {
  for (size_t index = 0; index < cache.slots.size(); ++index) {
    const PenSlotCache& slot = cache.slots[index];
    if (slot.connected && slot.instance_id == instance_id) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

[[nodiscard]] int AllocatePenSlot(PenCache& cache,
                                  uint32_t instance_id) noexcept {
  const int existing = FindPenSlot(cache, instance_id);
  if (existing >= 0) {
    return existing;
  }

  for (size_t index = 0; index < cache.slots.size(); ++index) {
    PenSlotCache& slot = cache.slots[index];
    if (!slot.connected) {
      slot.instance_id = instance_id;
      slot.connected = true;
      return static_cast<int>(index);
    }
  }
  return -1;
}

[[nodiscard]] ecs::Entity PenEntity(ecs::World& world, SDL_WindowID window_id) {
  return TryGetEntity(world, window_id).value_or(ecs::Entity{});
}

[[nodiscard]] int EnsurePenSlot(PenCache& cache, ecs::World& world,
                                SDL_PenID which, SDL_WindowID window_id) {
  const auto instance_id = static_cast<uint32_t>(which);
  const bool already_connected = FindPenSlot(cache, instance_id) >= 0;
  const int slot_id = AllocatePenSlot(cache, instance_id);
  if (slot_id < 0) {
    return -1;
  }
  if (!already_connected) {
    world.WriteMessages<helios::input::PenProximityMsg>().Write({
        .entity = PenEntity(world, window_id),
        .id = slot_id,
        .device_type = PenDeviceTypeFromSdl(SDL_GetPenDeviceType(which)),
        .in_proximity = true,
    });
  }
  return slot_id;
}

void HandlePenProximityEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  auto* cache = world.TryWriteResource<PenCache>();
  if (cache == nullptr) [[unlikely]] {
    return;
  }

  const SDL_PenProximityEvent& proximity = event.pproximity;
  const auto instance_id = static_cast<uint32_t>(proximity.which);
  const bool in_proximity = event.type == SDL_EVENT_PEN_PROXIMITY_IN;
  int slot_id = -1;
  if (in_proximity) {
    slot_id = AllocatePenSlot(*cache, instance_id);
  } else {
    slot_id = FindPenSlot(*cache, instance_id);
    if (slot_id >= 0) {
      cache->slots[static_cast<size_t>(slot_id)] = {};
    }
  }
  if (slot_id < 0) {
    return;
  }

  world.WriteMessages<helios::input::PenProximityMsg>().Write({
      .entity = PenEntity(world, proximity.windowID),
      .id = slot_id,
      .device_type =
          PenDeviceTypeFromSdl(SDL_GetPenDeviceType(proximity.which)),
      .in_proximity = in_proximity,
  });
}

void HandlePenTouchEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  auto* cache = world.TryWriteResource<PenCache>();
  if (cache == nullptr) [[unlikely]] {
    return;
  }

  const SDL_PenTouchEvent& touch = event.ptouch;
  const int slot_id = EnsurePenSlot(*cache, world, touch.which, touch.windowID);
  if (slot_id < 0) {
    return;
  }

  world.WriteMessages<helios::input::PenTouchMsg>().Write({
      .entity = PenEntity(world, touch.windowID),
      .x = static_cast<double>(touch.x),
      .y = static_cast<double>(touch.y),
      .id = slot_id,
      .down = touch.down,
      .eraser = touch.eraser,
  });
}

void HandlePenButtonEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  auto* cache = world.TryWriteResource<PenCache>();
  if (cache == nullptr) [[unlikely]] {
    return;
  }

  const SDL_PenButtonEvent& button_event = event.pbutton;
  const auto mapped = PenButtonFromSdl(button_event.button);
  if (!mapped.has_value()) {
    return;
  }

  const int slot_id =
      EnsurePenSlot(*cache, world, button_event.which, button_event.windowID);
  if (slot_id < 0) {
    return;
  }

  world.WriteMessages<helios::input::PenButtonInputMsg>().Write({
      .entity = PenEntity(world, button_event.windowID),
      .id = slot_id,
      .button = *mapped,
      .state = ButtonStateFromSdl(button_event.down, false),
  });
}

void HandlePenMotionEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  auto* cache = world.TryWriteResource<PenCache>();
  if (cache == nullptr) [[unlikely]] {
    return;
  }

  const SDL_PenMotionEvent& motion = event.pmotion;
  const int slot_id =
      EnsurePenSlot(*cache, world, motion.which, motion.windowID);
  if (slot_id < 0) {
    return;
  }

  world.WriteMessages<helios::input::PenMovedMsg>().Write({
      .entity = PenEntity(world, motion.windowID),
      .x = static_cast<double>(motion.x),
      .y = static_cast<double>(motion.y),
      .id = slot_id,
  });
}

void HandlePenAxisEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  auto* cache = world.TryWriteResource<PenCache>();
  if (cache == nullptr) [[unlikely]] {
    return;
  }

  const SDL_PenAxisEvent& axis_event = event.paxis;
  const auto mapped = PenAxisFromSdl(axis_event.axis);
  if (!mapped.has_value()) {
    return;
  }

  const int slot_id =
      EnsurePenSlot(*cache, world, axis_event.which, axis_event.windowID);
  if (slot_id < 0) {
    return;
  }

  world.WriteMessages<helios::input::PenAxisChangedMsg>().Write({
      .entity = PenEntity(world, axis_event.windowID),
      .x = static_cast<double>(axis_event.x),
      .y = static_cast<double>(axis_event.y),
      .value = axis_event.value,
      .id = slot_id,
      .axis = *mapped,
  });
}

void HandleInputEvent(const SDL_Event& event, ecs::World& world) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      HandleKeyboardEvent(event, world);
      break;
    case SDL_EVENT_TEXT_INPUT:
      HandleTextInputEvent(event, world);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      HandleMouseButtonEvent(event, world);
      break;
    case SDL_EVENT_MOUSE_MOTION:
      HandleMouseMotionEvent(event, world);
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      HandleMouseWheelEvent(event, world);
      break;
    case SDL_EVENT_PEN_PROXIMITY_IN:
    case SDL_EVENT_PEN_PROXIMITY_OUT:
      HandlePenProximityEvent(event, world);
      break;
    case SDL_EVENT_PEN_DOWN:
    case SDL_EVENT_PEN_UP:
      HandlePenTouchEvent(event, world);
      break;
    case SDL_EVENT_PEN_BUTTON_DOWN:
    case SDL_EVENT_PEN_BUTTON_UP:
      HandlePenButtonEvent(event, world);
      break;
    case SDL_EVENT_PEN_MOTION:
      HandlePenMotionEvent(event, world);
      break;
    case SDL_EVENT_PEN_AXIS:
      HandlePenAxisEvent(event, world);
      break;
    case SDL_EVENT_JOYSTICK_ADDED:
    case SDL_EVENT_GAMEPAD_ADDED: {
      auto* cache = world.TryWriteResource<GamepadCache>();
      if (cache != nullptr) {
        cache->pending_added.push_back(
            static_cast<uint32_t>(event.jdevice.which));
      }
      break;
    }
    case SDL_EVENT_JOYSTICK_REMOVED:
    case SDL_EVENT_GAMEPAD_REMOVED: {
      auto* cache = world.TryWriteResource<GamepadCache>();
      if (cache != nullptr) {
        cache->pending_removed.push_back(
            static_cast<uint32_t>(event.jdevice.which));
      }
      break;
    }
    case SDL_EVENT_GAMEPAD_REMAPPED: {
      auto* cache = world.TryWriteResource<GamepadCache>();
      if (cache != nullptr) {
        cache->pending_remapped.push_back(
            static_cast<uint32_t>(event.gdevice.which));
      }
      break;
    }
    case SDL_EVENT_GAMEPAD_SENSOR_UPDATE: {
      if (!InputEnabled(world)) {
        break;
      }
      const auto* cache = world.TryReadResource<GamepadCache>();
      if (cache == nullptr) {
        break;
      }
      int slot_id = -1;
      const auto instance_id = static_cast<uint32_t>(event.gsensor.which);
      for (size_t index = 0; index < cache->slots.size(); ++index) {
        const GamepadSlotCache& slot = cache->slots[index];
        if (slot.connected && slot.instance_id == instance_id) {
          slot_id = static_cast<int>(index);
          break;
        }
      }
      if (slot_id < 0) {
        break;
      }
      helios::input::GamepadSensor sensor = helios::input::GamepadSensor::kGyro;
      if (event.gsensor.sensor == SDL_SENSOR_ACCEL) {
        sensor = helios::input::GamepadSensor::kAccel;
      } else if (event.gsensor.sensor != SDL_SENSOR_GYRO) {
        break;
      }
      world.WriteMessages<helios::input::GamepadSensorUpdateMsg>().Write({
          .id = slot_id,
          .sensor = sensor,
          .value = {event.gsensor.data[0], event.gsensor.data[1],
                    event.gsensor.data[2]},
      });
      break;
    }
    default:
      break;
  }
}

}  // namespace

void RegisterEventHandlers(EventDispatcher& dispatcher) {
  dispatcher.Register(HandleInputEvent);
}

}  // namespace helios::sdl3::input
