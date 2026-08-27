#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.sdl3;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/app/plugin.hpp>

#include <string_view>
#endif

HELIOS_MODULE_EXPORT
namespace helios::sdl3 {

/// @brief Named set for `Init` on `app::kMainStartup`.
struct StartupSet {
  static constexpr std::string_view kName = "helios::sdl3::StartupSet";
};

inline constexpr StartupSet kStartupSet{};

/// @brief Named set for `PumpEvents` on `window::kEvents`.
struct EventPumpSet {
  static constexpr std::string_view kName = "helios::sdl3::EventPumpSet";
};

inline constexpr EventPumpSet kEventPumpSet{};

/// @brief Named set for `Shutdown` on `app::kShutdown`.
struct ShutdownSet {
  static constexpr std::string_view kName = "helios::sdl3::ShutdownSet";
};

inline constexpr ShutdownSet kShutdownSet{};

/// @brief Registers the process-global SDL3 runtime, event pump, and shutdown.
struct Plugin final : public app::Plugin {
  static constexpr std::string_view kName = "helios::sdl3::Plugin";

  void Build(app::App& app) override;
  void Finish(app::App& app) override;
  void Destroy(app::App& app) override;
};

}  // namespace helios::sdl3
#endif  // HELIOS_MODULE_CONSUMER_SHIM
