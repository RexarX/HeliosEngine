#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/app/plugin.hpp>

#include <string_view>
#endif
#include <helios/input/resources.hpp>

HELIOS_MODULE_EXPORT
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
#endif  // HELIOS_MODULE_CONSUMER_SHIM
