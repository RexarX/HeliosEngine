#include <doctest/doctest.h>

#include <helios/core/app/app.hpp>
#include <helios/example/example.hpp>

TEST_SUITE("Example Module - Components") {
  TEST_CASE("ExampleComponent default construction") {
    helios::example::ExampleComponent comp;
    CHECK_EQ(comp.value, 0);
  }

  TEST_CASE("ExampleComponent with value") {
    helios::example::ExampleComponent comp{42};
    CHECK_EQ(comp.value, 42);
  }

  TEST_CASE("ExampleComponent value modification") {
    helios::example::ExampleComponent comp{10};
    comp.value += 5;
    CHECK_EQ(comp.value, 15);
  }
}

TEST_SUITE("Example Module - Resources") {
  TEST_CASE("ExampleResource default construction") {
    helios::example::ExampleResource resource;
    CHECK_EQ(resource.counter, 0);
  }

  TEST_CASE("ExampleResource GetName") {
    CHECK_EQ(helios::example::ExampleResource::GetName(), "ExampleResource");
  }

  TEST_CASE("ExampleResource counter increment") {
    helios::example::ExampleResource resource;
    ++resource.counter;
    CHECK_EQ(resource.counter, 1);
    resource.counter += 10;
    CHECK_EQ(resource.counter, 11);
  }
}

TEST_SUITE("Example Module - Systems") {
  TEST_CASE("ExampleSystem GetName") {
    CHECK_EQ(helios::example::ExampleSystem::GetName(), "ExampleSystem");
  }

  TEST_CASE("ExampleSystem access policy") {
    auto policy = helios::example::ExampleSystem::GetAccessPolicy();
    // Basic check that policy was created
    CHECK(true);  // Policy creation doesn't throw
  }
}

TEST_SUITE("Example Module - Module") {
  TEST_CASE("ExampleModule GetName") {
    CHECK_EQ(helios::example::ExampleModule::GetName(), "Example");
  }

  TEST_CASE("ExampleModule Build") {
    helios::app::App app;
    helios::example::ExampleModule module;

    // Should not throw
    CHECK_NOTHROW(module.Build(app));
  }

  TEST_CASE("ExampleModule Destroy") {
    helios::app::App app;
    helios::example::ExampleModule module;

    module.Build(app);

    // Should not throw
    CHECK_NOTHROW(module.Destroy(app));
  }
}
