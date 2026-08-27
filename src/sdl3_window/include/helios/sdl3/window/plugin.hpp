#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3.window;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/app/plugin.hpp>
#include <helios/app/plugin_group.hpp>
#include <helios/sdl3/plugin.hpp>
#include <helios/window/plugin.hpp>
#include <helios/window/resources.hpp>

#include <string_view>
#endif

HELIOS_MODULE_EXPORT
namespace helios::sdl3::window {

/// @brief Named set for window `Init` on `app::kMainStartup`.
struct StartupSet {
  static constexpr std::string_view kName = "helios::sdl3::window::StartupSet";
};

inline constexpr StartupSet kStartupSet{};

/// @brief Named set for `CreateNativeWindows` on `window::kEvents`.
struct CreateSet {
  static constexpr std::string_view kName = "helios::sdl3::window::CreateSet";
};

inline constexpr CreateSet kCreateSet{};

/// @brief Named set for `PollEvents` / `ApplyChanges` / `DestroyClosedWindows`.
struct ApplySet {
  static constexpr std::string_view kName = "helios::sdl3::window::ApplySet";
};

inline constexpr ApplySet kApplySet{};

/// @brief Named set for window `Shutdown` on `app::kShutdown`.
struct ShutdownSet {
  static constexpr std::string_view kName = "helios::sdl3::window::ShutdownSet";
};

inline constexpr ShutdownSet kShutdownSet{};

/// @brief SDL3 backend that synchronizes OS windows with `helios::window` ECS
/// types.
struct Plugin final : public app::Plugin {
  static constexpr std::string_view kName = "helios::sdl3::window::Plugin";

  void Build(app::App& app) override;
  void Destroy(app::App& app) override;
};

/// @brief A plugin group that includes the `helios::window::Plugin`,
/// `helios::sdl3::Plugin` and `helios::window::sdl3::Plugin`.
struct WindowPlugin final : public app::PluginGroup {
  WindowPlugin(::helios::window::Settings settings = {})
      : PluginGroup(::helios::window::Plugin{settings}, sdl3::Plugin{},
                    Plugin{}) {}
};

}  // namespace helios::sdl3::window
#endif  // HELIOS_MODULE_CONSUMER_SHIM
