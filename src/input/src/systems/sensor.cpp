#include <pch.hpp>

#include <helios/input/systems/sensor.hpp>

#include <helios/ecs/resource/params.hpp>
#include <helios/input/params.hpp>
#include <helios/input/sensor.hpp>

namespace helios::input {

void UpdateSensorState::operator()(ecs::Res<Sensors> sensors,
                                   SensorMessages messages) const {
  for (const auto msg : messages.connection) {
    Sensor* sensor = sensors->TryGet(msg->id);
    if (sensor == nullptr) [[unlikely]] {
      continue;
    }

    sensor->Reset();
    sensor->connected = msg->connected;
    if (msg->connected) {
      sensor->id = msg->id;
      sensor->name = msg->name;
      sensor->type = msg->type;
    }
  }

  for (const auto msg : messages.samples) {
    Sensor* sensor = sensors->TryGet(msg->id);
    if (sensor == nullptr || !sensor->connected) [[unlikely]] {
      continue;
    }
    sensor->value = msg->value;
    sensor->type = msg->type;
  }
}

}  // namespace helios::input
