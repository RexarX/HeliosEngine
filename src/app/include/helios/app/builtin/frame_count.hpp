#pragma once

#include <helios/app/plugin.hpp>
#include <helios/ecs/resource/params.hpp>

#include <cstddef>
#include <string_view>

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
