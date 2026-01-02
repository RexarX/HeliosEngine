#pragma once

#include <helios/core/app/module.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/ecs/system.hpp>

#include <string_view>

namespace helios::test {

/**
 * @brief Test resource for dynamic module testing.
 */
struct TestResource {
  int counter = 0;
  bool initialized = false;

  static constexpr std::string_view GetName() noexcept { return "TestResource"; }
};

/**
 * @brief Test system that increments the test resource counter.
 */
struct TestSystem final : public ecs::System {
  static constexpr std::string_view GetName() noexcept { return "TestSystem"; }

  static constexpr auto GetAccessPolicy() noexcept { return app::AccessPolicy().WriteResources<TestResource>(); }

  void Update(app::SystemContext& ctx) override;
};

/**
 * @brief Test module for dynamic loading integration tests.
 * @details This module is compiled as a shared library and loaded dynamically
 * to test the DynamicModule functionality.
 */
class TestModule final : public app::Module {
public:
  static constexpr std::string_view GetName() noexcept { return "TestModule"; }

  void Build(app::App& app) override;
  void Destroy(app::App& app) override;
};

}  // namespace helios::test
