#include <pch.hpp>

#include <helios/glfw/systems/input.hpp>

#ifdef HELIOS_MODULE_INPUT_AVAILABLE

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/glfw/details/input_map.hpp>
#include <helios/input/components.hpp>
#include <helios/input/joystick.hpp>
#include <helios/input/messages.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/params.hpp>
#include <helios/input/resources.hpp>
#include <helios/window/components.hpp>
#include <helios/window/properties.hpp>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <utility>

namespace helios::glfw {

namespace {

[[nodiscard]] GLFWcursor*& FindOrInsertCustom(CursorCache& cache,
                                              ecs::Entity entity) {
  const auto it = std::ranges::find_if(
      cache.custom, [entity](const CursorCache::CustomEntry& entry) {
        return entry.entity == entity;
      });
  if (it != cache.custom.end()) {
    return it->cursor;
  }

  cache.custom.push_back({.entity = entity, .cursor = nullptr});
  return cache.custom.back().cursor;
}

void EraseCustom(CursorCache& cache, ecs::Entity entity) {
  const auto it = std::ranges::find_if(
      cache.custom, [entity](const CursorCache::CustomEntry& entry) {
        return entry.entity == entity;
      });
  if (it == cache.custom.end()) [[unlikely]] {
    return;
  }

  if (it->cursor != nullptr) [[likely]] {
    glfwDestroyCursor(it->cursor);
  }

  cache.custom.erase(it);
}

[[nodiscard]] GLFWcursor* EnsureStandardCursor(CursorCache& cache,
                                               input::CursorIcon icon) {
  const auto index = static_cast<size_t>(icon);
  if (index >= cache.standard.size()) [[unlikely]] {
    return nullptr;
  }

  if (cache.standard[index] == nullptr) {
    cache.standard[index] = glfwCreateStandardCursor(GlfwFromCursorIcon(icon));
  }

  return cache.standard[index];
}

[[nodiscard]] std::string JoystickGuid(int jid) {
  const char* guid = glfwGetJoystickGUID(jid);
  return guid != nullptr ? guid : "";
}

[[nodiscard]] std::string JoystickName(int jid) {
  const char* name = glfwGetJoystickName(jid);
  return name != nullptr ? name : "";
}

void DisconnectGamepad(int jid, GamepadSlotCache& slot,
                       input::GamepadWriters& writers) {
  if (!slot.connected) {
    return;
  }
  writers.connection.Write(
      {.id = jid, .connected = false, .name = std::move(slot.name)});
  slot = {};
}

void DisconnectJoystick(int jid, JoystickSlotCache& slot,
                        input::JoystickWriters& writers) {
  if (!slot.connected) {
    return;
  }
  writers.connection.Write(
      {.name = std::move(slot.name), .id = jid, .connected = false});
  slot = {};
}

void EmitGamepadState(int jid, const GLFWgamepadstate& state,
                      GamepadSlotCache& slot, input::GamepadWriters& writers,
                      bool seed) {
  for (size_t i = 0; i < slot.buttons.size(); ++i) {
    const unsigned char next = state.buttons[i];
    if (!seed && next == slot.buttons[i]) {
      continue;
    }
    slot.buttons[i] = next;
    if (seed && next != GLFW_PRESS) {
      continue;
    }
    writers.buttons.Write({
        .id = jid,
        .button = GamepadButtonFromGlfw(static_cast<int>(i)),
        .state = next == GLFW_PRESS ? input::ButtonState::kPressed
                                    : input::ButtonState::kReleased,
    });
  }

  for (size_t i = 0; i < slot.axes.size(); ++i) {
    const float next = state.axes[i];
    if (!seed && next == slot.axes[i]) {
      continue;
    }
    slot.axes[i] = next;
    writers.axes.Write({
        .id = jid,
        .axis = GamepadAxisFromGlfw(static_cast<int>(i)),
        .value = next,
    });
  }
}

void ConnectGamepad(int jid, GamepadSlotCache& slot,
                    input::GamepadWriters& writers) {
  const char* name = glfwGetGamepadName(jid);
  slot = {};
  slot.name = name != nullptr ? name : "";
  slot.connected = true;
  writers.connection.Write({
      .id = jid,
      .connected = true,
      .name = slot.name,
      .guid = JoystickGuid(jid),
  });

  GLFWgamepadstate state{};
  if (glfwGetGamepadState(jid, &state) == GLFW_TRUE) {
    EmitGamepadState(jid, state, slot, writers, true);
  }
}

[[nodiscard]] uint8_t ClampCount(int count, size_t max_count) noexcept {
  if (count <= 0) {
    return 0;
  }
  return static_cast<uint8_t>(std::min(static_cast<size_t>(count), max_count));
}

void EmitJoystickState(int jid, JoystickSlotCache& slot,
                       input::JoystickWriters& writers, bool seed) {
  int axis_count = 0;
  const float* axes = glfwGetJoystickAxes(jid, &axis_count);
  slot.axis_count = ClampCount(axis_count, input::Joystick::kMaxAxes);
  for (uint8_t i = 0; i < slot.axis_count; ++i) {
    const float next = axes != nullptr ? axes[i] : 0.0F;
    if (!seed && next == slot.axes[i]) {
      continue;
    }
    slot.axes[i] = next;
    writers.axes.Write({.id = jid, .axis = i, .value = next});
  }

  int button_count = 0;
  const unsigned char* buttons = glfwGetJoystickButtons(jid, &button_count);
  slot.button_count = ClampCount(button_count, input::Joystick::kMaxButtons);
  for (uint8_t i = 0; i < slot.button_count; ++i) {
    const unsigned char next = buttons != nullptr ? buttons[i] : GLFW_RELEASE;
    if (!seed && next == slot.buttons[i]) {
      continue;
    }
    slot.buttons[i] = next;
    if (seed && next != GLFW_PRESS) {
      continue;
    }
    writers.buttons.Write({
        .id = jid,
        .button = i,
        .state = next == GLFW_PRESS ? input::ButtonState::kPressed
                                    : input::ButtonState::kReleased,
    });
  }

  int hat_count = 0;
  const unsigned char* hats = glfwGetJoystickHats(jid, &hat_count);
  slot.hat_count = ClampCount(hat_count, input::Joystick::kMaxHats);
  for (uint8_t i = 0; i < slot.hat_count; ++i) {
    const auto next = static_cast<input::JoystickHat>(
        hats != nullptr ? hats[i] : GLFW_HAT_CENTERED);
    if (!seed && static_cast<unsigned char>(next) == slot.hats[i]) {
      continue;
    }
    slot.hats[i] = static_cast<unsigned char>(next);
    writers.hats.Write({.id = jid, .hat = i, .value = next});
  }
}

void ConnectJoystick(int jid, JoystickSlotCache& slot,
                     input::JoystickWriters& writers) {
  slot = {};
  slot.name = JoystickName(jid);
  slot.guid = JoystickGuid(jid);
  slot.connected = true;
  EmitJoystickState(jid, slot, writers, true);
  writers.connection.Write({
      .name = slot.name,
      .guid = slot.guid,
      .id = jid,
      .axis_count = slot.axis_count,
      .button_count = slot.button_count,
      .hat_count = slot.hat_count,
      .connected = true,
  });
}

}  // namespace

void DestroyCursorCache(CursorCache& cache) {
  for (GLFWcursor*& cursor : cache.standard) {
    if (cursor != nullptr) [[likely]] {
      glfwDestroyCursor(cursor);
      cursor = nullptr;
    }
  }

  for (CursorCache::CustomEntry& entry : cache.custom) {
    if (entry.cursor != nullptr) [[likely]] {
      glfwDestroyCursor(entry.cursor);
      entry.cursor = nullptr;
    }
  }
  cache.custom.clear();
}

void ApplyGamepadMappings::operator()(
    ecs::Res<const Context> context,
    ecs::Res<input::GamepadMappings> mappings) const {
  if (!context->initialized || !context->input_enabled || !mappings->dirty)
      [[unlikely]] {
    return;
  }

  for (const std::string& line : mappings->pending_lines) {
    glfwUpdateGamepadMappings(line.c_str());
  }
  for (const std::string& path : mappings->pending_files) {
    std::ifstream file{path};
    if (!file) {
      continue;
    }
    const std::string contents{std::istreambuf_iterator<char>{file},
                               std::istreambuf_iterator<char>{}};
    if (!contents.empty()) {
      glfwUpdateGamepadMappings(contents.c_str());
    }
  }
  mappings->ClearPending();
}

void PollGamepads::operator()(ecs::Res<const Context> context,
                              input::GamepadWriters gamepads,
                              input::JoystickWriters sticks,
                              ecs::Res<GamepadCache> cache) const {
  if (!context->initialized || !context->input_enabled) [[unlikely]] {
    return;
  }

  for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
    const auto slot_index = static_cast<size_t>(jid);
    if (slot_index >= cache->slots.size()) {
      continue;
    }

    GamepadSlotCache& pad = cache->slots[slot_index];
    JoystickSlotCache& stick = cache->joysticks[slot_index];
    const bool present = glfwJoystickPresent(jid) == GLFW_TRUE;
    const bool is_gamepad = present && glfwJoystickIsGamepad(jid) == GLFW_TRUE;
    const bool is_joystick = present && !is_gamepad;

    if (is_gamepad) {
      DisconnectJoystick(jid, stick, sticks);
      if (!pad.connected) {
        ConnectGamepad(jid, pad, gamepads);
        continue;
      }
      GLFWgamepadstate state{};
      if (glfwGetGamepadState(jid, &state) == GLFW_TRUE) {
        EmitGamepadState(jid, state, pad, gamepads, false);
      }
      continue;
    }

    DisconnectGamepad(jid, pad, gamepads);
    if (is_joystick) {
      if (!stick.connected) {
        ConnectJoystick(jid, stick, sticks);
        continue;
      }
      EmitJoystickState(jid, stick, sticks, false);
      continue;
    }

    DisconnectJoystick(jid, stick, sticks);
  }
}

