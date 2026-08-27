#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.app;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/ecs/resource/params.hpp>

#include <cstddef>
#include <string_view>
#endif
#include <helios/app/plugin.hpp>

HELIOS_MODULE_EXPORT
namespace helios::app {

class App;

/// @brief Application frame counter resource.
struct FrameCount {
  static constexpr std::string_view kName = "helios::app::FrameCount";

  size_t count = 0;
};

/// @brief Increments the application frame counter resource.
struct CountFrame {
  static constexpr std::string_view kName = "helios::app::CountFrame";

  void operator()(ecs::Res<FrameCount> frame_count) const noexcept {
    ++frame_count->count;
  }
};

/// @brief Adds the `FrameCount` resource and its per-frame update system.
class FrameCountPlugin final : public Plugin {
public:
  static constexpr std::string_view kName = "helios::app::FrameCountPlugin";

  void Build(App& app) override;
};

}  // namespace helios::app
#endif  // HELIOS_MODULE_CONSUMER_SHIM
