#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>
#include <helios/sdl3/window/window.hpp>
#include <helios/window/window.hpp>

#include <cstddef>
#include <cstdio>
#include <format>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;
namespace hwindow = helios::window;
namespace hsdl3 = helios::sdl3;

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

struct ComputeAvgFps {
  void operator()(hecs::Res<const happ::Time> time,
                  hecs::Res<const happ::FrameCount> count,
                  hecs::Res<AvgFps> avg_fps) {
    avg_fps->Update(time->Fps(), count->count);
  }
};

struct TrackWindowLifecycle {
  void operator()(hwindow::LifecycleMessages messages) const {
    for (const auto msg : messages.created) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.closed) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.close_requested) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.failed) {
      hlog::Info("{}", *msg);
    }
  }
};

struct TrackWindowGeometry {
  void operator()(hwindow::GeometryMessages messages) const {
    for (const auto msg : messages.resized) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.client_resized) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.content_scale) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.pos) {
      hlog::Info("{}", *msg);
    }
  }
};

struct TrackWindowAppearance {
  void operator()(hwindow::AppearanceMessages messages) const {
    for (const auto msg : messages.mode) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.cursor_mode) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.visibility) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.focus) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.maximized) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.icon) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.resizable) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.decorated) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.opacity) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.floating) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.hover) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.mouse_passthrough) {
      hlog::Info("{}", *msg);
    }
  }
};

struct TrackWindowPlatform {
  void operator()(hwindow::PlatformMessages messages) const {
    for (const auto msg : messages.clipboard) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.dropped_files) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.monitor_connected) {
      hlog::Info("{}", *msg);
    }

    for (const auto msg : messages.monitor_disconnected) {
      hlog::Info("{}", *msg);
    }
  }
};

struct ChangeWindowTitle {
  void operator()(
      hecs::Query<hwindow::Window&, hecs::With<hwindow::Primary>> query,
      hecs::Res<const happ::FrameCount> count,
      hecs::Res<const AvgFps> avg_fps) {
    if (count->count % 10000 == 0) [[unlikely]] {
      for (auto&& [window] : query) {
        window.SetTitle(std::format("Avarage FPS: {:.2f}", avg_fps->avg));
      }
    }
  }
};

}  // namespace

int main() {
  happ::App app;

  app.InsertResources(AvgFps{});
  app.AddPlugins(happ::TimePlugin{}, happ::FrameCountPlugin{});
  app.AddPluginGroups(hsdl3::window::WindowPlugin{});
  app.AddSystem(happ::kStartup, SpawnWindowStartup{});
  app.AddSystem(happ::kPreUpdate, ComputeAvgFps{});
  app.AddSystems(happ::kUpdate, TrackWindowLifecycle{}, TrackWindowGeometry{},
                 TrackWindowAppearance{}, TrackWindowPlatform{},
                 ChangeWindowTitle{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
