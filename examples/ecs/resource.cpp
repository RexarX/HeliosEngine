#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <optional>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// Regular resources live in the World. Any system that asks for
// `Res<GameConfig>` observes the same shared object.
struct GameConfig {
  void OnInsert(hecs::World& /*world*/) {
    hlog::Info("resources: GameConfig::OnInsert");
  }

  void OnRemove(hecs::World& /*world*/) {
    hlog::Info("resources: GameConfig::OnRemove");
  }

  int difficulty = 1;
};

// Thread-safe resources can be requested through `AsyncRes<T>`. The parameter
// works like `Res<T>`, but documents that parallel access is intended.
struct ThreadSafeCounter {
  static constexpr bool kThreadSafe = true;
  int value = 0;
};

// Local resources are stored in the system's local data instead of the World.
// Current schedule execution clears that data around each run, so this is best
// used for per-run scratch state here.
struct LocalScratch {
  int updates_this_run = 0;
};

struct DebugOverlay {
  bool visible = false;
};

struct UseRegularResource {
  void operator()(hecs::Res<GameConfig> config) const {
    ++config->difficulty;
    hlog::Info("resources: regular difficulty={}", config->difficulty);
  }
};

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

struct ReadOptionalResources {
  void operator()(std::optional<hecs::Res<const GameConfig>> config,
                  std::optional<hecs::Res<const DebugOverlay>> overlay) const {
    // Optional world resources let a system run whether the resource exists or
    // not. GameConfig exists, but DebugOverlay is intentionally not inserted.
    if (config.has_value()) {
      hlog::Info("resources: optional config difficulty={}",
                 (*config)->difficulty);
    }

    if (!overlay.has_value()) {
      hlog::Info("resources: optional debug overlay is absent");
    }
  }
};

struct UseLocalScratch {
  void operator()(hecs::Local<LocalScratch> scratch) const {
    // Required local resources are default-created before the system runs.
    ++scratch->updates_this_run;
    hlog::Info("resources: local scratch updates={}",
               scratch->updates_this_run);
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

  // Insert world resources before any systems try to access them.
  app.InsertResources(GameConfig{}, ThreadSafeCounter{});

  app.AddSystems(happ::kUpdate, UseRegularResource{}, IncrementThreadSafe{},
                 UseLocalScratch{});

  app.AddSystems(happ::kPostUpdate, ReadThreadSafeResource{},
                 ReadOptionalResources{}, ExitAfterFrames{});

  // Removing the resource on shutdown triggers GameConfig::OnRemove.
  app.AddSystem(happ::kShutdown, RemoveConfigOnShutdown{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
