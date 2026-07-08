#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

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
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 5) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.InsertResources(GameConfig{}, ThreadSafeCounter{});
  app.AddSystems(happ::kUpdate, UseRegularResource{}, IncrementThreadSafe{});
  app.AddSystem(happ::kPostUpdate, ReadThreadSafeResource{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});
  app.AddSystem(happ::kShutdown, RemoveConfigOnShutdown{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
