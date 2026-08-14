#pragma once

#include <helios/app/plugin.hpp>
#include <helios/app/plugin_group.hpp>
#include <helios/window/plugin.hpp>

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
#include <helios/input/plugin.hpp>
#endif

#include <string_view>

namespace helios::glfw {

/// @brief GLFW backend that synchronizes OS windows with `helios::window` ECS
/// types.
struct Plugin final : public app::Plugin {
  static constexpr std::string_view kName = "helios::glfw::Plugin";

  void Build(app::App& app) override;
  void Finish(app::App& app) override;
  void Destroy(app::App& app) override;
};

/// @brief A plugin group that includes the `helios::glfw::Plugin` and
/// the `helios::window::Plugin`.
struct WindowPlugin final : public app::PluginGroup {
  WindowPlugin() : PluginGroup(Plugin{}, window::Plugin{}) {}
};

#ifdef HELIOS_MODULE_INPUT_AVAILABLE
/// @brief Window + GLFW + input plugin group for interactive applications.
struct WindowInputPlugin final : public app::PluginGroup {
  WindowInputPlugin()
      : PluginGroup(Plugin{}, window::Plugin{}, input::Plugin{}) {}
};
#endif

}  // namespace helios::glfw