void ApplyGamepadOutputs::operator()(ecs::Res<const Context> context,
                                     ecs::Res<input::Gamepads> gamepads) const {
  if (!context->initialized || !context->input_enabled) [[unlikely]] {
    return;
  }

  for (input::Gamepad& pad : gamepads->pads) {
    pad.ClearDirty();
  }
}

void ApplyCursors::operator()(
    ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
    ecs::Res<CursorCache> cache,
    ecs::Query<window::Window&, input::Cursor&> cursors) const {
  if (!context->initialized || !context->input_enabled) [[unlikely]] {
    return;
  }

  for (auto&& [entity, window, cursor] : cursors.WithEntity()) {
    if (!cursor.dirty) {
      continue;
    }

    NativeWindows::Entry* entry = native->TryGet(entity);
    if (entry == nullptr || entry->native.window == nullptr) [[unlikely]] {
      continue;
    }

    GLFWwindow* glfw_window = entry->native.window;
    if (cursor.custom.has_value()) {
      input::CursorImage& image = *cursor.custom;
      if (image.width == 0 || image.height == 0 ||
          image.rgba.size() <
              static_cast<size_t>(image.width) * image.height * 4U) {
        cursor.dirty = false;
        continue;
      }

      GLFWimage glfw_image{
          .width = static_cast<int>(image.width),
          .height = static_cast<int>(image.height),
          .pixels = reinterpret_cast<unsigned char*>(image.rgba.data()),
      };
      GLFWcursor* created =
          glfwCreateCursor(&glfw_image, image.hotspot_x, image.hotspot_y);
      if (created == nullptr) [[unlikely]] {
        cursor.dirty = false;
        continue;
      }

      GLFWcursor*& custom = FindOrInsertCustom(*cache, entity);
      if (custom != nullptr) [[likely]] {
        glfwDestroyCursor(custom);
      }
      custom = created;
      glfwSetCursor(glfw_window, custom);
    } else {
      EraseCustom(*cache, entity);
      GLFWcursor* standard = EnsureStandardCursor(*cache, cursor.icon);
      glfwSetCursor(glfw_window, standard);
    }

    cursor.dirty = false;
  }
}

void ApplyRawMouseMotion::operator()(
    ecs::Res<const Context> context, ecs::Res<NativeWindows> native,
    ecs::Res<const input::Settings> settings,
    ecs::Query<const window::Window&> windows) const {
  if (!context->initialized || !context->input_enabled) [[unlikely]] {
    return;
  }

  if (!glfwRawMouseMotionSupported()) [[unlikely]] {
    return;
  }

  const int mode = settings->raw_mouse_motion ? GLFW_TRUE : GLFW_FALSE;

  for (auto&& [entity, window] : windows.WithEntity()) {
    if (window.properties.cursor_mode != window::CursorMode::kDisabled) {
      continue;
    }

    NativeWindows::Entry* entry = native->TryGet(entity);
    if (entry == nullptr || entry->native.window == nullptr) [[unlikely]] {
      continue;
    }

    if (glfwGetInputMode(entry->native.window, GLFW_RAW_MOUSE_MOTION) == mode) {
      continue;
    }

    glfwSetInputMode(entry->native.window, GLFW_RAW_MOUSE_MOTION, mode);
  }
}

}  // namespace helios::glfw

#endif  // HELIOS_MODULE_INPUT_AVAILABLE
