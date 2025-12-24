#pragma once

#include <helios/core/app/module.hpp>
#include <helios/core/app/schedule.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/ecs/system.hpp>

#include <string_view>

namespace helios::example {

/**
 * @brief Example component to demonstrate module components.
 */
struct ExampleComponent {
  int value = 0;
};

/**
 * @brief Example resource to demonstrate module resources.
 */
struct ExampleResource {
  int counter = 0;

  /**
   * @brief Returns the name of the resource.
   * @return Resource name.
   */
  static constexpr std::string_view GetName() noexcept { return "ExampleResource"; }
};

/**
 * @brief Example system that increments the counter in ExampleResource.
 *
 * This system demonstrates how to create a system that operates on resources.
 */
struct ExampleSystem final : public ecs::System {
  /**
   * @brief Returns the name of the system.
   * @return System name.
   */
  static constexpr std::string_view GetName() noexcept { return "ExampleSystem"; }

  /**
   * @brief Returns the access policy for this system.
   * @return Access policy declaring resource writes.
   */
  static constexpr auto GetAccessPolicy() noexcept { return app::AccessPolicy().WriteResources<ExampleResource>(); }

  /**
   * @brief Updates the system, incrementing the example resource counter.
   * @param ctx The system context providing access to resources.
   */
  void Update(app::SystemContext& ctx) override;
};

/**
 * @brief Example module that demonstrates module structure.
 *
 * This module can be added to an App to demonstrate the module system.
 * It registers the ExampleResource and ExampleSystem.
 *
 * @example
 * @code
 * helios::app::App app;
 * app.AddModule<helios::example::ExampleModule>();
 * @endcode
 */
struct ExampleModule final : public app::Module {
  /**
   * @brief Returns the name of the module.
   * @return Module name.
   */
  static constexpr std::string_view GetName() noexcept { return "Example"; }

  /**
   * @brief Builds the module, adding resources and systems to the app.
   * @param app The application to configure.
   */
  void Build(app::App& app) override;

  /**
   * @brief Destroys the module, performing cleanup.
   * @param app The application to clean up.
   */
  void Destroy(app::App& app) override;
};

}  // namespace helios::example
