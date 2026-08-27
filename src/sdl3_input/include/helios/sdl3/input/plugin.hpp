#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.input;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/app/plugin.hpp>
#include <helios/app/plugin_group.hpp>
#include <helios/input/plugin.hpp>
#include <helios/input/resources.hpp>
#include <helios/sdl3/plugin.hpp>

#include <string_view>
#endif

HELIOS_MODULE_EXPORT
namespace helios::sdl3::input {
/// @brief Named set for input `Init` on `app::kMainStartup`.
struct StartupSet {
  static constexpr std::string_view kName = "helios::sdl3::input::StartupSet";
};

inline constexpr StartupSet kStartupSet{};

/// @brief Named set for gamepad / cursor / raw-mouse systems on `kEvents`.
struct ApplySet {
  static constexpr std::string_view kName = "helios::sdl3::input::ApplySet";
};

inline constexpr ApplySet kApplySet{};

/// @brief SDL3 backend that emits `helios::input` messages and applies
/// cursors.
struct Plugin final : public app::Plugin {
  static constexpr std::string_view kName = "helios::sdl3::input::Plugin";

  void Build(app::App& app) override;
  void Finish(app::App& app) override;
  void Destroy(app::App& app) override;
};

/// @brief A plugin group that includes the `helios::input::Plugin`,
/// `::sdl3::Plugin` and `helios::input::sdl3::Plugin`.
struct InputPlugin final : public app::PluginGroup {
  InputPlugin(::helios::input::Settings settings = {})
      : PluginGroup(::helios::input::Plugin{settings}, sdl3::Plugin{},
                    Plugin{}) {}
};

}  // namespace helios::sdl3::input
#endif  // HELIOS_MODULE_CONSUMER_SHIM
