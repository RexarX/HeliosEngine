#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>
#include <helios/app/sub_app.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/log/logger.hpp>

#include <chrono>
#include <cstddef>
#include <thread>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct AsyncLabel {
  static constexpr bool kAsync = true;
};

struct FrameCount {
  size_t count = 0;
};

struct AsyncCounter {
  size_t ticks = 0;
};

struct BumpFrame {
  void operator()(hecs::Res<FrameCount> frames) const { ++frames->count; }
};

struct LogMain {
  void operator()(hecs::Res<const FrameCount> frames) const {
    hlog::Info("async_sub_apps: main frame={}", frames->count);
  }
};

struct AsyncTick {
  void operator()(hecs::Res<AsyncCounter> counter) const {
    ++counter->ticks;
    hlog::Info("async_sub_apps: async tick {}", counter->ticks);
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }
};

void MirrorMain(const hecs::World& main, hecs::World& sub) {
  sub.InsertResources(
      AsyncCounter{.ticks = main.ReadResource<FrameCount>().count});
}

struct ExitAfterFrames {
  void operator()(hecs::Res<const FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 15) {
      hlog::Info("async_sub_apps: requesting shutdown");
      exit_writer.Write({.code = happ::ExitCode::kSuccess});
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.InsertResources(FrameCount{});
  app.AddSystem(happ::kFirst, BumpFrame{});
  app.AddSystem(happ::kUpdate, LogMain{});
  app.AddSystem(happ::kLast, ExitAfterFrames{});

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
