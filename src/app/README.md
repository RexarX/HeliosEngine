# `app` — Application Framework

High-level application layer that wires together the ECS, async executor, plugins, and frame scheduling. Owns the main game loop, builtin schedules, and optional sub-apps for parallel or background worlds.

## Public API

| Type / Function    | Purpose                                                                                   |
| ------------------ | ----------------------------------------------------------------------------------------- |
| `App`              | Central owner: executor, scheduler, main sub-app, plugins, sub-apps.                      |
| `SubApp`           | Independent ECS world + scheduler; supports blocking, overlapping, or async update modes. |
| `Scheduler`        | Frame orchestrator: startup, update, extract, shutdown stages.                            |
| `Plugin`           | Static plugin base (`Build`, `Finish`, `Destroy`, `Poll`, `IsReady`).                     |
| `DynamicPlugin`    | Runtime-loaded plugin wrapper around a shared library.                                    |
| `ExitCode`         | `kSuccess`, `kFailure`.                                                                   |
| `AppExit`          | Message that requests application shutdown (`kManual` clear policy).                      |
| `RunDefault`       | Runner: loop `Update()` until `AppExit`.                                                  |
| `RunFixed`         | Fixed-timestep runner via `FixedRunnerConfig`.                                            |
| `RunOnce`          | Single-frame runner.                                                                      |
| `RunDefaultSubApp` | Default async sub-app runner.                                                             |

### Builtin Stages & Schedules (`schedules.hpp`)

| Stage            | Schedules                                                 |
| ---------------- | --------------------------------------------------------- |
| `kStartupStage`  | `kMainStartup`, `kPreStartup`, `kStartup`, `kPostStartup` |
| `kUpdateStage`   | `kFirst`, `kPreUpdate`, `kUpdate`, `kPostUpdate`, `kLast` |
| `kExtractStage`  | `kExtract`                                                |
| `kShutdownStage` | `kPreShutdown`, `kShutdown`, `kPostShutdown`              |

Stages are grouping namespaces only — they are never run directly. Call `RegisterBuiltinSchedules(scheduler)` to wire default ordering and executor kinds.

### Sub-App Label Traits

```cpp
struct PhysicsLabel {};  // default: blocking

struct AsyncSimLabel {
  static constexpr bool kAsync = true;
};

struct OverlapLabel {
  static constexpr bool kAllowOverlappingUpdates = true;
  static constexpr size_t kMaxOverlappingUpdates = 2;  // 0 = unlimited skips
};
```

Async and overlapping modes are mutually exclusive (compile-time enforced).

## Quick Start

```cpp
#include <helios/app/app.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>

struct Position { float x = 0, y = 0; };
struct Velocity { float dx = 1, dy = 0; };

struct MoveEntities {
  void operator()(helios::ecs::Query<Position&, const Velocity&> movables) {
    movables.ForEach([](Position& pos, const Velocity& vel) {
      pos.x += vel.dx;
      pos.y += vel.dy;
    });
  }
};

struct RequestExit {
  void operator()(helios::ecs::MessageWriter<helios::app::AppExit> exit) {
    exit.Write({.code = helios::app::ExitCode::kSuccess});
  }
};

int main() {
  helios::app::App app;

  app.AddSystems(helios::app::kUpdate, MoveEntities{}, RequestExit{})
      .Sequence();
  app.InsertResources(/* resources... */);
  app.AddMessages<helios::app::AppExit>();

  return static_cast<int>(app.Run());
}
```

## Lifecycle

```
Run() → Initialize() → runner_(*this) → CleanUp()
```

1. **Initialize** — builds plugins, waits for readiness, finishes plugins, builds scheduler, runs startup.
2. **Runner** — defaults to `RunDefault` (calls `Update()` each frame until `AppExit`).
3. **CleanUp** — shutdown schedules, plugin destruction, waits for async work.

`ShouldExit()` reads `AppExit` from the main world. Systems write it via `MessageWriter<AppExit>`.

## Frame Lifecycle

Each `Update()` runs one frame through `Scheduler::RunFrame`:

1. Reset extract flags.
2. **Update stage** (main) — schedules `kFirst` → `kLast`, then `World::Update()` (message lifecycle).
3. **Extract stage** — main extract schedules, then sub-app extraction (mode-dependent).
4. **Launch sub-app updates** — blocking sub-apps in parallel; overlapping only when fresh extract exists.
5. **WaitForSubApps** — joins blocking sub-apps.

## Sub-Apps

```cpp
struct PhysicsLabel {};

app.InsertSubApp(PhysicsLabel{}, helios::app::SubApp{});
app.GetSubApp<PhysicsLabel>().SetExtractFunction(
    [](const helios::ecs::World& main, helios::ecs::World& sub) {
      // Copy data from main into sub world each frame
    });
```

| Mode                   | Extraction                                                                   | Update                                                                  |
| ---------------------- | ---------------------------------------------------------------------------- | ----------------------------------------------------------------------- |
| **Blocking** (default) | Waits for previous update, then extracts.                                    | Launched in parallel with other blocking sub-apps; joined at frame end. |
| **Overlapping**        | Skips extraction while update is in flight (up to `kMaxOverlappingUpdates`). | Fire-and-forget when fresh extract is available.                        |
| **Async**              | Every frame (`allow_while_updating=true`); user handles thread safety.       | Background loop from startup until exit.                                |

## Plugins

```cpp
class GamePlugin final : public helios::app::Plugin {
public:
  void Build(helios::app::App& app) override {
    app.AddSystems(helios::app::kStartup, SetupWorld{});
  }
};

app.AddPlugins(GamePlugin{});
```

Dynamic plugins export `helios_plugin_create` and `helios_plugin_type_id` (see `dynamic_plugin.hpp`).

## Custom Runner

```cpp
app.SetRunner([](helios::app::App& app) -> helios::app::ExitCode {
  while (!app.ShouldExit()) {
    app.Update();
    // custom pacing, rendering, etc.
  }
  return app.ShouldExit().value_or(helios::app::ExitCode::kSuccess);
});
```

## Dependencies

- `async` — work-stealing executor
- `ecs` — world, schedules, systems
- `log` — logging
- `utils` — type info, traits
