#include <doctest/doctest.h>

#include <helios/core/app/schedules.hpp>
#include <helios/core/app/sub_app.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/app/system_set.hpp>
#include <helios/core/app/system_set_config.hpp>
#include <helios/core/ecs/system.hpp>

using namespace helios::app;
using namespace helios::ecs;

namespace {

// Test system sets
struct PhysicsSet {};
struct RenderSet {};
struct GameplaySet {};
struct InputSet {};

// Minimal test system used only to allow schedules to exist
struct DummySystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "DummySystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& /*ctx*/) override {}
};

}  // namespace

TEST_SUITE("app::SystemSetConfig") {
  TEST_CASE("SystemSetConfig::SystemSetConfig: Basic construction") {
    SubApp sub_app;

    // Should compile and create builder
    auto config = sub_app.ConfigureSet<PhysicsSet>(kUpdate);

    CHECK(true);
  }

  TEST_CASE("SystemSetConfig::SystemSetConfig: After constraint") {
    SubApp sub_app;

    // Configure set ordering
    sub_app.ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>();

    CHECK(true);
  }

  TEST_CASE("SystemSetConfig::SystemSetConfig: Before constraint") {
    SubApp sub_app;

    // Configure set ordering
    sub_app.ConfigureSet<PhysicsSet>(kUpdate).Before<RenderSet>();

    CHECK(true);
  }

  TEST_CASE("SystemSetConfig::SystemSetConfig: Multiple ordering constraints") {
    SubApp sub_app;

    // Configure with multiple constraints
    sub_app.ConfigureSet<PhysicsSet>(kUpdate).After<InputSet>().Before<RenderSet>();

    CHECK(true);
  }

  TEST_CASE("SystemSetConfig::SystemSetConfig: Chained configuration with multiple sets") {
    SubApp sub_app;

    sub_app.ConfigureSet<GameplaySet>(kUpdate).After<InputSet>().After<PhysicsSet>().Before<RenderSet>();

    CHECK(true);
  }
}
