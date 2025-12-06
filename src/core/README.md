<a name="top"></a>

# Helios Engine - Core Module

The Core module is the foundation of Helios Engine, providing a high-performance Entity Component System (ECS), async runtime, event system, and essential utilities.

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
  - [Scheduling](#scheduling)
  - [Entity Component System (ECS)](#entity-component-system-ecs)
    - [Entities](#entities)
    - [Components](#components)
    - [Systems](#systems)
    - [Events](#events)
    - [Resources](#resources)
  - [SystemContext API](#systemcontext-api)
    - [Key Methods](#key-methods)
- [Usage Examples](#usage-examples)
  - [Creating Systems](#creating-systems)
  - [Working with Queries](#working-with-queries)
  - [Entity Creation and Modification](#entity-creation-and-modification)
  - [Event System](#event-system)
  - [Resources and State Management](#resources-and-state-management)
  - [Modules](#modules)
  - [SubApps](#subapps)
- [API Reference](#api-reference)
  - [Core Classes](#core-classes)
  - [System Context Methods](#system-context-methods)
  - [Query Builder](#query-builder)
  - [Access Policy Builder](#access-policy-builder)
- [Best Practices](#best-practices)
  - [Anti-patterns](#anti-patterns)
- [Testing](#testing)

---

<a name="overview"></a>

## Overview

The Core module provides:

- **Archetype-based ECS** - High-performance entity management with cache-friendly data layouts
- **Parallel System Execution** - Automatic parallelization based on declared data access
- **Type-safe Event System** - Efficient event propagation with compile-time type checking
- **Resource Management** - Global state management with thread-safe access
- **Query System** - Powerful component queries with filters and optional components
- **Command Buffers** - Deferred entity/component operations for safe mutations
- **Async Runtime** - Async task scheduling
- **Modular Architecture** - App, Module, and SubApp abstractions for organizing code

**Key Files:**

- `include/helios/core/app/` - Application framework (App, Module, SystemContext)
- `include/helios/core/ecs/` - ECS implementation (World, Entity, System, Query, Event)
- `include/helios/core/async/` - Async runtime
- `include/helios/core/utils/` - Utilities (random, timer, etc.)

---

<a name="architecture"></a>

## Architecture

<a name="scheduling"></a>

### Scheduling

Systems are organized into **schedules** that define execution order:

**Built-in Schedules:**

- `kPreStartup` - Runs once before startup
- `kStartup` - Runs once at application start
- `kPostStartup` - Runs once after startup
- `kMain` - Runs every frame on the main thread
- `kPreUpdate` - Runs before main update cycle
- `kUpdate` - Main update cycle
- `kPostUpdate` - Runs after main update cycle
- `kPreCleanUp` - Runs before cleanup
- `kCleanUp` - Resource cleanup
- `kPostCleanUp` - Runs after cleanup

**Custom Schedules:**

```cpp
struct PhysicsSchedule {
  static constexpr std::string_view GetName() noexcept {
    return "Physics";
  }

  static constexpr ScheduleId GetStage() noexcept {
    return ScheduleIdOf<Update>();
  }

  static constexpr auto After() noexcept {
    return std::array{ScheduleIdOf<Update>()};
  }

  static constexpr auto Before() noexcept {
    return std::array{ScheduleIdOf<PostUpdate>()};
  }
};

inline constexpr PhysicsSchedule kPhysicsSchedule{};

// Use custom schedule
app.AddSystem<PhysicsSystem>(kPhysicsSchedule);
```

<a name="entity-component-system-ecs"></a>

### Entity Component System (ECS)

Helios uses an **archetype-based ECS** where entities with the same component types are stored together in contiguous memory (archetypes). This provides:

- **Cache-friendly iteration** - Components are stored in arrays for optimal CPU cache utilization
- **Fast queries** - O(archetypes) instead of O(entities)
- **Type safety** - Compile-time component type checking
- **Parallel execution** - Systems declare data access patterns for automatic parallelization

<a name="entities"></a>

#### Entities

Entities are unique identifiers (UUID-based) that serve as handles to component data:

```cpp
// Entity is a lightweight handle
struct Entity {
  // Internal UUID-based identifier
  // Cheap to copy, pass by value
};
```

<a name="components"></a>

#### Components

Components are plain data structures (POD types) with no behavior:

```cpp
struct Transform {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
  float rotation = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;
};

struct Health {
  int max_health = 100;
  int current_health = 100;

  bool IsDead() const noexcept { return current_health <= 0; }
};

// Tag components (zero-size)
struct Player {};
struct Enemy {};
struct Dead {};
```

<a name="systems"></a>

#### Systems

Systems are classes that process entities with specific components. They derive from `System` and implement:

1. **GetName()** - Returns system name for diagnostics
2. **GetAccessPolicy()** - Declares what data the system reads/writes
3. **Update(SystemContext&)** - System logic, called each frame

```cpp
struct MovementSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "MovementSystem";
  }

  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Transform&, const Velocity&>().ReadResources<GameTime>();
  }

  void Update(SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<GameTime>();
    auto query = ctx.Query().Get<Transform&, const Velocity&>();

    query.ForEach([&time](Transform& transform, const Velocity& velocity) {
      transform.x += velocity.dx * time.delta_time;
      transform.y += velocity.dy * time.delta_time;
      transform.z += velocity.dz * time.delta_time;
    });
  }
};
```

<a name="events"></a>

#### Events

Events provide decoupled communication between systems:

```cpp
// Define event
struct CollisionEvent {
  Entity entity_a;
  Entity entity_b;
  float impact_force = 0.0F;

  static constexpr std::string_view GetName() noexcept {
    return "CollisionEvent";
  }
};

// Register event
app.AddEvent<CollisionEvent>();

// Emit event
void Update(SystemContext& ctx) override {
  ctx.EmitEvent(CollisionEvent{entity_a, entity_b, 10.0F});
}

// Read events
void Update(SystemContext& ctx) override {
  auto reader = ctx.ReadEvents<CollisionEvent>();
  for (const auto& event : reader) {
    // Handle event
  }
}
```

<a name="resources"></a>

#### Resources

Resources are global singletons accessible from any system:

```cpp
// Define resource
struct GameTime {
  float delta_time = 0.016F;
  float total_time = 0.0F;
  int frame_count = 0;

  static constexpr std::string_view GetName() noexcept {
    return "GameTime";
  }
};

// Insert resource
app.InsertResource(GameTime{});

// Access in system
void Update(SystemContext& ctx) override {
auto& time = ctx.WriteResource<GameTime>(); // Mutable access
++time.frame_count;

const auto& time_ref = ctx.ReadResource<GameTime>(); // Immutable access
}

```

<a name="systemcontext-api"></a>

### SystemContext API

`SystemContext` is the primary interface systems use to interact with the ECS. It provides access to:

- **Resources** - Global state (read/write)
- **Queries** - Entity component iteration
- **Commands** - Deferred entity/component operations
- **Events** - Event emission and reading
- **World** - Direct world access (when needed)

<a name="key-methods"></a>

#### Key Methods

```cpp
// Resources
auto& resource = ctx.WriteResource<T>();
const auto& resource = ctx.ReadResource<T>();

// Queries
auto query = ctx.Query().Get<Transform&, const Velocity&>();
auto query = ctx.Query().With<Player>().Without<Dead>().Get<Health&>();

// Commands
auto world_cmd = ctx.Commands();  // World-level commands

auto entity_cmd = ctx.EntityCommands(entity); // Entity-specific commands
auto new_entity_cmd = ctx.EntityCommands(ctx.ReserveEntity()); // Create new entity

// Events
ctx.EmitEvent(CollisionEvent{entity_a, entity_b});
auto reader = ctx.ReadEvents<CollisionEvent>();

// World access (advanced)
Entity entity = ctx.ReserveEntity();
```

---

<a name="usage-examples"></a>

## Usage Examples

<a name="creating-systems"></a>

### Creating Systems

Systems declare their data dependencies via `GetAccessPolicy()`:

```cpp
struct PhysicsSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "PhysicsSystem";
  }

  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Transform&, Velocity&>().ReadResources<GameTime, PhysicsSettings>();
  }

  void Update(SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<GameTime>();
    const auto& settings = ctx.ReadResource<PhysicsSettings>();
    auto query = ctx.Query().Get<Transform&, Velocity&>();

    query.ForEach([&](Transform& transform, Velocity& velocity) {
      // Apply gravity
      velocity.dy -= settings.gravity * time.delta_time;

      // Apply friction
      velocity.dx *= (1.0F - settings.friction * time.delta_time);
      velocity.dz *= (1.0F - settings.friction * time.delta_time);
    });
  }
};
```

<a name="working-with-queries"></a>

### Working with Queries

Queries retrieve entities with specific component combinations:

#### Basic Query

```cpp
void Update(SystemContext& ctx) override {
  auto query = ctx.Query().Get<Transform&, const Velocity&>();

  query.ForEach([](Transform& transform, const Velocity& velocity) {
    transform.x += velocity.dx;
  });
}
```

#### Query with Entity Access

```cpp
void Update(SystemContext& ctx) override {
  auto query = ctx.Query().Get<const Health&>();

  query.ForEachWithEntity([&](Entity entity, const Health& health) {
    if (health.IsDead()) {
      std::cout << "Entity " << entity << " is dead\n";
    }
  });
}
```

#### Query with Filters

```cpp
void Update(SystemContext& ctx) override {
  // Query entities with Player tag, without Dead tag
  auto query = ctx.Query().With<Player>().Without<Dead>().Get<Health&, Transform&>();

  query.ForEach([](Health& health, Transform& transform) {
    // Only processes alive players
  });
}
```

#### Query with Manual Filtering

```cpp
void Update(SystemContext& ctx) override {
  auto query = ctx.Query().Get<Health&>();

  // Filter during iteration
  for (auto&& [health] : query.Filter([](const Health& hp) { return hp.current_health < 50; })) {
    health.current_health += 10;  // Heal low-health entities
  }
}
```

<a name="entity-creation-and-modification"></a>

### Entity Creation and Modification

Systems create and modify entities through command buffers:

#### Creating Entities

```cpp
struct SpawnerSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "SpawnerSystem";
  }

  static constexpr AccessPolicy GetAccessPolicy() noexcept {
    return {};
  }

  void Update(SystemContext& ctx) override {
    // Reserve entity ID
    Entity new_entity = ctx.ReserveEntity();

    // Get entity command buffer
    auto entity_cmd = ctx.EntityCommands(new_entity);

    // Add components
    entity_cmd.AddComponents(
        Transform{0.0F, 0.0F, 0.0F},
        Velocity{1.0F, 0.0F, 0.0F},
        Health{100, 100},
        Enemy{});

    // Emit spawn event
    ctx.EmitEvent(EntitySpawnedEvent{new_entity, "Enemy"});
  }
};
```

#### Modifying Entities

```cpp
struct DamageSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "DamageSystem";
  }

  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Health&>();
  }

  void Update(SystemContext& ctx) override {
    auto query = ctx.Query().With<Enemy>().Get<Health&>();

    for (auto&& [entity, health] : query.WithEntity()) {
      health.current_health -= 10;

      if (health.IsDead()) {
        auto entity_cmd = ctx.EntityCommands(entity);
        entity_cmd.AddComponent(Dead{});
      }
    }
  }
};
```

#### Destroying Entities

```cpp
struct CleanupSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "CleanupSystem";
  }

  static constexpr AccessPolicy GetAccessPolicy() noexcept {
    return {};
  }

  void Update(SystemContext& ctx) override {
    auto query = ctx.Query().With<Dead>().Get();
    auto world_cmd = ctx.Commands();

    query.ForEachWithEntity([&world_cmd](Entity entity) {
      world_cmd.Destroy(entity);
    });
  }
};
```

<a name="event-system"></a>

### Event System

Events enable decoupled communication:

```cpp
// Event definition
struct CombatEvent {
  Entity attacker;
  Entity target;
  int damage = 0;

  static constexpr std::string_view GetName() noexcept {
    return "CombatEvent";
  }
};

// Emitting events
struct CombatSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "CombatSystem";
  }

  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().Query<Health&>();
  }

  void Update(SystemContext& ctx) override {
    auto query = ctx.Query().With<Enemy>().Get<Health&>();
    for (auto&& [entity, health] : query.WithEntity()) {
      if (!health.IsDead()) {
        health.current_health -= 10;
        ctx.EmitEvent(CombatEvent{
            .attacker = Entity{},
            .target = entity,
            .damage = 10
        });
      }
    }
  }

};

// Reading events
struct EventLoggerSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "EventLoggerSystem";
  }

  static constexpr AccessPolicy GetAccessPolicy() noexcept {
    return {};
  }

  void Update(SystemContext& ctx) override {
    auto combat_reader = ctx.ReadEvents<CombatEvent>();
    for (const auto& event : combat_reader) {
      std::cout << "Combat: " << event.attacker
                << " dealt " << event.damage
                << " damage to " << event.target << "\n";
    }
  }

};
```

<a name="resources-and-state-management"></a>

### Resources and State Management

Resources hold global state:

```cpp
// Time resource
struct GameTime {
  float delta_time = 0.016F;
  float total_time = 0.0F;
  int frame_count = 0;

  static constexpr std::string_view GetName() noexcept {
    return "GameTime";
  }
};

// Statistics resource
struct GameStats {
  int entities_spawned = 0;
  int entities_destroyed = 0;
  int combat_events = 0;

  static constexpr std::string_view GetName() noexcept {
    return "GameStats";
  }
};

// System that updates time
struct TimeUpdateSystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "TimeUpdateSystem";
  }

  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().WriteResources<GameTime>();
  }

  void Update(SystemContext& ctx) override {
    auto& time = ctx.WriteResource<GameTime>();
    time.delta_time = 0.016F;
    time.total_time += time.delta_time;
    ++time.frame_count;
  }
};

// System that reads multiple resources
struct StatsDisplaySystem final : public System {
  static constexpr std::string_view GetName() noexcept {
    return "StatsDisplaySystem";
  }

  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().ReadResources<GameTime, GameStats>();
  }

  void Update(SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<GameTime>();
    const auto& stats = ctx.ReadResource<GameStats>();

    if (time.frame_count % 60 == 0) {  // Every 60 frames
      std::cout << "Frame: " << time.frame_count
                << ", Spawned: " << stats.entities_spawned
                << ", Destroyed: " << stats.entities_destroyed << "\n";
    }
  }
};
```

<a name="modules"></a>

### Modules

Modules organize systems and resources:

```cpp
struct GameplayModule final : public Module {
  static constexpr std::string_view GetName() noexcept {
    return "GameplayModule";
  }

  void Build(App& app) override {
    // Add resources
    app.InsertResource(GameStats{});

    // Register events
    app.AddEvent<CombatEvent>();
    app.AddEvent<EntitySpawnedEvent>();

    // Add systems to schedules
    app.AddSystems<CombatSystem, DamageSystem, SpawnerSystem>(kUpdate);
    app.AddSystem<CleanupSystem>(kPostUpdate);
  }

  void Destroy(App& app) override {
    // Cleanup if needed
  }
};

// Composite module
struct GameModule final : public Module {
  static constexpr std::string_view GetName() noexcept {
    return "GameModule";
  }

  void Build(App& app) override {
    // Add sub-modules
    app.AddModule<CoreModule>();
    app.AddModule<PhysicsModule>();
    app.AddModule<GameplayModule>();
  }

  void Destroy(App& app) override {}
};
```

<a name="subapps"></a>

### SubApps

SubApps provide isolated worlds for subsystems (e.g., rendering, physics):

```cpp
// Define SubApp
struct RenderSubApp {
  static constexpr std::string_view GetName() noexcept {
    return "RenderSubApp";
  }
};

// Setup SubApp
app.AddSubApp<RenderSubApp>();
auto& render_sub_app = app.GetSubApp<RenderSubApp>();
render_sub_app.InsertResource(RenderData{});
render_sub_app.AddSystem<RenderSystem>(kUpdate);

// Extraction: copy data from main world to SubApp
app.SetSubAppExtraction<RenderSubApp>([](World& main_world, World& render_world) {
  auto& render_data = render_world.WriteResource<RenderData>();
  render_data.entities.clear();

  // Extract entities with Transform and Name
  auto query = QueryBuilder(main_world).Get<const Transform&, const Name&>();
  query.ForEachWithEntity([&](Entity entity, const Transform& transform, const Name& name) {
    render_data.entities.push_back({entity, transform, name.value});
  });
});
```

---

<a name="api-reference"></a>

## API Reference

<a name="core-classes"></a>

### Core Classes

- **`App`** - Main application class, manages lifecycle and schedules
- **`Module`** - Interface for organizing systems and resources
- **`System`** - Base class for all systems
- **`SystemContext`** - Primary interface for systems to access ECS
- **`World`** - Central ECS container
- **`Entity`** - Unique entity identifier
- **`AccessPolicy`** - Declares system data access patterns
- **`Query`** / `QueryBuilder` - Entity component queries

<a name="system-context-methods"></a>

### System Context Methods

```cpp
// Resources
T& WriteResource<T>();
const T& ReadResource<T>();

// Queries
QueryBuilder Query();

// Commands
WorldCommandBuffer Commands();
EntityCommandBuffer EntityCommands(Entity entity);
Entity ReserveEntity();

// Events
void EmitEvent(EventType event);
EventReader<EventType> ReadEvents<EventType>();
```

<a name="query-builder"></a>

### Query Builder

```cpp
QueryBuilder Query()
    .With<Component>()      // Filter: must have component
    .Without<Component>()   // Filter: must not have component
    .Get<Components...>();  // Execute query
```

<a name="access-policy-builder"></a>

### Access Policy Builder

```cpp
AccessPolicy()
    .Query<Components...>()
    .ReadResources<Resources...>()
    .WriteResources<Resources...>();
```

---

<a name="best-practices"></a>

## Best Practices

1. **Prefer read-only access** - Use `const` references when possible to enable parallelization
2. **Minimize archetype transitions** - Adding/removing components is expensive
3. **Batch operations** - Use command buffers to defer mutations
4. **Query efficiently** - More specific queries are faster
5. **Use tag components** - Zero-size components for filtering
6. **Declare access correctly** - Accurate `GetAccessPolicy()` enables better parallelization

<a name="anti-patterns"></a>

### Anti-patterns

**Avoid Random entity access**

```cpp
for (auto entity : all_entities) {
  auto* component = world.get<Component>(entity);  // Slow!
}
```

**Use sequential iteration instead**

```cpp
auto query = ctx.Query().Get<Component&>();
query.ForEach([](Component& comp) {
  // Fast!
});
```

**Avoid frequent component add/remove**

```cpp
entity_cmd.RemoveComponent<A>();
entity_cmd.AddComponent<B>();  // Triggers archetype change
```

**Use flags/states instead**

```cpp
struct State {
  enum class Value { A, B, C };
  Value current = Value::A;
};
```

---

<a name="testing"></a>

## Testing

The Core module has comprehensive test coverage:

- **Unit tests**: `tests/core/unit/` - Test individual components
- **Integration tests**: `tests/core/integration/app/app_integration.cpp` - Full application scenarios

Run tests:

```bash
make build BUILD_TESTS=1
make test BUILD_TYPE=<type> PLATFORM=<platform>
```

See [tests/core/integration/app/app_integration.cpp](../../tests/core/integration/app/app_integration.cpp) for complete, working examples of all features.

---

For more information:

- **[Main README](../../README.md)** - Project overview
- **[Integration Tests](../../tests/core/integration/app/app_integration.cpp)** - Working examples
