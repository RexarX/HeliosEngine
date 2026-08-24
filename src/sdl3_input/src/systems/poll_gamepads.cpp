#include <pch.hpp>

#include <helios/sdl3/input/systems/poll_gamepads.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/input/gamepad.hpp>
#include <helios/input/joystick.hpp>
#include <helios/log/log.hpp>
#include <helios/sdl3/input/details/input_map.hpp>

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>

namespace helios::sdl3::input {

namespace {

[[nodiscard]] std::string GuidToString(SDL_GUID guid) {
  char buffer[33] = {};
  SDL_GUIDToString(guid, buffer, static_cast<int>(sizeof(buffer)));
  return buffer;
}

[[nodiscard]] int FindSlotByInstance(const GamepadCache& cache,
                                     SDL_JoystickID instance_id) noexcept {
  const auto id = static_cast<uint32_t>(instance_id);
  for (size_t index = 0; index < cache.slots.size(); ++index) {
    const GamepadSlotCache& slot = cache.slots[index];
    if (slot.connected && slot.instance_id == id) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

[[nodiscard]] int ResolvePlayerSlot(SDL_JoystickID instance_id,
                                    SDL_Gamepad* gamepad) {
  int player_index = SDL_GetJoystickPlayerIndexForID(instance_id);
  if (player_index < 0 && gamepad != nullptr) {
    player_index = SDL_GetGamepadPlayerIndex(gamepad);
  }
  if (player_index < 0 ||
      player_index >= static_cast<int>(GamepadCache::kSlotCount)) {
    return -1;
  }
  return player_index;
}

[[nodiscard]] int AllocateSlot(GamepadCache& cache, SDL_JoystickID instance_id,
                               SDL_Gamepad* gamepad) {
  const int existing = FindSlotByInstance(cache, instance_id);
  if (existing >= 0) {
    return existing;
  }

  int slot_id = ResolvePlayerSlot(instance_id, gamepad);
  if (slot_id < 0 || cache.slots[static_cast<size_t>(slot_id)].connected) {
    slot_id = -1;
    for (size_t free_index = 0; free_index < cache.slots.size(); ++free_index) {
      if (!cache.slots[free_index].connected) {
        slot_id = static_cast<int>(free_index);
        break;
      }
    }
  }
  return slot_id;
}

[[nodiscard]] uint8_t ClampCount(int count, size_t max_count,
                                 std::string_view kind) noexcept {
  if (count <= 0) {
    return 0;
  }
  if (static_cast<size_t>(count) > max_count) {
    helios::log::Warn("Joystick {} count {} exceeds max {}; clamping", kind,
                      count, max_count);
    return static_cast<uint8_t>(max_count);
  }
  return static_cast<uint8_t>(count);
}

[[nodiscard]] float AxisValue(SDL_Gamepad* gamepad,
                              helios::input::GamepadAxis axis) {
  const auto sdl_axis = static_cast<SDL_GamepadAxis>(axis);
  const Sint16 raw = SDL_GetGamepadAxis(gamepad, sdl_axis);
  if (axis == helios::input::GamepadAxis::kLeftTrigger ||
      axis == helios::input::GamepadAxis::kRightTrigger) {
    return NormalizeTriggerAxis(raw);
  }
  return NormalizeStickAxis(raw);
}

[[nodiscard]] helios::input::GamepadPower PowerFromSdl(
    SDL_Gamepad* gamepad) noexcept {
  int percent = -1;
  const SDL_PowerState state = SDL_GetGamepadPowerInfo(gamepad, &percent);
  helios::input::GamepadPower power;
  power.percent = static_cast<int8_t>(percent);
  switch (state) {
    case SDL_POWERSTATE_ON_BATTERY:
      power.state = helios::input::GamepadPowerState::kOnBattery;
      break;
    case SDL_POWERSTATE_NO_BATTERY:
      power.state = helios::input::GamepadPowerState::kNoBattery;
      break;
    case SDL_POWERSTATE_CHARGING:
      power.state = helios::input::GamepadPowerState::kCharging;
      break;
    case SDL_POWERSTATE_CHARGED:
      power.state = helios::input::GamepadPowerState::kCharged;
      break;
    default:
      power.state = helios::input::GamepadPowerState::kUnknown;
      break;
  }
  return power;
}

void CloseSlot(GamepadSlotCache& slot) {
  if (slot.gamepad != nullptr) {
    SDL_CloseGamepad(slot.gamepad);
  }
  if (slot.joystick != nullptr) {
    SDL_CloseJoystick(slot.joystick);
  }
  slot = {};
}

void EmitGamepadState(int slot_id, GamepadSlotCache& slot,
                      helios::input::GamepadWriters& writers, bool seed) {
  SDL_Gamepad* gamepad = slot.gamepad;
  if (gamepad == nullptr) {
    return;
  }

  for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; ++button) {
    const auto mapped =
        GamepadButtonFromSdl(static_cast<SDL_GamepadButton>(button));
    if (!mapped.has_value()) {
      continue;
    }
    const size_t index = static_cast<size_t>(*mapped);
    const bool next =
        SDL_GetGamepadButton(gamepad, static_cast<SDL_GamepadButton>(button));
    if (!seed && next == slot.pad_buttons[index]) {
      continue;
    }
    slot.pad_buttons[index] = next;
    if (seed && !next) {
      continue;
    }
    writers.buttons.Write({
        .id = slot_id,
        .button = *mapped,
        .state = next ? helios::input::ButtonState::kPressed
                      : helios::input::ButtonState::kReleased,
    });
  }

  for (size_t i = 0; i < slot.pad_axes.size(); ++i) {
    const auto axis = static_cast<helios::input::GamepadAxis>(i);
    const float next = AxisValue(gamepad, axis);
    if (!seed && next == slot.pad_axes[i]) {
      continue;
    }
    slot.pad_axes[i] = next;
    writers.axes.Write({.id = slot_id, .axis = axis, .value = next});
  }

  const helios::input::GamepadPower power = PowerFromSdl(gamepad);
  if (seed || power.state != slot.power.state ||
      power.percent != slot.power.percent) {
    slot.power = power;
    writers.power.Write({.id = slot_id, .power = power});
  }

  const int touchpads = SDL_GetNumGamepadTouchpads(gamepad);
  if (touchpads > 0) {
    int fingers = SDL_GetNumGamepadTouchpadFingers(gamepad, 0);
    slot.touchpad_count =
        ClampCount(fingers, helios::input::Gamepad::kMaxTouchpadFingers,
                   "touchpad finger");
    for (uint8_t finger = 0; finger < slot.touchpad_count; ++finger) {
      bool down = false;
      float x = 0.0F;
      float y = 0.0F;
      float pressure = 0.0F;
      SDL_GetGamepadTouchpadFinger(gamepad, 0, finger, &down, &x, &y,
                                   &pressure);
      auto& cached = slot.touchpad[finger];
      if (!seed && cached.down == down && cached.x == x && cached.y == y &&
          cached.pressure == pressure) {
        continue;
      }
      cached = {.x = x, .y = y, .pressure = pressure, .down = down};
      writers.touchpad.Write({
          .id = slot_id,
          .x = x,
          .y = y,
          .pressure = pressure,
          .finger = finger,
          .down = down,
      });
    }
  }
}

void EmitJoystickState(int slot_id, GamepadSlotCache& slot,
                       helios::input::JoystickWriters& writers, bool seed) {
  SDL_Joystick* joystick = slot.joystick;
  if (joystick == nullptr) {
    return;
  }

  slot.joy_axis_count = ClampCount(SDL_GetNumJoystickAxes(joystick),
                                   helios::input::Joystick::kMaxAxes, "axis");
  for (uint8_t i = 0; i < slot.joy_axis_count; ++i) {
    const float next = NormalizeStickAxis(SDL_GetJoystickAxis(joystick, i));
    if (!seed && next == slot.joy_axes[i]) {
      continue;
    }
    slot.joy_axes[i] = next;
    writers.axes.Write({.id = slot_id, .axis = i, .value = next});
  }

  slot.joy_button_count =
      ClampCount(SDL_GetNumJoystickButtons(joystick),
                 helios::input::Joystick::kMaxButtons, "button");
  for (uint8_t i = 0; i < slot.joy_button_count; ++i) {
    const bool next = SDL_GetJoystickButton(joystick, i);
    if (!seed && next == slot.joy_buttons[i]) {
      continue;
    }
    slot.joy_buttons[i] = next;
    if (seed && !next) {
      continue;
    }
    writers.buttons.Write({
        .id = slot_id,
        .button = i,
        .state = next ? helios::input::ButtonState::kPressed
                      : helios::input::ButtonState::kReleased,
    });
  }

  slot.joy_hat_count = ClampCount(SDL_GetNumJoystickHats(joystick),
                                  helios::input::Joystick::kMaxHats, "hat");
  for (uint8_t i = 0; i < slot.joy_hat_count; ++i) {
    const auto next = static_cast<uint8_t>(SDL_GetJoystickHat(joystick, i));
    if (!seed && next == slot.hats[i]) {
      continue;
    }
    slot.hats[i] = next;
    writers.hats.Write({
        .id = slot_id,
        .hat = i,
        .value = static_cast<helios::input::JoystickHat>(next),
    });
  }
}

void OpenDevice(GamepadCache& cache, SDL_JoystickID instance_id,
                helios::input::GamepadWriters& gamepads,
                helios::input::JoystickWriters& sticks) {
  if (FindSlotByInstance(cache, instance_id) >= 0) {
    return;
  }

  const bool mapped = SDL_IsGamepad(instance_id);
  if (mapped) {
    SDL_Gamepad* opened = SDL_OpenGamepad(instance_id);
    if (opened == nullptr) {
      return;
    }
    const int slot_id = AllocateSlot(cache, instance_id, opened);
    if (slot_id < 0) {
      SDL_CloseGamepad(opened);
      return;
    }
    GamepadSlotCache& slot = cache.slots[static_cast<size_t>(slot_id)];
    const char* name = SDL_GetGamepadName(opened);
    const char* mapping = SDL_GetGamepadMapping(opened);
    slot = {};
    slot.name = name != nullptr ? name : "";
    slot.guid = GuidToString(SDL_GetGamepadGUIDForID(instance_id));
    slot.gamepad = opened;
    slot.instance_id = static_cast<uint32_t>(instance_id);
    slot.connected = true;
    slot.is_gamepad = true;
    gamepads.connection.Write({
        .id = slot_id,
        .connected = true,
        .name = slot.name,
        .guid = slot.guid,
    });
    if (mapping != nullptr) {
      gamepads.remapped.Write({.id = slot_id, .mapping = mapping});
    }
    EmitGamepadState(slot_id, slot, gamepads, true);
    return;
  }

  SDL_Joystick* opened = SDL_OpenJoystick(instance_id);
  if (opened == nullptr) {
    return;
  }
  const int slot_id = AllocateSlot(cache, instance_id, nullptr);
  if (slot_id < 0) {
    SDL_CloseJoystick(opened);
    return;
  }
  GamepadSlotCache& slot = cache.slots[static_cast<size_t>(slot_id)];
  const char* name = SDL_GetJoystickName(opened);
  slot = {};
  slot.name = name != nullptr ? name : "";
  slot.guid = GuidToString(SDL_GetJoystickGUID(opened));
  slot.joystick = opened;
  slot.instance_id = static_cast<uint32_t>(instance_id);
  slot.connected = true;
  slot.is_gamepad = false;
  EmitJoystickState(slot_id, slot, sticks, true);
  sticks.connection.Write({
      .name = slot.name,
      .guid = slot.guid,
      .id = slot_id,
      .axis_count = slot.joy_axis_count,
      .button_count = slot.joy_button_count,
      .hat_count = slot.joy_hat_count,
      .connected = true,
  });
}

void RemoveDevice(GamepadCache& cache, SDL_JoystickID instance_id,
                  helios::input::GamepadWriters& gamepads,
                  helios::input::JoystickWriters& sticks) {
  const int slot_id = FindSlotByInstance(cache, instance_id);
  if (slot_id < 0) {
    return;
  }
  GamepadSlotCache& slot = cache.slots[static_cast<size_t>(slot_id)];
  if (slot.is_gamepad) {
    gamepads.connection.Write({
        .id = slot_id,
        .connected = false,
        .name = std::move(slot.name),
        .guid = std::move(slot.guid),
    });
  } else {
    sticks.connection.Write({
        .name = std::move(slot.name),
        .guid = std::move(slot.guid),
        .id = slot_id,
        .connected = false,
    });
  }
  CloseSlot(slot);
}

void RemapDevice(GamepadCache& cache, SDL_JoystickID instance_id,
                 helios::input::GamepadWriters& gamepads,
                 helios::input::JoystickWriters& sticks) {
  const int slot_id = FindSlotByInstance(cache, instance_id);
  if (slot_id < 0) {
    OpenDevice(cache, instance_id, gamepads, sticks);
    return;
  }

  GamepadSlotCache& slot = cache.slots[static_cast<size_t>(slot_id)];
  const bool mapped = SDL_IsGamepad(instance_id);
  if (mapped && !slot.is_gamepad) {
    RemoveDevice(cache, instance_id, gamepads, sticks);
    OpenDevice(cache, instance_id, gamepads, sticks);
    return;
  }
  if (!mapped && slot.is_gamepad) {
    RemoveDevice(cache, instance_id, gamepads, sticks);
    OpenDevice(cache, instance_id, gamepads, sticks);
    return;
  }
  if (mapped && slot.gamepad != nullptr) {
    const char* mapping = SDL_GetGamepadMapping(slot.gamepad);
    gamepads.remapped.Write({
        .id = slot_id,
        .mapping = mapping != nullptr ? mapping : "",
    });
    EmitGamepadState(slot_id, slot, gamepads, true);
  }
}

void EnumerateDevices(GamepadCache& cache,
                      helios::input::GamepadWriters& gamepads,
                      helios::input::JoystickWriters& sticks) {
  int count = 0;
  SDL_JoystickID* ids = SDL_GetJoysticks(&count);
  if (ids == nullptr) {
    return;
  }
  for (int index = 0; index < count; ++index) {
    OpenDevice(cache, ids[index], gamepads, sticks);
  }
  SDL_free(ids);
}

}  // namespace

void DestroyGamepadCache(GamepadCache& cache) {
  for (GamepadSlotCache& slot : cache.slots) {
    CloseSlot(slot);
  }
  cache.pending_added.clear();
  cache.pending_removed.clear();
  cache.pending_remapped.clear();
  cache.enumerated = false;
}

void ApplyGamepadMappings::operator()(
    ecs::Res<const Context> context,
    ecs::Res<helios::input::GamepadMappings> mappings) const {
  if (!context->input_enabled || !mappings->dirty) [[unlikely]] {
    return;
  }

  for (const std::string& line : mappings->pending_lines) {
    SDL_AddGamepadMapping(line.c_str());
  }
  for (const std::string& path : mappings->pending_files) {
    SDL_AddGamepadMappingsFromFile(path.c_str());
  }
  mappings->ClearPending();
}

void PollGamepads::operator()(ecs::Res<const Context> context,
                              helios::input::GamepadWriters gamepads,
                              helios::input::JoystickWriters sticks,
                              ecs::Res<GamepadCache> cache) const {
  if (!context->input_enabled) [[unlikely]] {
    return;
  }

  if (!cache->enumerated) {
    EnumerateDevices(*cache, gamepads, sticks);
    cache->enumerated = true;
  }

  for (const uint32_t instance_id : cache->pending_removed) {
    RemoveDevice(*cache, static_cast<SDL_JoystickID>(instance_id), gamepads,
                 sticks);
  }
  cache->pending_removed.clear();

  for (const uint32_t instance_id : cache->pending_added) {
    OpenDevice(*cache, static_cast<SDL_JoystickID>(instance_id), gamepads,
               sticks);
  }
  cache->pending_added.clear();

  for (const uint32_t instance_id : cache->pending_remapped) {
    RemapDevice(*cache, static_cast<SDL_JoystickID>(instance_id), gamepads,
                sticks);
  }
  cache->pending_remapped.clear();

  for (size_t slot_index = 0; slot_index < cache->slots.size(); ++slot_index) {
    GamepadSlotCache& slot = cache->slots[slot_index];
    if (!slot.connected) {
      continue;
    }
    const int slot_id = static_cast<int>(slot_index);
    if (slot.is_gamepad) {
      EmitGamepadState(slot_id, slot, gamepads, false);
    } else {
      EmitJoystickState(slot_id, slot, sticks, false);
    }
  }
}

void ApplyGamepadOutputs::operator()(ecs::Res<const Context> context,
                                     ecs::Res<helios::input::Gamepads> gamepads,
                                     ecs::Res<GamepadCache> cache) const {
  if (!context->input_enabled) [[unlikely]] {
    return;
  }

  for (size_t i = 0; i < gamepads->pads.size(); ++i) {
    helios::input::Gamepad& pad = gamepads->pads[i];
    GamepadSlotCache& slot = cache->slots[i];
    if (!pad.connected || slot.gamepad == nullptr) {
      pad.ClearDirty();
      continue;
    }

    if (pad.Dirty(helios::input::GamepadDirtyFlags::kRumble)) {
      SDL_RumbleGamepad(slot.gamepad, pad.rumble_low, pad.rumble_high,
                        pad.rumble_duration_ms);
      pad.ClearDirty(helios::input::GamepadDirtyFlags::kRumble);
    }
    if (pad.Dirty(helios::input::GamepadDirtyFlags::kTriggerRumble)) {
      SDL_RumbleGamepadTriggers(slot.gamepad, pad.trigger_rumble_left,
                                pad.trigger_rumble_right,
                                pad.trigger_rumble_duration_ms);
      pad.ClearDirty(helios::input::GamepadDirtyFlags::kTriggerRumble);
    }
    if (pad.Dirty(helios::input::GamepadDirtyFlags::kLed)) {
      SDL_SetGamepadLED(slot.gamepad, pad.led_r, pad.led_g, pad.led_b);
      pad.ClearDirty(helios::input::GamepadDirtyFlags::kLed);
    }
    if (pad.Dirty(helios::input::GamepadDirtyFlags::kSensors)) {
      if (SDL_GamepadHasSensor(slot.gamepad, SDL_SENSOR_GYRO)) {
        SDL_SetGamepadSensorEnabled(slot.gamepad, SDL_SENSOR_GYRO,
                                    pad.gyro_enabled);
      }
      if (SDL_GamepadHasSensor(slot.gamepad, SDL_SENSOR_ACCEL)) {
        SDL_SetGamepadSensorEnabled(slot.gamepad, SDL_SENSOR_ACCEL,
                                    pad.accel_enabled);
      }
      pad.ClearDirty(helios::input::GamepadDirtyFlags::kSensors);
    }
  }
}

}  // namespace helios::sdl3::input
