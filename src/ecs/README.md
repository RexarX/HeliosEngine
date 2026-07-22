# `ecs` — Entity Component System

Data-oriented ECS with deferred commands, double-buffered messages, archetype-based component storage, and parallel schedule execution. The heart of the engine.

## Public API

### Core Types

| Type                                              | Purpose                                                                       |
| ------------------------------------------------- | ----------------------------------------------------------------------------- |
| `World`                                           | Owns entities, components, resources, messages, and the global command queue. |
| `WorldView`                                       | Read-only, thread-safe projection of a `const World&`.                        |
| `Entity`                                          | 64-bit ID: 32-bit index + 32-bit generation.                                  |
| `ComponentBundle<Ts...>`                          | Owning, composable group of component values for structural operations.       |
| `Schedule`                                        | System collection with ordering, access policies, and executor selection.     |
| `Scheduler`                                       | Groups schedules into stages; builds and runs them.                           |
| `Commands`                                        | Deferred mutation queue for systems (`Spawn`, `Entity`, `World`).             |
| `Query<Args...>`                                  | Entity iteration with component access and `With<>` / `Without<>` filters.    |
| `Res<T>`                                          | Resource access (`Res<const T>` for read-only).                               |
| `Local<T>`                                        | Per-system local storage (never conflicts in scheduling).                     |
| `MessageWriter<T>`                                | Write regular messages (current frame buffer).                                |
| `MessageReader<T>`                                | Read regular messages (previous frame buffer).                                |
| `AsyncMessageWriter<T>` / `AsyncMessageReader<T>` | Lock-free async message queue.                                                |

### World Lifecycle

| Method     | Effect                                                                          |
| ---------- | ------------------------------------------------------------------------------- |
| `Flush()`  | Flush reserved entities + execute global commands. No message lifecycle change. |
| `Update()` | `Flush()` + advance message lifecycle (swap buffers, clear aged messages).      |
| `Clear()`  | Remove all world data.                                                          |

`World::Update()` is called once per frame after the update stage completes (handled by `app::Scheduler`).

## Quick Start

```cpp
#include <helios/ecs/world.hpp>
#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/schedule/schedule.hpp>

struct Position { float x = 0, y = 0; };
struct Velocity { float dx = 1, dy = 0; };
struct Player {};

helios::ecs::World world;

// Components
auto entity = world.CreateEntity();
world.AddComponents(entity, Player{}, Position{}, Velocity{});

// Resources
world.InsertResources(GameConfig{.speed = 1.0F});

// Query
auto movables = world.Query<Position&, const Velocity&, helios::ecs::With<Player>>();
movables.ForEach([](Position& pos, const Velocity& vel) {
  pos.x += vel.dx;
  pos.y += vel.dy;
});

world.Update();
```

## Systems & Schedules

Systems declare data access through parameter types. The schedule auto-deduces access policies and builds a parallel execution DAG.

```cpp
#include <helios/ecs/system/system.hpp>

struct MoveSystem {
  void operator()(helios::ecs::Res<const GameConfig> config,
                  helios::ecs::Query<Position&, const Velocity&> movables) {
    if (config->paused) return;
    movables.ForEach([&config](Position& pos, const Velocity& vel) {
      pos.x += vel.dx * config->speed;
      pos.y += vel.dy * config->speed;
    });
  }
};

helios::ecs::Schedule schedule("Update");
schedule.Add(MoveSystem{});
helios::async::Executor async_executor;
schedule.SetExecutor(
    std::make_unique<helios::ecs::MultiThreadedExecutor>(async_executor));
schedule.Build();

schedule.RunAndWait(world);
```

### System Parameters

| Parameter                 | Access                                                                   |
| ------------------------- | ------------------------------------------------------------------------ |
| `Res<T>` / `Res<const T>` | Mutable / read-only resource                                             |
| `Query<Args...>`          | Entity iteration (`T&`, `const T&`, `const T*`, `With<T>`, `Without<T>`) |
| `Commands`                | Deferred spawn/destroy/component/resource mutations                      |
| `Local<T>`                | Per-system persistent state                                              |
| `MessageWriter<T>`        | Write messages (visible next frame)                                      |
| `MessageReader<T>`        | Read messages (from previous frame)                                      |

### Custom System Parameters

