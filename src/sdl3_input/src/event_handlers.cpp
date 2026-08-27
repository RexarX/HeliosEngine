#include <pch.hpp>

#include <helios/sdl3/input/event_handlers.hpp>

#include <details/input_map.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/ids.hpp>
#include <helios/input/keyboard.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/pen.hpp>
#include <helios/input/sensor.hpp>
#include <helios/input/touch.hpp>
#include <helios/sdl3/event_dispatcher.hpp>
#include <helios/sdl3/input/event_handlers.hpp>
#include <helios/sdl3/input/state.hpp>
#include <helios/sdl3/input/systems/poll_gamepads.hpp>
#include <helios/sdl3/input/systems/poll_sensors.hpp>

#ifdef HELIOS_MODULE_SDL3_WINDOW_AVAILABLE
#include <helios/sdl3/window/window_map.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_touch.h>
#include <SDL3/SDL_video.h>

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

void HandleTextEditingEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const SDL_TextEditingEvent& edit_event = event.edit;
  const auto entity = TryGetEntity(world, edit_event.windowID);
  if (!entity.has_value()) [[unlikely]] {
    return;
  }

  world.WriteMessages<helios::input::TextEditingMsg>().Write({
      .entity = *entity,
      .composition = edit_event.text != nullptr ? edit_event.text : "",
      .start = edit_event.start,
      .length = edit_event.length,
  });
}

void HandleTextEditingCandidatesEvent(const SDL_Event& event,
                                      ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const SDL_TextEditingCandidatesEvent& candidates_event =
      event.edit_candidates;
  const auto entity = TryGetEntity(world, candidates_event.windowID);
  if (!entity.has_value()) [[unlikely]] {
    return;
  }

  std::vector<std::string> candidates;
  if (candidates_event.candidates != nullptr &&
      candidates_event.num_candidates > 0) {
    candidates.reserve(static_cast<size_t>(candidates_event.num_candidates));
    for (Sint32 index = 0; index < candidates_event.num_candidates; ++index) {
      const char* candidate = candidates_event.candidates[index];
      candidates.emplace_back(candidate != nullptr ? candidate : "");
    }
  }

  world.WriteMessages<helios::input::TextEditingCandidatesMsg>().Write({
      .entity = *entity,
      .candidates = std::move(candidates),
      .selected =
          candidates_event.selected_candidate >= 0
              ? std::optional<helios::input::TextCandidateIndex>{static_cast<
                    helios::input::TextCandidateIndex>(
                    candidates_event.selected_candidate)}
              : std::nullopt,
      .horizontal = candidates_event.horizontal,
  });
}

void HandleKeyboardDeviceEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const char* name = SDL_GetKeyboardNameForID(event.kdevice.which);
  world.WriteMessages<helios::input::KeyboardConnectionMsg>().Write({
      .name = name != nullptr ? name : "",
      .id = static_cast<helios::input::KeyboardId>(event.kdevice.which),
      .connected = event.type == SDL_EVENT_KEYBOARD_ADDED,
  });
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

void HandleMouseDeviceEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const char* name = SDL_GetMouseNameForID(event.mdevice.which);
  world.WriteMessages<helios::input::MouseConnectionMsg>().Write({
      .name = name != nullptr ? name : "",
      .id = static_cast<helios::input::MouseId>(event.mdevice.which),
      .connected = event.type == SDL_EVENT_MOUSE_ADDED,
  });
}

[[nodiscard]] auto FindPenSlot(const PenCache& cache,
                               uint32_t instance_id) noexcept
    -> std::optional<helios::input::PenId> {
  for (size_t index = 0; index < cache.slots.size(); ++index) {
    const PenSlotCache& slot = cache.slots[index];
    if (slot.connected && slot.instance_id == instance_id) {
      return static_cast<helios::input::PenId>(index);
    }
  }
  return std::nullopt;
}

[[nodiscard]] auto AllocatePenSlot(PenCache& cache,
                                   uint32_t instance_id) noexcept
    -> std::optional<helios::input::PenId> {
  if (const auto existing = FindPenSlot(cache, instance_id)) {
    return existing;
  }

  for (size_t index = 0; index < cache.slots.size(); ++index) {
    PenSlotCache& slot = cache.slots[index];
    if (!slot.connected) {
      slot.instance_id = instance_id;
      slot.connected = true;
      return static_cast<helios::input::PenId>(index);
    }
  }
  return std::nullopt;
}

[[nodiscard]] ecs::Entity PenEntity(ecs::World& world, SDL_WindowID window_id) {
  return TryGetEntity(world, window_id).value_or(ecs::Entity{});
}

