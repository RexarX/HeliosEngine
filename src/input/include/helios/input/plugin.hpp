#pragma once

#include <helios/app/plugin.hpp>
#include <helios/input/resources.hpp>

#include <string_view>

namespace helios::input {

/// @brief Registers input ECS resources, messages, and update systems.
struct Plugin final : public app::Plugin {
  static constexpr std::string_view kName = "helios::input::Plugin";

  /**
   * @brief Constructs an input plugin with the given settings.
   * @param settings Global input behavior to insert when absent
   */
  explicit Plugin(Settings settings = {}) : settings_(settings) {}

  /**
   * @brief Inserts resources, registers messages, and schedules systems.
   * @param app Application to configure
   */
  void Build(app::App& app) override;

  Settings settings_;
};

}  // namespace helios::input
