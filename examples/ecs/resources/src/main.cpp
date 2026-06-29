#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/world.hpp>
#include <helios/log/logger.hpp>

#include <cstddef>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct FrameCount {
  size_t count = 0;
};

struct GameConfig {
  void OnInsert(hecs::World& /*world*/) {
    hlog::Info("resources: GameConfig::OnInsert");
  }

  void OnRemove(hecs::World& /*world*/) {
    hlog::Info("resources: GameConfig::OnRemove");
  }

  int difficulty = 1;
};

struct ThreadSafeCounter {
  static constexpr bool kThreadSafe = true;
  int value = 0;
};

struct BumpFrame {
  void operator()(hecs::Res<FrameCount> frames) const { ++frames->count; }
};

struct UseRegularResource {
  void operator()(hecs::Res<GameConfig> config) const {
    ++config->difficulty;
    hlog::Info("resources: regular difficulty={}", config->difficulty);
  }
};

// `AsyncRes` is no different from `Res`, used for more explicitness

struct IncrementThreadSafe {
  void operator()(hecs::AsyncRes<ThreadSafeCounter> counter) const {
    ++counter->value;
    hlog::Info("resources: bumped thread-safe counter to {}", counter->value);
  }
};

struct ReadThreadSafeResource {
  void operator()(hecs::AsyncRes<const ThreadSafeCounter> counter) const {
    hlog::Info("resources: thread-safe counter={}", counter->value);
  }
};

struct RemoveConfigOnShutdown {
  void operator()(hecs::Commands commands) const {
    commands.World().RemoveResource<GameConfig>();
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 5) {
      exit_writer.Write({.code = happ::ExitCode::kSuccess});
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.InsertResources(FrameCount{}, GameConfig{}, ThreadSafeCounter{});
  app.AddSystem(happ::kFirst, BumpFrame{});
  app.AddSystems(happ::kUpdate, UseRegularResource{}, IncrementThreadSafe{});
  app.AddSystem(happ::kPostUpdate, ReadThreadSafeResource{});
  app.AddSystem(happ::kLast, ExitAfterFrames{});
  app.AddSystem(happ::kShutdown, RemoveConfigOnShutdown{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
