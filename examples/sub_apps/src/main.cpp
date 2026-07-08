#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct MainCounter {
  int value = 0;
};

struct SubCounter {
  int extracted = 0;
};

struct WorkerLabel {};

struct BumpMain {
  void operator()(hecs::Res<MainCounter> counter) const {
    ++counter->value;
    hlog::Info("sub_apps: main update value={}", counter->value);
  }
};

struct BumpSub {
  void operator()(hecs::Res<SubCounter> counter) const {
    ++counter->extracted;
    hlog::Info("sub_apps: sub-app update extracted={}", counter->extracted);
  }
};

void ExtractToSub(const hecs::World& main, hecs::World& sub) {
  const int value = main.ReadResource<MainCounter>().value;
  hlog::Info("sub_apps: extract main={} -> sub", value);
  sub.InsertResources(SubCounter{.extracted = value});
}

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 5) {
      hlog::Info("sub_apps: shutdown after {} frames", frames->count);
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.InsertResources(MainCounter{});
  app.AddSystem(happ::kUpdate, BumpMain{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  auto worker = happ::SubApp::From(WorkerLabel{});
  worker.SetExtractFunction(ExtractToSub);
  worker.InsertResources(SubCounter{});
  worker.AddSystem(happ::kUpdate, BumpSub{});
  app.InsertSubApp(WorkerLabel{}, std::move(worker));

  const auto code = app.Run();
  return std::to_underlying(code);
}
