#include <pch.hpp>

#include <helios/sdl3/input/systems/poll_sensors.hpp>

#include <details/input_map.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/input/ids.hpp>
#include <helios/input/params.hpp>
#include <helios/input/sensor.hpp>
#include <helios/sdl3/input/state.hpp>

#include <SDL3/SDL_sensor.h>
#include <SDL3/SDL_stdinc.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace helios::sdl3::input {

namespace {

void CloseSlot(SensorSlotCache& slot) {
  if (slot.sensor != nullptr) {
    SDL_CloseSensor(slot.sensor);
  }
  slot = {};
}

[[nodiscard]] auto FindSlotByInstance(const SensorCache& cache,
                                      SDL_SensorID instance_id) noexcept
    -> std::optional<helios::input::SensorId> {
  const auto id = static_cast<uint32_t>(instance_id);
  for (size_t index = 0; index < cache.slots.size(); ++index) {
    const SensorSlotCache& slot = cache.slots[index];
    if (slot.connected && slot.instance_id == id) {
      return static_cast<helios::input::SensorId>(index);
    }
  }
  return std::nullopt;
}

[[nodiscard]] auto AllocateSlot(SensorCache& cache) noexcept
    -> std::optional<helios::input::SensorId> {
  for (size_t index = 0; index < cache.slots.size(); ++index) {
    if (!cache.slots[index].connected) {
      return static_cast<helios::input::SensorId>(index);
    }
  }
  return std::nullopt;
}

void OpenDevice(SensorCache& cache, SDL_SensorID instance_id,
                helios::input::SensorWriters& writers) {
  if (FindSlotByInstance(cache, instance_id)) {
    return;
  }

  SDL_Sensor* opened = SDL_OpenSensor(instance_id);
  if (opened == nullptr) {
    return;
  }

  const auto slot_id = AllocateSlot(cache);
  if (!slot_id) {
    SDL_CloseSensor(opened);
    return;
  }

  SensorSlotCache& slot = cache.slots[*slot_id];
  const char* name = SDL_GetSensorName(opened);
  const helios::input::SensorType type =
      SensorTypeFromSdl(SDL_GetSensorType(opened));
  slot = {};
  slot.name = name != nullptr ? name : "";
  slot.sensor = opened;
  slot.instance_id = static_cast<uint32_t>(instance_id);
  slot.type = type;
  slot.connected = true;
  writers.connection.Write({
      .name = slot.name,
      .id = *slot_id,
      .type = type,
      .connected = true,
  });

  float data[3] = {};
  if (SDL_GetSensorData(opened, data, 3)) {
    writers.samples.Write({
        .value = {data[0], data[1], data[2]},
        .id = *slot_id,
        .type = type,
    });
  }
}

void RemoveDevice(SensorCache& cache, helios::input::SensorId slot_id,
                  helios::input::SensorWriters& writers) {
  SensorSlotCache& slot = cache.slots[slot_id];
  writers.connection.Write({
      .name = std::move(slot.name),
      .id = slot_id,
      .type = slot.type,
      .connected = false,
  });
  CloseSlot(slot);
}

}  // namespace

void DestroySensorCache(SensorCache& cache) {
  for (SensorSlotCache& slot : cache.slots) {
    CloseSlot(slot);
  }
}

void PollSensors::operator()(ecs::Res<const Context> context,
                             helios::input::SensorWriters sensors,
                             ecs::Res<SensorCache> cache) const {
  if (!context->input_enabled) [[unlikely]] {
    return;
  }

  int count = 0;
  SDL_SensorID* ids = SDL_GetSensors(&count);
  if (ids == nullptr) {
    count = 0;
  }

  std::array<uint8_t, SensorCache::kSlotCount> seen = {};
  for (int index = 0; index < count; ++index) {
    const SDL_SensorID instance_id = ids[index];
    const auto existing = FindSlotByInstance(*cache, instance_id);
    if (existing) {
      seen[*existing] = 1;
      continue;
    }
    OpenDevice(*cache, instance_id, sensors);
    const auto opened = FindSlotByInstance(*cache, instance_id);
    if (opened) {
      seen[*opened] = 1;
    }
  }
  if (ids != nullptr) {
    SDL_free(ids);
  }

  for (size_t index = 0; index < cache->slots.size(); ++index) {
    if (cache->slots[index].connected && seen[index] == 0) {
      RemoveDevice(*cache, static_cast<helios::input::SensorId>(index),
                   sensors);
    }
  }
}

}  // namespace helios::sdl3::input
