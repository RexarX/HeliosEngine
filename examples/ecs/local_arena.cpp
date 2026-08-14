#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <memory_resource>
#include <utility>
#include <vector>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// `LocalArena` is inserted automatically into each system's local data. Request
// it through `Local<LocalArena>` or `Local<const LocalArena>` for scratch
// allocations that live only for the current system invocation and are
// reclaimed when the schedule applies deferred work or starts a new run.
//
// System params `Query`, `Commands`, and `MessageWriter` are also
// using this arena allocator implicitly.

struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct SpawnTargets {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Commands commands) const {
    if (frames->count != 0) {
      return;
    }

    commands.Spawn().AddComponents(Position{.x = 0.0F, .y = 0.0F});
    commands.Spawn().AddComponents(Position{.x = 3.0F, .y = 4.0F});
    commands.Spawn().AddComponents(Position{.x = 6.0F, .y = 8.0F});
  }
};

struct LogCentroidWithScratch {
  void operator()(hecs::Query<const Position&> positions,
                  hecs::Local<hecs::LocalArena> arena,
                  hecs::Res<const happ::FrameCount> frames) const {
    auto* alloc = arena->GetPtr();
    std::pmr::vector<Position> scratch{alloc};

    positions.ForEach(
        [&scratch](const Position& pos) { scratch.push_back(pos); });

    if (scratch.empty()) {
      return;
    }

    float sum_x = 0.0F;
    float sum_y = 0.0F;
    for (const Position& pos : scratch) {
      sum_x += pos.x;
      sum_y += pos.y;
    }

    const auto count = static_cast<float>(scratch.size());
    hlog::Info(
        "frame={} centroid=({}, {}) from {} points, arena_empty={} "
        "arena_capacity={}",
        frames->count, sum_x / count, sum_y / count, scratch.size(),
        alloc->Empty(), alloc->TotalCapacity());
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 3) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});

  // Spawn once, then reuse the arena-backed scratch buffer every update frame.
  app.AddSystem(happ::kPreUpdate, SpawnTargets{});
  app.AddSystem(happ::kUpdate, LogCentroidWithScratch{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
