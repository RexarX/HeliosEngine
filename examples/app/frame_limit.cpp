#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct AvgFps {
  double total_ms = 0.0;
  double avg = 0;

  constexpr void Update(double delta_ms, size_t frame_count) {
    total_ms += delta_ms;
    avg = total_ms / static_cast<double>(frame_count);
  }
};

struct ComputeAvgFps {
  void operator()(hecs::Res<const happ::Time> time,
                  hecs::Res<const happ::FrameCount> count,
                  hecs::Res<AvgFps> avg_fps) {
    if (count->count == 0) [[unlikely]] {
      return;
    }

    avg_fps->Update(time->DeltaMilliSec(), count->count);
  }
};

struct PrintAvgFps {
  void operator()(hecs::Res<const AvgFps> avg) const {
    hlog::Info("avg_delta={:.2f}ms avg_fps={:.1f}", avg->avg,
               1'000.0 / avg->avg);
  }
};

// Prints instantaneous FPS from `Time`. `TimePlugin` updates `Time` in
// `kFirst`, which runs after `FrameLimiter::Wait()` on the same frame, so
// `delta_time` includes the pacing sleep (from the second frame onward).
struct PrintFps {
  void operator()(hecs::Res<const happ::Time> time,
                  hecs::Res<const happ::FrameCount> frames,
                  hecs::Res<const happ::FrameLimiter> limiter) const {
    hlog::Info("frame={} fps={:.1f} dt={:.2f}ms cap={}Hz", frames->count,
               time->Fps(), time->DeltaMilliSec(),
               1'000'000'000 / limiter->Interval().count());
  }
};

struct ExitAfterSeconds {
  void operator()(hecs::Res<const happ::Time> time,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (time->ElapsedSec() >= 10.0) [[unlikely]] {
      hlog::Info("Stopping after {:.2f} s", time->ElapsedSec());
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;

  app.InsertResources(AvgFps{});

  // `TimePlugin` inserts `Time` and updates it every frame in `kFirst`.
  // `FrameCountPlugin` is only used here for log context.
  app.AddPlugins(happ::TimePlugin{}, happ::FrameCountPlugin{});

  // Cap the main loop at 30 FPS. Prefer this with `RunDefault` (the default
  // runner). Do not also use `RunFixed`, both would sleep.
  //
  // Other settings:
  //   FrameLimiterSettings::Off()              // no sleep
  //   FrameLimiterSettings::FromHz(144.0)      // manual interval from Hz
  //   FrameLimiterSettings::FromInterval(16ms) // explicit duration
  //   FrameLimiterSettings::Auto()             // uses `RefreshRate()`; the
  //     window plugin writes primary-monitor Hz. Without a window, Auto
  //     behaves like Off until you call `SetRefreshRate`.
  //
  // For a blocking sub-app, use `InstallFrameLimiter(sub_app, settings)`
  // instead of a second plugin (plugins are keyed by type on `App`).

  auto settings = happ::FrameLimiterSettings::FromFPS(30);
  app.AddPlugins(happ::FrameLimiterPlugin{settings});
  app.AddSystems(happ::kUpdate, ComputeAvgFps{}, PrintFps{},
                 ExitAfterSeconds{});
  app.AddSystem(happ::kShutdown, PrintAvgFps{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
