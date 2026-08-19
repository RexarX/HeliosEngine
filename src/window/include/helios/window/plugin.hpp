#pragma once

#include <helios/app/builtin/frame_limiter.hpp>
#include <helios/app/plugin.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/window/params.hpp>
#include <helios/window/resources.hpp>

#include <string_view>

namespace helios::window {

/// @brief Copies the active display refresh rate into `app::FrameLimiter`.
struct SyncFrameLimiterRefreshRate {
  static constexpr std::string_view kName =
      "helios::window::SyncFrameLimiterRefreshRate";

  void operator()(ecs::Res<app::FrameLimiter> limiter,
                  ecs::Res<const Monitors> monitors,
                  PrimaryWindowsView primaries) const;
};

/// @brief Registers window ECS types without creating OS windows.
struct Plugin final : public app::Plugin {
  static constexpr std::string_view kName = "helios::window::Plugin";

  /**
   * @brief Constructs a window plugin with the given settings.
   * @param settings Global window behavior to insert when absent
   */
  explicit Plugin(Settings settings = {}) : settings(settings) {}

  void Build(app::App& app) override;

  Settings settings;
};

}  // namespace helios::window
