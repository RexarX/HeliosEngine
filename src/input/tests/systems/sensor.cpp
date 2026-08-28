#include <doctest/doctest.h>

#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/system/param.hpp>
#include <helios/ecs/system/param_policy.hpp>
#include <helios/ecs/world.hpp>
#include <helios/input/systems/sensor.hpp>

using namespace helios::ecs;
using namespace helios::input;

namespace {

void AddSensorMessages(World& world) {
  world.AddMessages<SensorConnectionMsg, SensorUpdateMsg>();
}

void RunUpdateSensor(World& world) {
  auto local_data = SystemLocalData::From();
  const AccessPolicy policy =
      BuildPolicyFromParams<Res<Sensors>, SensorMessages>();
  UpdateSensorState{}(
      SystemParamTraits<Res<Sensors>>::Make(world, local_data, policy),
      SystemParamTraits<SensorMessages>::Make(world, local_data, policy));
}

}  // namespace

TEST_SUITE("helios::input::UpdateSensorState") {
  TEST_CASE("helios::input::UpdateSensorState::operator()") {
    SUBCASE("Applies connect and sample messages") {
      World world;
      world.InsertResources(Sensors{});
      AddSensorMessages(world);

      world.WriteMessages<SensorConnectionMsg>().Write({
          .name = "Accel",
          .id = 0,
          .type = SensorType::kAccel,
          .connected = true,
      });
      world.WriteMessages<SensorUpdateMsg>().Write({
          .value = {1.0F, 2.0F, 3.0F},
          .id = 0,
          .type = SensorType::kAccel,
      });
      RunUpdateSensor(world);

      const Sensor& sensor = world.ReadResource<Sensors>().devices[0];
      CHECK(sensor.connected);
      CHECK_EQ(sensor.name, "Accel");
      CHECK_EQ(sensor.type, SensorType::kAccel);
      CHECK_EQ(sensor.value[0], doctest::Approx(1.0F));
      CHECK_EQ(sensor.value[2], doctest::Approx(3.0F));
    }

    SUBCASE("Disconnect clears identity") {
      World world;
      world.InsertResources(Sensors{});
      AddSensorMessages(world);

      world.WriteMessages<SensorConnectionMsg>().Write({
          .name = "Gyro",
          .id = 1,
          .type = SensorType::kGyro,
          .connected = true,
      });
      RunUpdateSensor(world);
      world.WriteMessages<SensorConnectionMsg>().Write({
          .id = 1,
          .connected = false,
      });
      RunUpdateSensor(world);

      const Sensor& sensor = world.ReadResource<Sensors>().devices[1];
      CHECK_FALSE(sensor.connected);
      CHECK(sensor.name.empty());
      CHECK_FALSE(sensor.id.has_value());
    }
  }
}