Built-in parameters (`Res`, `Query`, `Commands`, …) are wired through `SystemParamTraits` in `helios/ecs/system/param_traits.hpp`. User types extend the same mechanism by specializing `SystemParamTraits<T>` in `namespace helios::ecs`.

A type models the `SystemParam` concept when the specialization provides:

| Method                                                | Purpose                                                     |
| ----------------------------------------------------- | ----------------------------------------------------------- |
| `RegisterAccess(AccessPolicyBuilder&)`                | Declare component/resource accesses for parallel scheduling |
| `Make(World&, SystemLocalData&, const AccessPolicy&)` | Construct the parameter at system invocation time           |

`Commands`, `Local<T>`, `WorldView`, and message params register no scheduling access — they never participate in conflict detection.

#### Aggregate parameters (`CompositeSystemParam`)

Bundle multiple built-in params into one struct; field types must match the template list exactly:

```cpp
#include <helios/ecs/query/query.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/system/composite_param.hpp>
#include <helios/ecs/system/param_traits.hpp>

struct Mesh {};
struct Transform { float x = 0; };
struct Camera { float fov = 90.0F; };

struct RenderParam {
  helios::ecs::Res<const Camera> camera;
  helios::ecs::Query<const Mesh&, const Transform&> meshes;
};

namespace helios::ecs {

template <>
struct helios::ecs::SystemParamTraits<RenderParam>
    : helios::ecs::CompositeSystemParam<RenderParam,
          helios::ecs::Res<const Camera>,
          helios::ecs::Query<const Mesh&, const Transform&>
      > {};

}  // namespace helios::ecs

struct RenderSystem {
  void operator()(RenderParam param) {
    param.meshes.ForEach([&](const Mesh&, const Transform& t) {
      // use param.camera->fov and t.x
    });
  }
};
```

#### Fully custom parameters

Implement `SystemParamTraits` directly when you need custom `Make` logic or non-standard access declaration:

```cpp
#include <helios/ecs/resource/param.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/system/access_decl.hpp>
#include <helios/ecs/system/param_traits.hpp>

struct Time { float delta = 0.0F; };
struct PhysicsWorld {};

struct PhysicsStepParam {
  float dt = 0.0F;
  helios::ecs::Res<PhysicsWorld> world;
};

namespace helios::ecs {

template <>
struct helios::ecs::SystemParamTraits<PhysicsStepParam> {
  static PhysicsStepParam Make(helios::ecs::World& world,
                               helios::ecs::SystemLocalData& /*data*/,
                               const helios::ecs::AccessPolicy& /*policy*/) {
    return PhysicsStepParam{
        .dt = world.ReadResource<Time>().delta,
        .world = Res<PhysicsWorld>(world.WriteResource<PhysicsWorld>())};
  }

  static constexpr void RegisterAccess(helios::ecs::AccessPolicyBuilder& builder) {
    builder.ReadResources<Time>();
    builder.WriteResources<PhysicsWorld>();
  }
};

}  // namespace helios::ecs

struct PhysicsSystem {
  void operator()(PhysicsStepParam step) {
    step.world->Simulate(step.dt);
  }
};
```

Use `DeclareReadComponents`, `DeclareWriteComponents`, or `DeclareQueryAccess` from `helios/ecs/system/access_decl.hpp` when you need finer-grained component declarations inside `RegisterAccess`.

After specialization, pass the type to `Schedule::Add` like any other system — access policy and parallel ordering are deduced automatically.

### Ordering

```cpp
struct SpawnSet {};
struct MovementSet {};

schedule.Add(SpawnSystem{}, MoveSystem{}).InSet(MovementSet{}).Sequence();
schedule.Set<SpawnSet>().Sequence();
schedule.Add(RenderSystem{}).Before<MoveSystem>();
```

## Commands — Deferred Mutations

Systems never mutate the world directly during parallel execution. All structural changes go through `Commands`:

```cpp
struct SpawnEnemies {
  void operator()(helios::ecs::Commands commands) {
    commands.Spawn()
        .AddComponents(Enemy{}, Position{0, 0}, Velocity{1, 0});
  }
};

struct Cleanup {
  void operator()(helios::ecs::Query<Lifetime&> lifetimes,
                  helios::ecs::Commands commands) {
    lifetimes.ForEachWithEntity([&](helios::ecs::Entity e, Lifetime& lt) {
      if (lt.remaining <= 0) {
        commands.Entity(e).Destroy();
      }
    });
  }
};
```

