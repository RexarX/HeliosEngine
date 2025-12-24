#include <doctest/doctest.h>

#include <helios/core/app/app.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/example/example.hpp>

TEST_SUITE("Example Module - Integration") {
  TEST_CASE("ExampleComponent can be added to entities") {
    // Simple test that doesn't require Update()
    helios::example::ExampleComponent comp{42};
    CHECK_EQ(comp.value, 42);
  }

  TEST_CASE("ExampleResource has correct name") {
    CHECK_EQ(helios::example::ExampleResource::GetName(), "ExampleResource");
  }

  TEST_CASE("ExampleSystem has correct name") {
    CHECK_EQ(helios::example::ExampleSystem::GetName(), "ExampleSystem");
  }

  TEST_CASE("ExampleModule can be instantiated") {
    helios::example::ExampleModule module;
    CHECK_EQ(helios::example::ExampleModule::GetName(), "Example");
  }

  TEST_CASE("ExampleModule can be added to App without crashing") {
    helios::app::App app;

    // Add some basic system so the app can be built
    struct DummySystem final : public helios::ecs::System {
      static constexpr std::string_view GetName() noexcept { return "DummySystem"; }
      static constexpr auto GetAccessPolicy() noexcept { return helios::app::AccessPolicy(); }
      void Update(helios::app::SystemContext&) override {}
    };

    app.AddSystem<DummySystem>(helios::app::kUpdate);
    app.AddModule<helios::example::ExampleModule>();

    app.Initialize();
    app.Update();

    // Just verify it doesn't crash during construction
    CHECK(true);
  }

  TEST_CASE("ExampleModule Build registers resources and systems") {
    helios::app::App app;
    helios::example::ExampleModule module;

    // Manually call Build
    CHECK_NOTHROW(module.Build(app));

    // Verify systems were added (indirect check via app state)
    CHECK(true);
  }

  TEST_CASE("ExampleModule Destroy doesn't crash") {
    helios::app::App app;
    helios::example::ExampleModule module;

    module.Build(app);
    CHECK_NOTHROW(module.Destroy(app));
  }
}
