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

struct OverlapLabel {
  static constexpr bool kAllowOverlappingUpdates = true;
  static constexpr size_t kMaxOverlappingUpdates = 1;
};

struct MainTick {
  int value = 0;
};

struct SubTick {
  int generation = 0;
};

struct BumpMain {
  void operator()(hecs::Res<MainTick> tick) const {
    ++tick->value;
    hlog::Info("overlapping_sub_apps: main tick={}", tick->value);
  }
};

struct SlowSubUpdate {
  void operator()(hecs::Res<SubTick> tick) const {
    hlog::Info("overlapping_sub_apps: sub update start gen={}",
               tick->generation);
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    ++tick->generation;
    hlog::Info("overlapping_sub_apps: sub update end gen={}", tick->generation);
  }
};

void ExtractSub(const hecs::World& main, hecs::World& sub) {
  hlog::Info("overlapping_sub_apps: extract main={}",
             main.ReadResource<MainTick>().value);
  sub.InsertResources(
      SubTick{.generation = main.ReadResource<MainTick>().value});
}

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 8) {
      hlog::Info("overlapping_sub_apps: exit frame {}", frames->count);
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.InsertResources(MainTick{});
  app.AddSystem(happ::kUpdate, BumpMain{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  auto overlap = happ::SubApp::From(OverlapLabel{});
  overlap.SetExtractFunction(ExtractSub);
  overlap.InsertResources(SubTick{});
  overlap.AddSystem(happ::kUpdate, SlowSubUpdate{});
  app.InsertSubApp(OverlapLabel{}, std::move(overlap));

  const auto code = app.Run();
  return std::to_underlying(code);
}