[[nodiscard]] auto EnsurePenSlot(PenCache& cache, ecs::World& world,
                                 SDL_PenID which, SDL_WindowID window_id)
    -> std::optional<helios::input::PenId> {
  const auto instance_id = static_cast<uint32_t>(which);
  const bool already_connected = FindPenSlot(cache, instance_id).has_value();
  const auto slot_id = AllocatePenSlot(cache, instance_id);
  if (!slot_id.has_value()) {
    return std::nullopt;
  }
  if (!already_connected) {
    world.WriteMessages<helios::input::PenProximityMsg>().Write({
        .entity = PenEntity(world, window_id),
        .id = *slot_id,
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
  std::optional<helios::input::PenId> slot_id;
  if (in_proximity) {
    slot_id = AllocatePenSlot(*cache, instance_id);
  } else {
    slot_id = FindPenSlot(*cache, instance_id);
    if (slot_id.has_value()) {
      cache->slots[*slot_id] = {};
    }
  }
  if (!slot_id.has_value()) {
    return;
  }

  world.WriteMessages<helios::input::PenProximityMsg>().Write({
      .entity = PenEntity(world, proximity.windowID),
      .id = *slot_id,
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
  const auto slot_id =
      EnsurePenSlot(*cache, world, touch.which, touch.windowID);
  if (!slot_id.has_value()) {
    return;
  }

  world.WriteMessages<helios::input::PenTouchMsg>().Write({
      .entity = PenEntity(world, touch.windowID),
      .x = static_cast<double>(touch.x),
      .y = static_cast<double>(touch.y),
      .id = *slot_id,
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

  const auto slot_id =
      EnsurePenSlot(*cache, world, button_event.which, button_event.windowID);
  if (!slot_id.has_value()) {
    return;
  }

  world.WriteMessages<helios::input::PenButtonInputMsg>().Write({
      .entity = PenEntity(world, button_event.windowID),
      .id = *slot_id,
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
  const auto slot_id =
      EnsurePenSlot(*cache, world, motion.which, motion.windowID);
  if (!slot_id.has_value()) {
    return;
  }

  world.WriteMessages<helios::input::PenMovedMsg>().Write({
      .entity = PenEntity(world, motion.windowID),
      .x = static_cast<double>(motion.x),
      .y = static_cast<double>(motion.y),
      .id = *slot_id,
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

  const auto slot_id =
      EnsurePenSlot(*cache, world, axis_event.which, axis_event.windowID);
  if (!slot_id.has_value()) {
    return;
  }

  world.WriteMessages<helios::input::PenAxisChangedMsg>().Write({
      .entity = PenEntity(world, axis_event.windowID),
      .x = static_cast<double>(axis_event.x),
      .y = static_cast<double>(axis_event.y),
      .value = axis_event.value,
      .id = *slot_id,
      .axis = *mapped,
  });
}

[[nodiscard]] auto FindTouchSlot(const TouchCache& cache, uint64_t touch_id,
                                 uint64_t finger_id) noexcept
    -> std::optional<helios::input::TouchId> {
  for (size_t index = 0; index < cache.slots.size(); ++index) {
    const TouchSlotCache& slot = cache.slots[index];
    if (slot.connected && slot.touch_id == touch_id &&
        slot.finger_id == finger_id) {
      return static_cast<helios::input::TouchId>(index);
    }
  }
  return std::nullopt;
}

[[nodiscard]] auto AllocateTouchSlot(TouchCache& cache, uint64_t touch_id,
                                     uint64_t finger_id) noexcept
    -> std::optional<helios::input::TouchId> {
  if (const auto existing = FindTouchSlot(cache, touch_id, finger_id)) {
    return existing;
  }

  for (size_t index = 0; index < cache.slots.size(); ++index) {
    TouchSlotCache& slot = cache.slots[index];
    if (!slot.connected) {
      slot.touch_id = touch_id;
      slot.finger_id = finger_id;
      slot.connected = true;
      return static_cast<helios::input::TouchId>(index);
    }
  }
  return std::nullopt;
}

void ConvertTouchCoords(SDL_WindowID window_id, float x, float y, float dx,
                        float dy, double& out_x, double& out_y, double& out_dx,
                        double& out_dy) noexcept {
  double scale_x = 1.0;
  double scale_y = 1.0;
  SDL_Window* window = SDL_GetWindowFromID(window_id);
  if (window != nullptr) {
    int width = 0;
    int height = 0;
    if (SDL_GetWindowSize(window, &width, &height)) {
      if (width > 0) {
        scale_x = static_cast<double>(width);
      }
      if (height > 0) {
        scale_y = static_cast<double>(height);
      }
    }
  }
  out_x = static_cast<double>(x) * scale_x;
  out_y = static_cast<double>(y) * scale_y;
  out_dx = static_cast<double>(dx) * scale_x;
  out_dy = static_cast<double>(dy) * scale_y;
}

void HandleTouchEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  auto* cache = world.TryWriteResource<TouchCache>();
  if (cache == nullptr) [[unlikely]] {
    return;
  }

  const SDL_TouchFingerEvent& finger = event.tfinger;
  const auto touch_id = static_cast<uint64_t>(finger.touchID);
  const auto finger_id = static_cast<uint64_t>(finger.fingerID);
  const bool ended = event.type == SDL_EVENT_FINGER_UP ||
                     event.type == SDL_EVENT_FINGER_CANCELED;
  auto slot_id = ended ? FindTouchSlot(*cache, touch_id, finger_id)
                       : AllocateTouchSlot(*cache, touch_id, finger_id);
  if (!slot_id.has_value() && ended) {
    slot_id = AllocateTouchSlot(*cache, touch_id, finger_id);
  }
  if (!slot_id.has_value()) {
    return;
  }

  helios::input::TouchPhase phase = helios::input::TouchPhase::kMoved;
  if (event.type == SDL_EVENT_FINGER_DOWN) {
    phase = helios::input::TouchPhase::kStarted;
  } else if (event.type == SDL_EVENT_FINGER_UP) {
    phase = helios::input::TouchPhase::kEnded;
  } else if (event.type == SDL_EVENT_FINGER_CANCELED) {
    phase = helios::input::TouchPhase::kCanceled;
  }

  double x = 0.0;
  double y = 0.0;
  double dx = 0.0;
  double dy = 0.0;
  ConvertTouchCoords(finger.windowID, finger.x, finger.y, finger.dx, finger.dy,
                     x, y, dx, dy);

  world.WriteMessages<helios::input::TouchInputMsg>().Write({
      .entity = TryGetEntity(world, finger.windowID).value_or(ecs::Entity{}),
      .x = x,
      .y = y,
      .dx = dx,
      .dy = dy,
      .pressure = finger.pressure,
      .id = *slot_id,
      .phase = phase,
      .device_type =
          TouchDeviceTypeFromSdl(SDL_GetTouchDeviceType(finger.touchID)),
  });

  if (ended) {
    cache->slots[*slot_id] = {};
  }
}

void HandleSensorUpdateEvent(const SDL_Event& event, ecs::World& world) {
  if (!InputEnabled(world)) [[unlikely]] {
    return;
  }

  const auto* cache = world.TryReadResource<SensorCache>();
  if (cache == nullptr) {
    return;
  }

  std::optional<helios::input::SensorId> slot_id;
  const auto instance_id = static_cast<uint32_t>(event.sensor.which);
  for (size_t index = 0; index < cache->slots.size(); ++index) {
    const SensorSlotCache& slot = cache->slots[index];
    if (slot.connected && slot.instance_id == instance_id) {
      slot_id = static_cast<helios::input::SensorId>(index);
      break;
    }
  }
  if (!slot_id.has_value()) {
    return;
  }

  const SensorSlotCache& slot = cache->slots[*slot_id];
  world.WriteMessages<helios::input::SensorUpdateMsg>().Write({
      .value = {event.sensor.data[0], event.sensor.data[1],
                event.sensor.data[2]},
      .id = *slot_id,
      .type = slot.type,
  });
}

void HandleInputEvent(const SDL_Event& event, ecs::World& world) {
  switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      HandleKeyboardEvent(event, world);
      break;
    case SDL_EVENT_TEXT_EDITING:
      HandleTextEditingEvent(event, world);
      break;
    case SDL_EVENT_TEXT_EDITING_CANDIDATES:
      HandleTextEditingCandidatesEvent(event, world);
      break;
    case SDL_EVENT_TEXT_INPUT:
      HandleTextInputEvent(event, world);
      break;
    case SDL_EVENT_KEYBOARD_ADDED:
    case SDL_EVENT_KEYBOARD_REMOVED:
      HandleKeyboardDeviceEvent(event, world);
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
    case SDL_EVENT_MOUSE_ADDED:
    case SDL_EVENT_MOUSE_REMOVED:
      HandleMouseDeviceEvent(event, world);
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
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_MOTION:
    case SDL_EVENT_FINGER_CANCELED:
      HandleTouchEvent(event, world);
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
      std::optional<helios::input::GamepadId> slot_id;
      const auto instance_id = static_cast<uint32_t>(event.gsensor.which);
      for (size_t index = 0; index < cache->slots.size(); ++index) {
        const GamepadSlotCache& slot = cache->slots[index];
        if (slot.connected && slot.instance_id == instance_id) {
          slot_id = static_cast<helios::input::GamepadId>(index);
          break;
        }
      }
      if (!slot_id.has_value()) {
        break;
      }
      helios::input::GamepadSensor sensor = helios::input::GamepadSensor::kGyro;
      if (event.gsensor.sensor == SDL_SENSOR_ACCEL) {
        sensor = helios::input::GamepadSensor::kAccel;
      } else if (event.gsensor.sensor != SDL_SENSOR_GYRO) {
        break;
      }
      world.WriteMessages<helios::input::GamepadSensorUpdateMsg>().Write({
          .id = *slot_id,
          .sensor = sensor,
          .value = {event.gsensor.data[0], event.gsensor.data[1],
                    event.gsensor.data[2]},
      });
      break;
    }
    case SDL_EVENT_SENSOR_UPDATE:
      HandleSensorUpdateEvent(event, world);
      break;
    default:
      break;
  }
}

}  // namespace

void RegisterEventHandlers(EventDispatcher& dispatcher) {
  dispatcher.Register(HandleInputEvent);
}

}  // namespace helios::sdl3::input
