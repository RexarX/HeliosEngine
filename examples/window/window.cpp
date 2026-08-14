#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/glfw/glfw.hpp>
#include <helios/log/log.hpp>
#include <helios/window/window.hpp>

#include <helios/profile/backends/tracy.hpp>
#include <helios/profile/profile.hpp>

#include <cstddef>
#include <cstdio>
#include <format>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;
namespace hwindow = helios::window;
namespace hglfw = helios::glfw;

namespace {

struct AvgFps {
  double total_fps = 0.0;
  double avg = 0;

  constexpr void Update(double fps, size_t frame_count) {
    total_fps += fps;
    avg = total_fps / static_cast<double>(frame_count);
  }
};

struct SpawnWindowStartup {
  void operator()(hecs::Commands commands) const {
    commands.Spawn().AddBundle(hwindow::PrimaryWindow{});
  }
};

struct SpawnWindows {
  void operator()(hecs::Res<const happ::FrameCount> count,
                  hecs::Commands commands) {
    if (count->count % 200000 == 0) {
      commands.Spawn().AddComponents(hwindow::Window{
          .properties = {.title = "Spawned Window", .auto_iconify = true}});
    }
  }

  int cnt = 0;
};

struct ComputeAvgFps {
  void operator()(hecs::Res<const happ::Time> time,
                  hecs::Res<const happ::FrameCount> count,
                  hecs::Res<AvgFps> avg_fps) {
    avg_fps->Update(time->Fps(), count->count);
  }
};

struct TrackWindowEvents {
  // MessageReader cursors are persisted per system, so nested FramePumpOrder /
  // update re-entry does not reprint the same CreatedMsg / ResizedMsg.
  void operator()(hecs::MessageReader<hwindow::CreatedMsg> created,
                  hecs::MessageReader<hwindow::ResizedMsg> resized) const {
    for (const auto& msg : created) {
      hlog::Info("Window '{}' created with '{}'", msg->entity, msg->properties);
    }

    for (const auto& msg : resized) {
      hlog::Info("Window '{}' resized to {}x{}", msg->entity, msg->width,
                 msg->height);
    }
  }
};

struct ChangeWindowTitle {
  void operator()(
      hecs::Query<hwindow::Window&, hecs::With<hwindow::Primary>> query,
      hecs::Res<const happ::FrameCount> count,
      hecs::Res<const AvgFps> avg_fps) {
    if (count->count % 10000 == 0) [[unlikely]] {
      hlog::Info("{}", query.Count());
      for (auto&& [window] : query) {
        window.SetTitle(std::format("Avarage FPS: {:.2f}", avg_fps->avg));
      }
    }
  }
};

struct PrintAvgFps {
  void operator()(hecs::Res<const happ::FrameCount> count,
                  hecs::Res<const AvgFps> avg_fps) {
    if (count->count % 10000 == 0) [[unlikely]] {
      hlog::Info("Avarage FPS: {:.2f}", avg_fps->avg);
    }
  }
};

}  // namespace

int main() {
  auto& profiler = helios::profile::Profiler::Instance();
  profiler.AddBackend<helios::profile::TracyBackend>();
  profiler.Finalize();
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  happ::App app;

  app.AddPlugins(happ::TimePlugin{}, happ::FrameCountPlugin{});
  app.AddPluginGroups(hglfw::WindowPlugin{});
  app.InsertResources(AvgFps{});
  app.AddSystem(happ::kStartup, SpawnWindowStartup{});
  app.AddSystem(happ::kPreUpdate, ComputeAvgFps{});
  app.AddSystems(happ::kUpdate, SpawnWindows{}, TrackWindowEvents{},
                 ChangeWindowTitle{}, PrintAvgFps{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
