#include <doctest/doctest.h>

#include <helios/ecs/world.hpp>
#include <helios/input/sensor.hpp>

using namespace helios::ecs;
using namespace helios::input;

TEST_SUITE("helios::input::Sensor") {
  TEST_CASE("helios::input::Sensor::Reset") {
    SUBCASE("Clears identity, type, and sample") {
      Sensor sensor;
      sensor.name = "Accel";
      sensor.value = {1.0F, 2.0F, 3.0F};
      sensor.id = 4;
      sensor.type = SensorType::kAccel;
      sensor.connected = true;

      sensor.Reset();

      CHECK(sensor.name.empty());
      CHECK_EQ(sensor.value[0], doctest::Approx(0.0F));
      CHECK_EQ(sensor.value[1], doctest::Approx(0.0F));
      CHECK_EQ(sensor.value[2], doctest::Approx(0.0F));
      CHECK_FALSE(sensor.id.has_value());
      CHECK_EQ(sensor.type, SensorType::kUnknown);
      CHECK_FALSE(sensor.connected);
    }
  }
}

TEST_SUITE("helios::input::Sensors") {
  TEST_CASE("helios::input::Sensors") {
    SUBCASE("Inserts as a resource") {
      World world;
      world.InsertResources(Sensors{});
      CHECK(world.HasResource<Sensors>());
    }
  }

  TEST_CASE("helios::input::Sensors::TryGet") {
    SUBCASE("Returns a mutable slot for a valid id") {
      Sensors sensors;
      Sensor* sensor = sensors.TryGet(0);
      REQUIRE_NE(sensor, nullptr);
      sensor->connected = true;
      CHECK(sensors.devices[0].connected);
    }

    SUBCASE("Returns a const slot for a valid id") {
      Sensors sensors;
      sensors.devices[3].id = 3;
      const Sensors& view = sensors;
      const Sensor* sensor = view.TryGet(3);
      REQUIRE_NE(sensor, nullptr);
      CHECK_EQ(sensor->id, 3);
    }

    SUBCASE("Returns nullptr for an out-of-range id") {
      Sensors sensors;
      CHECK_EQ(sensors.TryGet(static_cast<SensorId>(Sensors::kSlotCount)),
               nullptr);
    }
  }
}

TEST_SUITE("helios::input::SensorConnectionMsg") {
  TEST_CASE("helios::input::SensorConnectionMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<SensorConnectionMsg>();
      CHECK(world.HasMessage<SensorConnectionMsg>());
      world.WriteMessages<SensorConnectionMsg>().Write(
          {.name = "Accel",
           .id = 0,
           .type = SensorType::kAccel,
           .connected = true});
    }
  }
}

TEST_SUITE("helios::input::SensorUpdateMsg") {
  TEST_CASE("helios::input::SensorUpdateMsg") {
    SUBCASE("Registers as an ECS message") {
      World world;
      world.AddMessages<SensorUpdateMsg>();
      CHECK(world.HasMessage<SensorUpdateMsg>());
      world.WriteMessages<SensorUpdateMsg>().Write(
          {.value = {1.0F, 2.0F, 3.0F}, .id = 0, .type = SensorType::kAccel});
    }
  }
}
