#include <pch.hpp>

#include <helios/glfw/systems/input.hpp>

#ifdef HELIOS_MODULE_INPUT_AVAILABLE

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/system/system.hpp>
#include <helios/glfw/details/glfw_state.hpp>
#include <helios/glfw/details/input_map.hpp>
#include <helios/input/components.hpp>
#include <helios/input/messages.hpp>
#include <helios/input/mouse.hpp>
#include <helios/input/params.hpp>
#include <helios/input/resources.hpp>
#include <helios/window/components.hpp>
#include <helios/window/properties.hpp>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstddef>
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

namespace {

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

}  // namespace

void PollGamepads::operator()(ecs::Res<const Context> context,
                              input::GamepadWriters writers,
                              ecs::Res<GamepadCache> cache) const {
  if (!context->initialized || !context->input_enabled) [[unlikely]] {
    return;
  }

  for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
    const auto slot_index = static_cast<size_t>(jid);
    if (slot_index >= cache->slots.size()) {
      continue;
    }

    GamepadSlotCache& slot = cache->slots[slot_index];
    const bool present = glfwJoystickPresent(jid) == GLFW_TRUE;
    const bool is_gamepad = present && glfwJoystickIsGamepad(jid) == GLFW_TRUE;

    if (is_gamepad != slot.connected) {
      if (is_gamepad) {
        const char* name = glfwGetGamepadName(jid);
        slot.name = name != nullptr ? name : "";
        writers.connection.Write(
            {.id = jid, .connected = true, .name = slot.name});

        GLFWgamepadstate state{};
        if (glfwGetGamepadState(jid, &state) == GLFW_TRUE) {
          EmitGamepadState(jid, state, slot, writers, true);
        }
      } else {
        writers.connection.Write(
            {.id = jid, .connected = false, .name = std::move(slot.name)});
        slot = {};
      }
      slot.connected = is_gamepad;
      continue;
    }

    if (!is_gamepad) {
      continue;
    }

    GLFWgamepadstate state{};
    if (glfwGetGamepadState(jid, &state) != GLFW_TRUE) {
      continue;
    }

    EmitGamepadState(jid, state, slot, writers, false);
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

    glfwSetInputMode(entry->native.window, GLFW_RAW_MOUSE_MOTION, mode);
  }
}

}  // namespace helios::glfw

#endif  // HELIOS_MODULE_INPUT_AVAILABLE
