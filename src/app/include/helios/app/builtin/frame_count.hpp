#pragma once

#include <helios/app/application.hpp>
#include <helios/app/plugin.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/resource/param.hpp>

#include <cstddef>
#include <string_view>

namespace helios::app {

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

  void Build(App& app) override {
    app.TryInsertResources(FrameCount{});
    app.AddSystem(kLast, CountFrame{});
  }
};

}  // namespace helios::app