Commands apply at schedule boundaries (`RunAndWait` → `Flush()` → per-system local flush). Split spawning and consumption across schedules (e.g. `kPreUpdate` → `kUpdate`) when entities must exist before queries run.

## Messages

Regular messages use a double-buffer with a 2-frame TTL:

```
Frame N:   write via MessageWriter<T>
Frame N+1: read via MessageReader<T>
Frame N+2: auto-cleared (unless kManual clear policy)
```

```cpp
struct DamageEvent {
  helios::ecs::Entity target;
  float amount = 0;
};

world.AddMessage<DamageEvent>();

struct ApplyDamage {
  void operator()(helios::ecs::MessageReader<DamageEvent> events,
                  helios::ecs::Query<Health&> healths) {
    events.ForEach([&](const DamageEvent& evt) {
      if (auto* hp = healths.Get(evt.target)) {
        hp->value -= evt.amount;
      }
    });
  }
};
```

Async messages (`kAsync = true`) use a lock-free queue and are not managed by `World::Update()` — clear explicitly.

## Components

Two storage models, chosen per component type:

| Model          | Default for          | Behavior                                              |
| -------------- | -------------------- | ----------------------------------------------------- |
| **Archetype**  | Non-empty components | SoA columns; add/remove migrates between archetypes.  |
| **Sparse set** | Tag/empty components | Independent per-type storage; no archetype migration. |

Override with `static constexpr ComponentStorageType kStorageType = ...` in the component struct.

### Component Bundles

`ComponentBundle<Ts...>` groups component values that are commonly added or
removed together. A bundle can contain other bundles; nested leaves are
flattened depth-first and left-to-right.

```cpp
#include <helios/ecs/component/bundle.hpp>

using MovementBundle = helios::ecs::ComponentBundle<Position, Velocity>;
using PlayerBundle =
    helios::ecs::ComponentBundle<Player, MovementBundle, Health>;

world.AddBundle(entity, PlayerBundle{
    Player{}, MovementBundle{Position{}, Velocity{}}, Health{100}});
world.RemoveBundle<MovementBundle>(entity);
```

Bundles are transfer objects rather than components: queries access their leaf
components, and `ComponentTrait<ComponentBundle<...>>` is false. Bundle element
types must be unqualified values, every leaf must be a component, and flattened
leaf component types must be unique. Empty bundles are rejected.

## Queries

```cpp
// Mutable + const + optional pointer + filters
auto q = world.Query<Transform&, const Mesh&, const Light*,
                     helios::ecs::With<Visible>, helios::ecs::Without<Hidden>>();

q.ForEach([](Transform& t, const Mesh& mesh, const Light* light) { /* ... */ });
q.ForEachWithEntity([](helios::ecs::Entity e, Transform& t,
                       const Mesh& mesh, const Light* light) { /* ... */ });

// Lazy adapters: Filter, Map, Take, Skip, Enumerate, Chain, Zip, ...
// Terminals: Collect, Fold, Find, Count, Any, All, None, GroupBy, ...
```

## Executors

| Kind                     | Behavior                                     |
| ------------------------ | -------------------------------------------- |
| `MainThreadExecutor`     | Sequential on calling thread                 |
| `SingleThreadedExecutor` | Sequential on one background thread          |
| `MultiThreadedExecutor`  | Parallel via `async::Executor` work-stealing |

## Builtin Types

- **Commands:** `DestroyEntityCmd`, `AddComponentsCmd`, `AddBundleCmd`, `InsertResourceCmd`, `FunctionCmd`, … (`builtin_commands.hpp`)
- **Messages:** `EntityAddedMsg`, `EntityDestroyedMsg`, `ComponentAddedMsg<T>`, `ResourceInsertedMsg<T>`, … (`builtin_messages.hpp`)

## Dependencies

- `async` — parallel schedule execution
- `container` — sparse sets, typed buffers, multi-type maps
- `core`, `compiler`, `memory`, `utils`, `log`
- External: Boost `flat_map`, moodycamel `ConcurrentQueue`
