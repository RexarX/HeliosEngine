#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <chrono>
#include <cstddef>
#include <thread>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct AsyncLabel {
  // Async sub-apps run on their own worker loop instead of being stepped once
  // per main-frame update.
  static constexpr bool kAsync = true;
};

struct AsyncCounter {
  size_t ticks = 0;
};

struct LogMain {
  void operator()(hecs::Res<const happ::FrameCount> frames) const {
    hlog::Info("async_sub_apps: main frame={}", frames->count);
  }
};

struct AsyncTick {
  void operator()(hecs::Res<AsyncCounter> counter) const {
    // The sleep makes the background cadence visible beside the main app logs.
    ++counter->ticks;
    hlog::Info("async_sub_apps: async tick {}", counter->ticks);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }
};

void MirrorMain(const hecs::World& main, hecs::World& sub) {
  // Extraction still copies data from main to sub, but the async mode controls
  // when the sub-app consumes that snapshot.
  sub.InsertResources(
      AsyncCounter{.ticks = main.ReadResource<happ::FrameCount>().count});
}

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 15) {
      hlog::Info("async_sub_apps: requesting shutdown");
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.AddSystem(happ::kUpdate, LogMain{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  // The sub-app starts with its own world state, then MirrorMain refreshes it
  // from the main world as the app runs.
  auto async_sub = happ::SubApp::From(AsyncLabel{});
  async_sub.SetExtractFunction(MirrorMain);
  async_sub.InsertResources(AsyncCounter{});
  async_sub.AddSystem(happ::kUpdate, AsyncTick{});
  app.InsertSubApp(AsyncLabel{}, std::move(async_sub));

  hlog::Info("async_sub_apps: starting app with async sub-app");
  const auto code = app.Run();
  hlog::Info("async_sub_apps: app finished");
  return std::to_underlying(code);
}
