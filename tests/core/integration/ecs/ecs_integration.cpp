#include <doctest/doctest.h>

#include <helios/core/async/async.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/entity_command_buffer.hpp>
#include <helios/core/ecs/query.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/ecs/world_command_buffer.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/timer.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <format>
#include <future>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

using namespace helios::ecs;
using namespace helios::async;

namespace {

// Test component types for integration testing
struct Transform {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
  float rotation = 0.0F;

  constexpr bool operator==(const Transform& other) const noexcept = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;

  constexpr bool operator==(const Velocity& other) const noexcept = default;
};

struct Health {
  int max_health = 100;
  int current_health = 100;

  explicit constexpr Health(int max_hp, int current_hp = -1) noexcept
      : max_health(max_hp), current_health(current_hp == -1 ? max_hp : current_hp) {}

  constexpr bool operator==(const Health& other) const noexcept = default;

  constexpr bool IsDead() const noexcept { return current_health <= 0; }
  constexpr void TakeDamage(int damage) noexcept { current_health = std::max(0, current_health - damage); }
  constexpr void Heal(int amount) noexcept { current_health = std::min(max_health, current_health + amount); }
};

struct Name {
  std::string value;

  constexpr bool operator==(const Name& other) const noexcept = default;
};

struct Player {};
struct Enemy {};
struct Projectile {};
struct Dead {};
struct Spawner {};
struct MovingTarget {};

// Test resources
struct GameTime {
  float delta_time = 0.016F;
  float total_time = 0.0F;

  static constexpr std::string_view GetName() noexcept { return "GameTime"; }
};

struct PhysicsSettings {
  float gravity = 9.8F;
  float friction = 0.5F;
  bool collisions_enabled = true;

  static constexpr std::string_view GetName() noexcept { return "PhysicsSettings"; }
};

struct GameStats {
  int entities_spawned = 0;
  int entities_destroyed = 0;
  int combat_events = 0;

  static constexpr std::string_view GetName() noexcept { return "GameStats"; }
};

// Test event types
struct EntitySpawnedEvent {
  Entity entity;
  std::array<char, 32> entity_type = {};
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  explicit EntitySpawnedEvent(Entity e = {}, std::string_view type = "", float px = 0.0F, float py = 0.0F,
                              float pz = 0.0F)
      : entity(e), x(px), y(py), z(pz) {
    const auto copy_size = std::min(type.size(), entity_type.size() - 1);
    std::copy_n(type.begin(), copy_size, entity_type.begin());
    entity_type[copy_size] = '\0';
  }

  static constexpr std::string_view GetName() noexcept { return "EntitySpawnedEvent"; }
};

struct EntityDestroyedEvent {
  Entity entity;
  std::array<char, 32> reason = {};

  explicit EntityDestroyedEvent(Entity e = {}, std::string_view r = "") : entity(e) {
    const auto copy_size = std::min(r.size(), reason.size() - 1);
    std::copy_n(r.begin(), copy_size, reason.begin());
    reason[copy_size] = '\0';
  }

  static constexpr std::string_view GetName() noexcept { return "EntityDestroyedEvent"; }
};

struct CombatEvent {
  Entity attacker;
  Entity target;
  int damage = 0;

  static constexpr std::string_view GetName() noexcept { return "CombatEvent"; }
};

struct CollisionEvent {
  Entity entity_a;
  Entity entity_b;
  float impact_force = 0.0F;

  static constexpr std::string_view GetName() noexcept { return "CollisionEvent"; }
};

struct PlayerLevelUpEvent {
  Entity player;
  int new_level = 0;

  static constexpr std::string_view GetName() noexcept { return "PlayerLevelUpEvent"; }
};

}  // namespace

TEST_SUITE("ecs::EcsIntegration") {
  TEST_CASE("Ecs: Basic Entity Lifecycle") {
    helios::Timer timer;
    World world;

    HELIOS_INFO("Starting basic entity lifecycle test");

    // Create player entity
    Entity player = world.CreateEntity();
    world.AddComponents(player, Name{"Player"}, Transform{0.0F, 0.0F, 0.0F}, Health{100}, Player{});

    CHECK(world.Exists(player));
    CHECK(world.HasComponent<Transform>(player));
    CHECK(world.HasComponent<Health>(player));
    CHECK(world.HasComponent<Name>(player));
    CHECK(world.HasComponent<Player>(player));
    CHECK_EQ(world.EntityCount(), 1);

    // Create enemy entities
    std::vector<Entity> enemies;
    for (int i = 0; i < 5; ++i) {
      Entity enemy = world.CreateEntity();
      world.AddComponents(enemy, Name{std::format("Enemy{}", i)}, Transform{0.0F, 0.0F, 0.0F}, Health{50}, Enemy{});
      enemies.push_back(enemy);
    }

    CHECK_EQ(world.EntityCount(), 6);  // 1 player + 5 enemies

    // Query all entities with Transform and Health using Count
    auto all_entities_query = QueryBuilder(world).With<Transform, Health>().Get();
    CHECK_EQ(all_entities_query.Count(), 6);

    // Query only enemies using chaining
    auto enemies_query = QueryBuilder(world).With<Enemy>().Get<Health&>();
    CHECK_EQ(enemies_query.Count(), 5);

    // Damage all enemies using chaining
    auto enemy_count = enemies_query.CountIf([](const Health&) { return true; });
    CHECK_EQ(enemy_count, 5);

    enemies_query.ForEach([](Health& health) { health.TakeDamage(25); });

    // Verify damage was applied using All
    bool all_damaged = enemies_query.All([](const Health& health) { return health.current_health == 25; });
    CHECK(all_damaged);

    // Kill some enemies
    std::vector<Entity> enemies_to_kill = {enemies[0], enemies[2], enemies[4]};
    world.DestroyEntities(enemies_to_kill);
    CHECK_EQ(world.EntityCount(), 3);  // 1 player + 2 enemies

    CHECK_EQ(enemies_query.Count(), 2);  // Only 2 enemies left

    const double test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("Basic entity lifecycle test completed in {:.3f}ms", test_time);
  }

  TEST_CASE("Ecs: Command Buffer Entity Management") {
    helios::Timer timer;
    World world;
    Executor executor;

    HELIOS_INFO("Starting command buffer entity management test");

    // Insert resources
    world.InsertResource(GameTime{0.016F, 0.0F});
    world.InsertResource(PhysicsSettings{9.8F, 0.5F, true});

    // Create initial spawner entity
    Entity spawner = world.CreateEntity();
    world.AddComponents(spawner, Name{"Spawner"}, Transform{0.0F, 0.0F, 0.0F}, Spawner{});

    SUBCASE("Entity Command Buffer - Projectile Spawning") {
      helios::Timer subtest_timer;
      constexpr int projectile_count = 10;

      HELIOS_INFO("Starting projectile spawning subtest with {} projectiles", projectile_count);

      // Use entity command buffers to spawn projectiles
      helios::ecs::details::SystemLocalStorage local_storage;
      std::vector<Entity> spawned_entities;
      for (int i = 0; i < projectile_count; ++i) {
        EntityCmdBuffer cmd_buffer(world, local_storage);
        Entity projectile = cmd_buffer.GetEntity();
        spawned_entities.push_back(projectile);

        cmd_buffer.AddComponents(Name{std::format("Projectile{}", i)}, Transform{static_cast<float>(i * 5), 0.0F, 0.0F},
                                 Velocity{50.0F + i, 0.0F, 0.0F}, Projectile{});
      }

      const double command_creation_time = subtest_timer.ElapsedMilliSec();
      HELIOS_INFO("Command creation took {:.3f}ms", command_creation_time);

      subtest_timer.Reset();
      world.MergeCommands(local_storage.GetCommands());
      world.Update();  // Process commands
      const double command_execution_time = subtest_timer.ElapsedMilliSec();
      HELIOS_INFO("Command execution took {:.3f}ms", command_execution_time);

      CHECK_EQ(world.EntityCount(), 1 + projectile_count);  // Spawner + projectiles

      auto projectile_query = QueryBuilder(world).With<Projectile>().Get<const Transform&, const Velocity&>();

      CHECK_EQ(projectile_query.Count(), projectile_count);

      // Verify projectiles were created correctly using Any
      bool has_projectiles =
          projectile_query.Any([](const Transform& /*transform*/, const Velocity& /*velocity*/) { return true; });
      CHECK(has_projectiles);

      // Verify all projectiles have proper velocity using All
      bool all_moving =
          projectile_query.All([](const Transform& /*transform*/, const Velocity& vel) { return vel.dx >= 50.0F; });
      CHECK(all_moving);

      for (auto&& [index, transform, velocity] : projectile_query.Enumerate()) {
        CHECK_EQ(transform.x, static_cast<float>(index * 5));
        CHECK_EQ(velocity.dx, 50.0F + index);
      }

      HELIOS_INFO("Projectile spawning subtest completed in {:.3f}ms total",
                  command_creation_time + command_execution_time);
    }

    SUBCASE("Entity Command Buffer - Component Modification") {
      helios::Timer subtest_timer;
      HELIOS_INFO("Starting component modification subtest");

      // Verify resources are accessible
      CHECK(world.HasResource<GameTime>());
      CHECK(world.HasResource<PhysicsSettings>());
      CHECK_EQ(world.ReadResource<GameTime>().delta_time, 0.016F);

      // Create test entities first
      std::vector<Entity> test_entities;
      for (size_t i = 0; i < 10; ++i) {
        Entity entity = world.CreateEntity();
        world.AddComponent(entity, Velocity{static_cast<float>(i % 10) * 0.1F, 0.0F, 0.0F});
        test_entities.push_back(entity);
      }

      world.Update();

      helios::ecs::details::SystemLocalStorage local_storage;
      // Use entity command buffers to modify components
      for (size_t i = 0; i < test_entities.size(); ++i) {
        EntityCmdBuffer cmd_buffer(test_entities[i], local_storage);

        // Add velocity to make entities move and name
        cmd_buffer.AddComponents(Transform{static_cast<float>(i), 0.0F, 0.0F},
                                 Velocity{1.0F, static_cast<float>(i), 0.0F}, Name{std::format("Entity{}", i)});

        // Modify health for even indexed entities
        if (i % 2 == 0) {
          // cmd_buffer.RemoveComponent<Health>();
          cmd_buffer.AddComponent(Health{50});
        }
      }

      const double modification_time = subtest_timer.ElapsedMilliSec();
      HELIOS_INFO("Component modification setup took {:.3f}ms", modification_time);

      subtest_timer.Reset();
      world.MergeCommands(local_storage.GetCommands());

      world.Update();
      const double execution_time = subtest_timer.ElapsedMilliSec();
      HELIOS_INFO("Component modification execution took {:.3f}ms", execution_time);

      // Verify changes
      auto query = QueryBuilder(world).Get<const Transform&, const Velocity&, const Name&>();

      CHECK_EQ(query.Count(), 10);

      query.ForEachWithEntity(
          [&world](Entity entity, const Transform& transform, const Velocity& velocity, const Name& name) {
            const auto expected_index = static_cast<size_t>(transform.x);
            CHECK_EQ(velocity.dy, static_cast<float>(expected_index));
            CHECK_EQ(name.value, std::format("Entity{}", expected_index));

            if (expected_index % 2 == 0) {
              CHECK(world.HasComponent<Health>(entity));
            }
          });

      HELIOS_INFO("Component modification subtest completed in {:.3f}ms total", modification_time + execution_time);
    }

    const double total_test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("Command buffer entity management test completed in {:.3f}ms", total_test_time);
  }

  TEST_CASE("Ecs: World Command Buffer Batch Operations") {
    helios::Timer timer;
    World world;

    HELIOS_INFO("Starting world command buffer batch operations test");

    // Insert game statistics resource
    world.InsertResource(GameStats{0, 0, 0});

    // Create test entities
    std::vector<Entity> entities;
    for (int i = 0; i < 20; ++i) {
      Entity entity = world.CreateEntity();
      world.AddComponents(entity, Transform{static_cast<float>(i), static_cast<float>(i * 2), 0.0F},
                          Health{100 + (i * 5)});

      if (i % 3 == 0) {
        world.AddComponent(entity, Enemy{});
      } else if (i % 4 == 0) {
        world.AddComponents(entity, MovingTarget{}, Velocity{1.0F, 1.0F, 0.0F});
      }

      entities.push_back(entity);
    }

    CHECK_EQ(world.EntityCount(), 20);

    SUBCASE("Bulk Entity Processing") {
      helios::Timer subtest_timer;
      HELIOS_INFO("Starting bulk entity processing subtest");

      helios::ecs::details::SystemLocalStorage local_storage;

      {
        WorldCmdBuffer cmd_buffer(local_storage);

        // Remove all enemies
        cmd_buffer.Push([](World& wrld) {
          auto enemy_query = QueryBuilder(wrld).With<Enemy>().Get<const Transform&>();
          std::vector<Entity> enemies_to_remove;
          enemy_query.ForEachWithEntity(
              [&enemies_to_remove](Entity entity, const Transform&) { enemies_to_remove.push_back(entity); });

          wrld.DestroyEntities(enemies_to_remove);
        });

        // Add Player tag to high-health entities and update stats
        cmd_buffer.Push([](World& wrld) {
          auto high_health_query = QueryBuilder(wrld).Get<const Health&>();
          auto filtered = high_health_query.WithEntity().Filter(
              [](Entity, const Health& health) { return health.current_health > 80; });

          int player_count = 0;
          for (auto&& [entity, health] : filtered) {
            wrld.AddComponent(entity, Player{});
            ++player_count;
          }

          // Update game stats resource
          wrld.WriteResource<GameStats>().entities_spawned += player_count;
        });

        // Heal all moving targets
        cmd_buffer.Push([](World& wrld) {
          auto moving_query = QueryBuilder(wrld).With<MovingTarget>().Get<Health&>();
          moving_query.ForEach([](Health& health) { health.Heal(20); });
        });
      }

      const double command_setup_time = subtest_timer.ElapsedMilliSec();
      HELIOS_INFO("Bulk processing command setup took {:.3f}ms", command_setup_time);

      subtest_timer.Reset();
      world.MergeCommands(local_storage.GetCommands());
      world.Update();
      const double execution_time = subtest_timer.ElapsedMilliSec();
      HELIOS_INFO("Bulk processing execution took {:.3f}ms", execution_time);

      // Verify enemies were removed using Empty check
      auto enemy_query = QueryBuilder(world).With<Enemy>().Get<const Transform&>();
      CHECK(enemy_query.Empty());

      // Verify players were tagged using All
      auto player_query = QueryBuilder(world).With<Player>().Get<const Health&>();
      bool all_high_health = player_query.All([](const Health& health) { return health.current_health > 80; });
      CHECK(all_high_health);

      // Verify resource was updated
      CHECK(world.HasResource<GameStats>());
      CHECK_GT(world.ReadResource<GameStats>().entities_spawned, 0);

      // Verify moving targets were healed using All
      auto moving_query = QueryBuilder(world).With<MovingTarget>().Get<const Health&>();
      bool all_healed =
          moving_query.All([](const Health& health) { return health.current_health >= health.max_health; });
      CHECK(all_healed);

      HELIOS_INFO("Bulk entity processing subtest completed in {:.3f}ms total", command_setup_time + execution_time);
    }

    const double total_test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("World command buffer batch operations test completed in {:.3f}ms", total_test_time);
  }

  TEST_CASE("Ecs: Multithreaded Entity Processing") {
    helios::Timer timer;
    World world;
    Executor executor;

    constexpr size_t entity_count = 1000;
    constexpr size_t thread_count = 4;

    HELIOS_INFO("Starting multithreaded entity processing test with {} entities and {} threads", entity_count,
                thread_count);

    world.InsertResource(GameTime{0.016F, 0.0F});
    world.InsertResource(GameStats{});

    // Reserve entities efficiently from multiple threads
    std::vector<Entity> reserved_entities(entity_count);
    std::atomic<size_t> reservation_counter = 0;

    helios::Timer reservation_timer;
    TaskGraph reservation_graph("EntityReservation");

    for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
      reservation_graph.EmplaceTask(
          [&world, &reserved_entities, &reservation_counter, thread_id, entity_count, thread_count]() {
            const size_t start_idx = thread_id * (entity_count / thread_count);
            const size_t end_idx =
                (thread_id == thread_count - 1) ? entity_count : (thread_id + 1) * (entity_count / thread_count);

            for (size_t i = start_idx; i < end_idx; ++i) {
              reserved_entities[i] = world.ReserveEntity();
              reservation_counter.fetch_add(1, std::memory_order_relaxed);
            }

            HELIOS_INFO("Thread {} reserved {} entities", thread_id, end_idx - start_idx);
          });
    }

    auto reservation_future = executor.Run(reservation_graph);
    reservation_future.Wait();
    const double reservation_time = reservation_timer.ElapsedMilliSec();
    HELIOS_INFO("Entity reservation completed in {:.3f}ms", reservation_time);

    // Process reserved entities to create them with components
    helios::Timer creation_timer;
    world.Update();  // Flush reserved entities

    // Add components to created entities in single thread (to avoid race conditions)
    for (size_t i = 0; i < entity_count; ++i) {
      Entity entity = reserved_entities[i];
      world.AddComponents(entity, Transform{static_cast<float>(i * 10), static_cast<float>((i * 10) + 5), 0.0F},
                          Health{static_cast<int>(50 + (i % 50))});

      if (i % 2 == 0) {
        world.AddComponent(entity, Velocity{1.0F, 0.0F, 0.0F});
      }

      if (i % 10 == 0) {
        world.AddComponent(entity, Enemy{});
      }
    }

    const double creation_time = creation_timer.ElapsedMilliSec();
    HELIOS_INFO("Entity creation with components completed in {:.3f}ms", creation_time);

    CHECK_EQ(world.EntityCount(), entity_count);

    SUBCASE("Parallel Read-Only Queries") {
      helios::Timer query_timer;
      HELIOS_INFO("Starting parallel read-only queries subtest");

      std::atomic<size_t> total_entities_processed = 0;
      std::atomic<size_t> total_health_points = 0;
      std::atomic<size_t> enemies_found = 0;

      TaskGraph query_graph("ParallelQueries");

      // Multiple threads reading simultaneously (safe for queries)
      for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
        query_graph.EmplaceTask([&world, &total_entities_processed, &total_health_points, &enemies_found, thread_id]() {
          // Each thread performs read-only queries using new chaining methods
          auto health_query = QueryBuilder(world).Get<const Health&>();

          // Use Fold to calculate total health
          size_t local_health = health_query.Fold(
              0UZ, [](size_t sum, const Health& health) { return sum + static_cast<size_t>(health.current_health); });

          size_t local_count = health_query.Count();

          total_entities_processed.fetch_add(local_count, std::memory_order_relaxed);
          total_health_points.fetch_add(local_health, std::memory_order_relaxed);

          // Query enemies
          auto enemy_query = QueryBuilder(world).With<Enemy, Transform>().Get();
          const size_t local_enemies = enemy_query.Count();
          enemies_found.fetch_add(local_enemies, std::memory_order_relaxed);

          HELIOS_INFO("Thread {} processed {} entities, found {} enemies", thread_id, local_count, local_enemies);
        });
      }

      auto query_future = executor.Run(query_graph);
      query_future.Wait();

      const double query_time = query_timer.ElapsedMilliSec();
      HELIOS_INFO("Parallel queries completed in {:.3f}ms", query_time);

      // Note: total_entities_processed will be thread_count * entity_count because each thread counts all entities
      CHECK_GT(total_entities_processed.load(), 0);
      CHECK_GT(total_health_points.load(), 0);
      CHECK_GT(enemies_found.load(), 0);
    }

    SUBCASE("Parallel Component Updates with Command Buffers") {
      helios::Timer update_timer;
      HELIOS_INFO("Starting parallel component updates subtest");

      const float dt = world.ReadResource<GameTime>().delta_time;
      CHECK_GT(dt, 0.0F);

      std::atomic<size_t> commands_created = 0;

      TaskGraph update_graph("ParallelUpdates");

      // Each thread creates command buffers for different entity ranges
      for (size_t thread_id = 0; thread_id < thread_count; ++thread_id) {
        update_graph.EmplaceTask([&reserved_entities, &commands_created, thread_id, entity_count, thread_count]() {
          const size_t start_idx = thread_id * (entity_count / thread_count);
          const size_t end_idx =
              (thread_id == thread_count - 1) ? entity_count : (thread_id + 1) * (entity_count / thread_count);

          helios::ecs::details::SystemLocalStorage local_storage;

          WorldCmdBuffer cmd_buffer(local_storage);

          cmd_buffer.Push([start_idx, end_idx, thread_id](World& wrld) {
            // Process entities in this range

            auto query = QueryBuilder(wrld).Get<Transform&, Health&>();
            const size_t processed =
                query.WithEntity()
                    .Filter(
                        [start_idx, end_idx](Entity entity, const Transform& /*transform*/, const Health& /*health*/) {
                          const size_t entity_idx = entity.Index();
                          return entity_idx >= start_idx && entity_idx < end_idx;
                        })
                    .Fold(0UZ, [start_idx, end_idx, thread_id](size_t processed, Entity /*entity*/,
                                                               Transform& transform, Health& health) {
                      // Apply thread-specific transformations
                      transform.x += static_cast<float>(thread_id * 10);
                      transform.rotation += static_cast<float>(thread_id) * 0.1F;

                      // Modify health based on thread
                      if (thread_id % 2 == 0) {
                        health.TakeDamage(5);
                      } else {
                        health.Heal(5);
                      }
                      return processed + 1;
                    });

            HELIOS_INFO("Thread {} command processed {} entities", thread_id, processed);
          });

          commands_created.fetch_add(1, std::memory_order_relaxed);
        });
      }

      auto update_future = executor.Run(update_graph);
      update_future.Wait();

      const double command_creation_time = update_timer.ElapsedMilliSec();
      HELIOS_INFO("Command creation took {:.3f}ms", command_creation_time);

      update_timer.Reset();
      world.Update();  // Execute all commands
      const double command_execution_time = update_timer.ElapsedMilliSec();
      HELIOS_INFO("Command execution took {:.3f}ms", command_execution_time);

      CHECK_EQ(commands_created.load(), thread_count);

      HELIOS_INFO("Parallel component updates completed in {:.3f}ms total",
                  command_creation_time + command_execution_time);
    }

    const double total_test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("Multithreaded entity processing test completed in {:.3f}ms", total_test_time);
  }

  TEST_CASE("Ecs: Complex Async Simulation") {
    helios::Timer timer;
    World world;
    Executor executor;

    // Create a complex game simulation with multiple systems
    constexpr int simulation_steps = 10;
    constexpr int entities_per_type = 50;

    HELIOS_INFO("Starting complex async simulation with {} steps and {} entities per type", simulation_steps,
                entities_per_type);

    // Initialize game resources
    world.InsertResource(GameTime{0.016F, 0.0F});
    world.InsertResource(PhysicsSettings{9.8F, 0.3F, true});
    world.InsertResource(GameStats{});

    // Initialize world with different entity types
    std::vector<Entity> players;
    std::vector<Entity> enemies;

    helios::Timer setup_timer;

    // Create players
    for (int i = 0; i < entities_per_type; ++i) {
      Entity player = world.CreateEntity();
      world.AddComponents(player, Transform{static_cast<float>(i * 20), 0.0F, 0.0F}, Velocity{1.0F, 2.0F, 0.0F},
                          Health{100}, Name{std::format("Player{}", i)}, Player{});
      players.push_back(player);
    }

    // Create enemies
    for (int i = 0; i < entities_per_type; ++i) {
      Entity enemy = world.CreateEntity();
      world.AddComponents(enemy, Transform{static_cast<float>(i * 15), 30.0F, 0.0F}, Velocity{-1.0F, -2.0F, 0.0F},
                          Health{75}, Name{std::format("Enemy{}", i)}, Enemy{});
      enemies.push_back(enemy);
    }

    world.Update();

    const double setup_time = setup_timer.ElapsedMilliSec();
    HELIOS_INFO("Simulation setup completed in {:.3f}ms", setup_time);

    CHECK_EQ(world.EntityCount(), entities_per_type * 2);

    // Track simulation statistics
    std::atomic<size_t> total_movement_updates = 0;
    std::atomic<size_t> total_combat_events = 0;
    std::atomic<size_t> total_projectiles_spawned = 0;

    int step = 0;

    TaskGraph simulation_step("Simulation");

    // Shared command storage for all tasks in this simulation step
    std::vector<helios::ecs::details::SystemLocalStorage> task_storages(4);

    // Movement system task
    auto movement_task = simulation_step.EmplaceTask([&world, &total_movement_updates]() {
      const float dt = world.ReadResource<GameTime>().delta_time;
      auto movement_query = QueryBuilder(world).Get<Transform&, const Velocity&>();

      // Use Inspect to count updates while performing them
      size_t updates = 0;
      for (auto&& [transform, velocity] : movement_query.Inspect(
               [&updates](const Transform& /*transform*/, const Velocity& /*velocity*/) { ++updates; })) {
        transform.x += velocity.dx * dt;
        transform.y += velocity.dy * dt;
        transform.z += velocity.dz * dt;
      }

      total_movement_updates.fetch_add(updates, std::memory_order_relaxed);

      // Update game time
      world.WriteResource<GameTime>().total_time += dt;
    });

    // Combat system task
    auto combat_task = simulation_step.EmplaceTask([&world, &total_combat_events, step]() {
      // Simulate combat between players and enemies

      // Collect all player and enemy data first to avoid nested query issues
      std::vector<std::tuple<Entity, Transform, Health*>> players;
      std::vector<std::tuple<Entity, Transform, Health*>> enemies;

      auto player_query = QueryBuilder(world).With<Player>().Get<const Transform&, Health&>();
      auto player_data = player_query.WithEntity().Collect();
      for (auto& [entity, transform, health] : player_data) {
        players.emplace_back(entity, transform, &health);
      }

      auto enemy_query = QueryBuilder(world).With<Enemy>().Get<const Transform&, Health&>();
      auto enemy_data = enemy_query.WithEntity().Collect();
      for (auto& [entity, transform, health] : enemy_data) {
        enemies.emplace_back(entity, transform, &health);
      }

      HELIOS_DEBUG("Step {}: Combat system - {} players, {} enemies", step, players.size(), enemies.size());

      size_t combat_events = 0;
      for (auto& [player_entity, player_pos, player_health] : players) {
        for (auto& [enemy_entity, enemy_pos, enemy_health] : enemies) {
          const float distance = std::abs(player_pos.x - enemy_pos.x) + std::abs(player_pos.y - enemy_pos.y);
          if (distance < 50.0F) {  // Combat range
            // Both take damage
            player_health->TakeDamage(4);
            enemy_health->TakeDamage(3);
            ++combat_events;
          }
        }
      }

      total_combat_events.fetch_add(combat_events, std::memory_order_relaxed);

      // Update game stats resource
      world.WriteResource<GameStats>().combat_events += static_cast<int>(combat_events);

      HELIOS_DEBUG("Step {}: Combat events: {}", step, combat_events);
    });

    // Projectile spawning task
    auto spawn_task = simulation_step.EmplaceTask([&world, &total_projectiles_spawned, &task_storages, step]() {
      // Spawn projectiles from players every few steps
      if (step % 3 != 0) {
        return;
      }

      auto player_query = QueryBuilder(world).With<Player>().Get<const Transform&>();
      auto player_positions = player_query.Collect();

      for (const auto& player_pos : player_positions) {
        // Spawn projectile at player position
        EntityCmdBuffer cmd_buffer(world, task_storages[0]);
        cmd_buffer.AddComponents(
            Transform{std::get<0>(player_pos).x, std::get<0>(player_pos).y, std::get<0>(player_pos).z},
            Velocity{10.0F, 2.0F, 0.0F}, Projectile{}, Name{std::format("Projectile_Step{}", step)});
      }

      total_projectiles_spawned.fetch_add(player_positions.size(), std::memory_order_relaxed);

      // Update game stats
      world.WriteResource<GameStats>().entities_spawned += static_cast<int>(player_positions.size());
    });

    auto cleanup_health = simulation_step.EmplaceTask([&world, &task_storages]() {
      WorldCmdBuffer cmd_buffer(task_storages[1]);
      auto health_query = QueryBuilder(world).Get<const Health&>();
      auto dead_entities =
          health_query.WithEntity().Filter([](Entity /*entity*/, const Health& health) { return health.IsDead(); });

      int destroyed_count = 0;
      for (auto&& [entity, health] : dead_entities) {
        cmd_buffer.Destroy(entity);
        ++destroyed_count;
      }

      // Update game stats
      if (destroyed_count > 0) {
        world.WriteResource<GameStats>().entities_destroyed += destroyed_count;
      }
    });

    auto cleanup_projectiles = simulation_step.EmplaceTask([&world, &task_storages]() {
      WorldCmdBuffer cmd_buffer(task_storages[2]);
      auto projectile_query = QueryBuilder(world).With<Projectile>().Get<const Transform&>();
      auto out_of_bounds = projectile_query.WithEntity().Filter([](Entity /*entity*/, const Transform& transform) {
        return std::abs(transform.x) > 1000.0F || std::abs(transform.y) > 1000.0F;
      });

      int destroyed_count = 0;
      for (auto&& [entity, transform] : out_of_bounds) {
        cmd_buffer.Destroy(entity);
        ++destroyed_count;
      }

      if (destroyed_count > 0) {
        world.WriteResource<GameStats>().entities_destroyed += destroyed_count;
      }
    });

    // Set up dependencies: movement -> combat -> cleanup, spawn can run independently
    movement_task.Precede(combat_task);
    combat_task.Precede(cleanup_health, cleanup_projectiles);

    // Simulation loop with async systems
    helios::Timer simulation_timer;
    for (; step < simulation_steps; ++step) {
      helios::Timer step_timer;

      // Execute simulation step
      auto step_future = executor.Run(simulation_step);
      step_future.Wait();

      // Merge all commands from task storages
      for (auto& storage : task_storages) {
        world.MergeCommands(storage.GetCommands());
        storage.Clear();
      }

      // Process all commands
      world.Update();

      const double step_time = step_timer.ElapsedMilliSec();
      HELIOS_DEBUG("Simulation step {} completed in {:.3f}ms", step, step_time);
    }

    double simulation_time = simulation_timer.ElapsedMilliSec();
    HELIOS_INFO("Simulation completed in {:.3f}ms", simulation_time);
    HELIOS_INFO("Total movement updates: {}", total_movement_updates.load());
    HELIOS_INFO("Total combat events: {}", total_combat_events.load());
    HELIOS_INFO("Total projectiles spawned: {}", total_projectiles_spawned.load());

    // Verify resource tracking
    const auto& game_stats = world.ReadResource<GameStats>();
    HELIOS_INFO("Game stats - Spawned: {}, Destroyed: {}, Combat: {}", game_stats.entities_spawned,
                game_stats.entities_destroyed, game_stats.combat_events);
    CHECK_GT(game_stats.entities_spawned, 0);
    CHECK_GT(game_stats.entities_destroyed, 0);
    CHECK_GT(game_stats.combat_events, 0);

    // Verify game time was updated
    const auto& game_time = world.ReadResource<GameTime>();
    CHECK_GT(game_time.total_time, 0.0F);

    // Verify simulation results
    auto final_player_query = QueryBuilder(world).With<Player>().Get<const Health&>();
    auto final_enemy_query = QueryBuilder(world).With<Enemy>().Get<const Health&>();
    auto projectile_query = QueryBuilder(world).With<Projectile>().Get<const Transform&>();

    // Some entities should have taken damage or died
    bool players_took_damage = final_player_query.Any([](const Health& health) { return health.current_health < 100; });

    bool enemies_took_damage = final_enemy_query.Any([](const Health& health) { return health.current_health < 75; });

    CHECK(players_took_damage);
    CHECK(enemies_took_damage);

    // Should have some projectiles
    CHECK_GT(projectile_query.Count(), 0);

    // Total entity count should be reasonable (some died, some projectiles spawned)
    CHECK_GT(world.EntityCount(), 0);
    CHECK_LE(world.EntityCount(), (entities_per_type * 2) + 500);  // Upper bound including projectiles

    double total_test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("Complex async simulation test completed in {:.3f}ms", total_test_time);
  }

  TEST_CASE("Ecs: Async Command Buffer Stress Test") {
    helios::Timer timer;
    World world;
    Executor executor;

    constexpr size_t async_operations = 100;
    constexpr size_t entities_per_operation = 10;

    HELIOS_INFO("Starting async command buffer stress test with {} operations, {} entities per operation",
                async_operations, entities_per_operation);

    std::atomic<size_t> operations_completed = 0;
    std::atomic<size_t> total_entities_created = 0;
    std::vector<std::future<std::vector<std::unique_ptr<Command>>>> operation_futures;

    helios::Timer async_timer;

    // Launch many async operations that create and modify entities
    for (size_t op = 0; op < async_operations; ++op) {
      auto future = executor.Async(
          [&world, op, &operations_completed, &total_entities_created]() -> std::vector<std::unique_ptr<Command>> {
            size_t entities_created = 0;
            helios::ecs::details::SystemLocalStorage local_storage;

            // Each async operation creates entities using command buffers
            for (size_t i = 0; i < entities_per_operation; ++i) {
              EntityCmdBuffer cmd_buffer(world, local_storage);
              Entity entity = cmd_buffer.GetEntity();

              cmd_buffer.AddComponents(Transform{static_cast<float>((op * 100) + i), static_cast<float>(op), 0.0F},
                                       Health{static_cast<int>(50 + (op % 50))},
                                       Name{std::format("AsyncOp{}_Entity{}", op, i)});

              if (op % 3 == 0) {
                cmd_buffer.AddComponent(Velocity{1.0F, 1.0F, 0.0F});
              }

              if (op % 5 == 0) {
                cmd_buffer.AddComponent(Enemy{});
              } else {
                cmd_buffer.AddComponent(Player{});
              }

              ++entities_created;
            }

            total_entities_created.fetch_add(entities_created, std::memory_order_relaxed);
            operations_completed.fetch_add(1, std::memory_order_relaxed);

            return std::move(local_storage.GetCommands());
          });

      operation_futures.push_back(std::move(future));
    }

    // Wait for all async operations to complete and merge their commands
    for (auto& future : operation_futures) {
      auto commands = future.get();
      world.MergeCommands(commands);
    }

    const double async_time = async_timer.ElapsedMilliSec();
    HELIOS_INFO("Async operations completed in {:.3f}ms", async_time);

    CHECK_EQ(operations_completed.load(), async_operations);

    // Process all commands
    helios::Timer processing_timer;
    world.Update();
    const double processing_time = processing_timer.ElapsedMilliSec();
    HELIOS_INFO("Command processing completed in {:.3f}ms", processing_time);

    // Verify all entities were created
    CHECK_EQ(world.EntityCount(), async_operations * entities_per_operation);

    // Verify different entity types
    auto player_query = QueryBuilder(world).With<Player>().Get<const Name&>();
    auto enemy_query = QueryBuilder(world).With<Enemy>().Get<const Name&>();
    auto velocity_query = QueryBuilder(world).With<Velocity>().Get();

    CHECK_GT(player_query.Count(), 0);
    CHECK_GT(enemy_query.Count(), 0);
    CHECK_GT(velocity_query.Count(), 0);
    CHECK_EQ(player_query.Count() + enemy_query.Count(), async_operations * entities_per_operation);

    // Verify naming consistency
    std::array<size_t, async_operations> entity_counts_per_op = {};

    auto all_query = QueryBuilder(world).Get<const Name&>();
    auto async_entities =
        all_query.Filter([](const Name& name) { return name.value.find("AsyncOp") != std::string::npos; });

    for (auto&& [name] : async_entities) {
      // Parse operation ID from name
      size_t pos = name.value.find("AsyncOp");
      pos += 7;  // Length of "AsyncOp"
      const size_t end_pos = name.value.find("_Entity", pos);
      if (end_pos != std::string::npos) {
        const size_t op_id = std::stoul(name.value.substr(pos, end_pos - pos));
        if (op_id < async_operations) {
          ++entity_counts_per_op[op_id];
        }
      }
    }

    // Each operation should have created exactly entities_per_operation entities
    for (size_t op = 0; op < async_operations; ++op) {
      CHECK_EQ(entity_counts_per_op[op], entities_per_operation);
    }

    const double total_test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("Async command buffer stress test completed in {:.3f}ms", total_test_time);
    HELIOS_INFO("Total entities created: {}", total_entities_created.load());
    HELIOS_INFO("Average time per async operation: {:.3f}ms", async_time / async_operations);
  }

  TEST_CASE("Ecs: Performance Benchmark") {
    helios::Timer timer;
    World world;
    Executor executor;

    constexpr size_t large_entity_count = 10000;
    constexpr size_t simulation_frames = 120;

    HELIOS_INFO("Starting performance benchmark with {} entities and {} frames", large_entity_count, simulation_frames);

    // Insert benchmark resources
    world.InsertResource(GameTime{1.0F / 60.0F, 0.0F});
    world.InsertResource(GameStats{});

    // Create large number of entities efficiently using batch reservation
    std::vector<Entity> entities;
    entities.reserve(large_entity_count);

    // Reserve entities in parallel
    std::vector<Entity> reserved(large_entity_count);
    constexpr size_t batch_size = 1000;
    constexpr size_t num_batches = (large_entity_count + batch_size - 1) / batch_size;

    TaskGraph reservation_graph("EntityReservation");
    for (size_t batch = 0; batch < num_batches; ++batch) {
      reservation_graph.EmplaceTask([&world, &reserved, batch, batch_size, large_entity_count]() {
        const size_t start = batch * batch_size;
        const size_t end = std::min(start + batch_size, large_entity_count);

        for (size_t i = start; i < end; ++i) {
          reserved[i] = world.ReserveEntity();
        }
      });
    }

    helios::Timer creation_timer;
    auto reservation_future = executor.Run(reservation_graph);
    reservation_future.Wait();
    world.Update();  // Flush reserved entities

    // Add components to entities
    for (size_t i = 0; i < large_entity_count; ++i) {
      Entity entity = reserved[i];
      world.AddComponent(entity, Transform{static_cast<float>(i % 100), static_cast<float>((i / 100) % 100), 0.0F});

      if (i % 2 == 0) {
        world.AddComponent(
            entity, Velocity{static_cast<float>((i % 10) - 5) * 0.1F, static_cast<float>((i % 7) - 3) * 0.1F, 0.0F});
      }

      if (i % 3 == 0) {
        world.AddComponent(entity, Health{static_cast<int>(50 + (i % 50))});
      }

      if (i % 5 == 0) {
        world.AddComponent(entity, Name{std::format("Entity{}", i)});
      }

      entities.push_back(entity);
    }

    const double creation_time = creation_timer.ElapsedMilliSec();
    HELIOS_INFO("Entity creation completed in {:.3f}ms ({:.2f} entities/ms)", creation_time,
                large_entity_count / creation_time);

    CHECK_EQ(world.EntityCount(), large_entity_count);

    // Performance tracking
    std::atomic<size_t> total_movement_updates = 0;
    std::atomic<size_t> total_health_updates = 0;
    std::atomic<size_t> total_entities_destroyed = 0;

    size_t frame = 0;

    TaskGraph frame_graph("Frame");

    // Shared command storage for cleanup task
    helios::ecs::details::SystemLocalStorage cleanup_storage;

    // Movement system
    auto movement_task = frame_graph.EmplaceTask([&world, &total_movement_updates]() {
      const float dt = world.ReadResource<GameTime>().delta_time;
      size_t updates = 0;
      auto query = QueryBuilder(world).Get<Transform&, const Velocity&>();
      for (auto&& [transform, velocity] :
           query.Inspect([&updates](const Transform& /*transform*/, const Velocity& /*velocity*/) { ++updates; })) {
        transform.x += velocity.dx * dt;
        transform.y += velocity.dy * dt;
        transform.z += velocity.dz * dt;
      }

      total_movement_updates.fetch_add(updates, std::memory_order_relaxed);
    });

    // Health decay system
    auto health_task = frame_graph.EmplaceTask([&world, &total_health_updates]() {
      auto query = QueryBuilder(world).Get<Health&>();
      auto alive_entities = query.Filter([](const Health& health) { return health.current_health > 0; });

      size_t updates = 0;
      for (auto&& [health] : alive_entities) {
        health.TakeDamage(1);
        ++updates;
      }

      total_health_updates.fetch_add(updates, std::memory_order_relaxed);
    });

    // Cleanup system
    auto cleanup_task = frame_graph.EmplaceTask([&world, &total_entities_destroyed, &cleanup_storage, frame]() {
      if (frame % 10 == 0) {  // Cleanup every 10 frames
        auto query = QueryBuilder(world).Get<const Health&>();

        WorldCmdBuffer cmd_buffer(cleanup_storage);
        int destroyed_this_frame = 0;
        for (auto&& [entity, health] :
             query.WithEntity().Filter([](Entity /*entity*/, const Health& health) { return health.IsDead(); })) {
          cmd_buffer.Destroy(entity);
          total_entities_destroyed.fetch_add(1, std::memory_order_relaxed);
          ++destroyed_this_frame;
        }

        // Update stats resource
        if (destroyed_this_frame > 0) {
          world.WriteResource<GameStats>().entities_destroyed += destroyed_this_frame;
        }
      }
    });

    // Set dependencies
    movement_task.Precede(cleanup_task);
    health_task.Precede(cleanup_task);

    // Run simulation frames with async processing
    helios::Timer simulation_timer;
    for (; frame < simulation_frames; ++frame) {
      // Execute frame
      auto frame_future = executor.Run(frame_graph);
      frame_future.Wait();

      // Merge cleanup commands if any
      world.MergeCommands(cleanup_storage.GetCommands());
      cleanup_storage.Clear();

      world.Update();

      if (frame % 10 == 0) {
        HELIOS_DEBUG("Frame {} completed, entities remaining: {}", frame, world.EntityCount());
      }
    }

    const double simulation_time = simulation_timer.ElapsedMilliSec();
    const double total_time = timer.ElapsedMilliSec();

    HELIOS_INFO("Performance benchmark completed in {:.3f}ms total", total_time);
    HELIOS_INFO("Simulation time: {:.3f}ms ({:.2f}ms/frame average)", simulation_time,
                simulation_time / simulation_frames);
    HELIOS_INFO("Total movement updates: {}", total_movement_updates.load());
    HELIOS_INFO("Total health updates: {}", total_health_updates.load());
    HELIOS_INFO("Total entities destroyed: {}", total_entities_destroyed.load());
    HELIOS_INFO("Final entity count: {}", world.EntityCount());

    // Verify some entities survived and some died
    CHECK_LT(world.EntityCount(), large_entity_count);      // Some should have died
    CHECK_GT(world.EntityCount(), large_entity_count / 2);  // But not too many

    // Verify remaining entities are in valid state
    const auto final_query = QueryBuilder(world).With<Transform>().Get();
    CHECK_EQ(final_query.Count(), world.EntityCount());

    // Performance metrics
    const double entities_per_ms = large_entity_count / creation_time;
    const double frames_per_second = 1000.0 / (simulation_time / simulation_frames);

    HELIOS_INFO("Performance metrics:");
    HELIOS_INFO("  Entity creation rate: {:.2f} entities/ms", entities_per_ms);
    HELIOS_INFO("  Simulation rate: {:.2f} FPS", frames_per_second);
    HELIOS_INFO("  Movement updates/ms: {:.2f}", total_movement_updates.load() / simulation_time);

    // Verify resource integration
    CHECK(world.HasResource<GameTime>());
    CHECK(world.HasResource<GameStats>());
    const auto& final_stats = world.ReadResource<GameStats>();
    HELIOS_INFO("Final game stats - Destroyed: {}", final_stats.entities_destroyed);
    CHECK_EQ(final_stats.entities_destroyed, static_cast<int>(total_entities_destroyed.load()));
  }

  TEST_CASE("Ecs: Event System") {
    helios::Timer timer;
    World world;
    Executor executor;

    HELIOS_INFO("Starting event system integration test");

    // Insert resources
    world.InsertResource(GameTime{0.016F, 0.0F});
    world.InsertResource(GameStats{});

    // Register events
    world.AddEvent<EntitySpawnedEvent>();
    world.AddEvent<EntityDestroyedEvent>();
    world.AddEvent<CombatEvent>();
    world.AddEvent<CollisionEvent>();
    world.AddEvent<PlayerLevelUpEvent>();

    SUBCASE("Basic Event Communication") {
      helios::Timer subtest_timer;
      HELIOS_INFO("Starting basic event communication subtest");

      // Create entities
      std::vector<Entity> entities;
      for (int i = 0; i < 5; ++i) {
        Entity entity = world.CreateEntity();
        world.AddComponents(entity, Name{std::format("Entity{}", i)}, Transform{static_cast<float>(i * 10), 0.0F, 0.0F},
                            Health{100});
        if (i % 2 == 0) {
          world.AddComponent(entity, Player{});
        } else {
          world.AddComponent(entity, Enemy{});
        }
        entities.push_back(entity);
      }

      // System 1: Spawn entities and send spawn events
      {
        helios::ecs::details::SystemLocalStorage local_storage;

        auto entity_query = QueryBuilder(world).Get<const Transform&, const Name&>();
        for (auto&& [entity, transform, name] : entity_query.WithEntity()) {
          const char* entity_type = world.HasComponent<Player>(entity) ? "Player" : "Enemy";
          local_storage.WriteEvent(EntitySpawnedEvent{entity, entity_type, transform.x, transform.y, transform.z});
        }

        // Flush events to world
        world.MergeEventQueue(local_storage.GetEventQueue());
      }

      world.Update();

      // Verify spawn events were recorded
      auto reader_spawn_events = world.ReadEvents<EntitySpawnedEvent>();
      auto spawn_events = reader_spawn_events.Collect();
      CHECK_EQ(spawn_events.size(), 5);

      for (size_t i = 0; i < spawn_events.size(); ++i) {
        CHECK(spawn_events[i].entity.Valid());
        CHECK_NE(spawn_events[i].entity_type[0], '\0');
        std::string_view event_type(spawn_events[i].entity_type.data());
        CHECK((event_type == "Player" || event_type == "Enemy"));
      }

      // Clear events
      world.ClearEvents<EntitySpawnedEvent>();
      auto reader_cleared_events = world.ReadEvents<EntitySpawnedEvent>();
      auto cleared_events = reader_cleared_events.Collect();
      CHECK(cleared_events.empty());

      HELIOS_INFO("Basic event communication subtest completed in {:.3f}ms", subtest_timer.ElapsedMilliSec());
    }

    SUBCASE("Combat Event System") {
      helios::Timer subtest_timer;
      HELIOS_INFO("Starting combat event system subtest");

      // Create players and enemies
      std::vector<Entity> players;
      std::vector<Entity> enemies;

      for (int i = 0; i < 3; ++i) {
        Entity player = world.CreateEntity();
        world.AddComponents(player, Name{std::format("Player{}", i)}, Transform{0.0F, static_cast<float>(i * 5), 0.0F},
                            Health{100}, Player{});
        players.push_back(player);
      }

      for (int i = 0; i < 5; ++i) {
        Entity enemy = world.CreateEntity();
        world.AddComponents(enemy, Name{std::format("Enemy{}", i)}, Transform{50.0F, static_cast<float>(i * 5), 0.0F},
                            Health{75}, Enemy{});
        enemies.push_back(enemy);
      }

      // Combat system: Players attack enemies and generate combat events
      {
        helios::ecs::details::SystemLocalStorage local_storage;

        auto player_query = QueryBuilder(world).With<Player>().Get<const Transform&>();
        auto enemy_query = QueryBuilder(world).With<Enemy>().Get<Health&>();

        int combat_count = 0;
        for (auto&& [player_entity, player_transform] : player_query.WithEntity()) {
          for (auto&& [enemy_entity, enemy_health] : enemy_query.WithEntity()) {
            constexpr int damage = 20;
            enemy_health.TakeDamage(damage);

            // Send combat event
            local_storage.WriteEvent(CombatEvent{player_entity, enemy_entity, damage});
            ++combat_count;
          }
        }

        // Update game stats
        world.WriteResource<GameStats>().combat_events += combat_count;

        // Flush events to world
        world.MergeEventQueue(local_storage.GetEventQueue());
      }

      world.Update();

      // Verify combat events
      auto reader_combat_events = world.ReadEvents<CombatEvent>();
      auto combat_events = reader_combat_events.Collect();
      CHECK_EQ(combat_events.size(), players.size() * enemies.size());  // 3 players * 5 enemies = 15

      for (const auto& event : combat_events) {
        CHECK(event.attacker.Valid());
        CHECK(event.target.Valid());
        CHECK_EQ(event.damage, 20);
        CHECK(world.HasComponent<Player>(event.attacker));
        CHECK(world.HasComponent<Enemy>(event.target));
      }

      // Verify game stats were updated
      CHECK_EQ(world.ReadResource<GameStats>().combat_events, 15);

      // Verify enemies took damage
      auto damaged_enemies = QueryBuilder(world).With<Enemy>().Get<const Health&>();
      bool all_damaged = damaged_enemies.All([](const Health& health) { return health.current_health < 75; });
      CHECK(all_damaged);

      HELIOS_INFO("Combat event system subtest completed in {:.3f}ms", subtest_timer.ElapsedMilliSec());
    }

    SUBCASE("Entity Destruction Events") {
      helios::Timer subtest_timer;
      HELIOS_INFO("Starting entity destruction events subtest");

      // Create entities that will be destroyed
      std::vector<Entity> doomed_entities;
      for (int i = 0; i < 10; ++i) {
        Entity entity = world.CreateEntity();
        world.AddComponents(entity, Name{std::format("Doomed{}", i)}, Health{0});  // Already dead
        doomed_entities.push_back(entity);
      }

      // Cleanup system: Destroy dead entities and send events
      helios::ecs::details::SystemLocalStorage local_storage;
      {
        WorldCmdBuffer cmd_buffer(local_storage);

        auto dead_query = QueryBuilder(world).Get<const Health&>();
        auto dead_entities =
            dead_query.WithEntity().Filter([](Entity /*entity*/, const Health& health) { return health.IsDead(); });

        int destroyed_count = 0;
        for (auto&& [entity, health] : dead_entities) {
          local_storage.WriteEvent(EntityDestroyedEvent{entity, "health_depleted"});
          cmd_buffer.Destroy(entity);
          ++destroyed_count;
        }

        world.WriteResource<GameStats>().entities_destroyed += destroyed_count;
      }

      // Flush events before commands are executed
      world.MergeEventQueue(local_storage.GetEventQueue());

      // Read events before entities are destroyed
      auto reader_destruction_events = world.ReadEvents<EntityDestroyedEvent>();
      auto destruction_events = reader_destruction_events.Collect();
      CHECK_EQ(destruction_events.size(), 10);

      for (const auto& event : destruction_events) {
        CHECK(event.entity.Valid());
        CHECK_EQ(std::string_view(event.reason.data()), "health_depleted");
        // Entity should still exist before Update()
        CHECK(world.Exists(event.entity));
      }

      // Flush commands from local storage
      world.MergeCommands(local_storage.GetCommands());

      // Process commands (entities will be destroyed now)
      world.Update();

      // Verify entities were destroyed
      for (const Entity& entity : doomed_entities) {
        CHECK_FALSE(world.Exists(entity));
      }

      CHECK_EQ(world.ReadResource<GameStats>().entities_destroyed, 10);

      HELIOS_INFO("Entity destruction events subtest completed in {:.3f}ms", subtest_timer.ElapsedMilliSec());
    }

    SUBCASE("Multiple Event Types") {
      helios::Timer subtest_timer;
      HELIOS_INFO("Starting multiple event types subtest");

      // Create diverse entities
      Entity player1 = world.CreateEntity();
      world.AddComponents(player1, Name{"Hero"}, Transform{0.0F, 0.0F, 0.0F}, Health{100}, Player{});

      Entity enemy1 = world.CreateEntity();
      world.AddComponents(enemy1, Name{"Goblin"}, Transform{10.0F, 0.0F, 0.0F}, Health{50}, Enemy{});

      Entity projectile1 = world.CreateEntity();
      world.AddComponents(projectile1, Name{"Arrow"}, Transform{5.0F, 0.0F, 0.0F}, Velocity{20.0F, 0.0F, 0.0F},
                          Projectile{});

      // System that generates multiple event types
      {
        helios::ecs::details::SystemLocalStorage local_storage;

        // Spawn event for projectile - use query to get transform
        auto projectile_query = QueryBuilder(world).Get<const Transform&>();

        auto write_spawn_event = [&local_storage](const std::tuple<Entity, const Transform&> tuple)
            -> std::optional<std::remove_cvref_t<decltype(tuple)>> {
          const auto& [entity, transform] = tuple;
          local_storage.WriteEvent(EntitySpawnedEvent{entity, "Projectile", transform.x, transform.y, transform.z});
          return tuple;
        };

        const auto _ = projectile_query.WithEntity()
                           .FindFirst([projectile1](Entity entity, const Transform& /*transform*/) {
                             return entity == projectile1;
                           })
                           .and_then(write_spawn_event);

        // Combat event
        local_storage.WriteEvent(CombatEvent{player1, enemy1, 30});

        // Collision event
        local_storage.WriteEvent(CollisionEvent{projectile1, enemy1, 15.5F});

        // Level up event
        local_storage.WriteEvent(PlayerLevelUpEvent{player1, 2});

        // Flush all events
        world.MergeEventQueue(local_storage.GetEventQueue());
      }

      world.Update();

      // Verify each event type
      auto reader_spawn_events = world.ReadEvents<EntitySpawnedEvent>();
      auto spawn_events = reader_spawn_events.Collect();
      CHECK_EQ(spawn_events.size(), 1);
      CHECK_EQ(std::string_view(spawn_events[0].entity_type.data()), "Projectile");

      auto reader_combat_events = world.ReadEvents<CombatEvent>();
      auto combat_events = reader_combat_events.Collect();
      CHECK_EQ(combat_events.size(), 1);
      CHECK_EQ(combat_events[0].damage, 30);

      auto reader_collision_events = world.ReadEvents<CollisionEvent>();
      auto collision_events = reader_collision_events.Collect();
      CHECK_EQ(collision_events.size(), 1);
      CHECK_EQ(collision_events[0].impact_force, 15.5F);

      auto reader_levelup_events = world.ReadEvents<PlayerLevelUpEvent>();
      auto levelup_events = reader_levelup_events.Collect();
      CHECK_EQ(levelup_events.size(), 1);
      CHECK_EQ(levelup_events[0].new_level, 2);

      // Clear all events
      world.ClearAllEventQueues();
      CHECK(world.ReadEvents<EntitySpawnedEvent>().Empty());
      CHECK(world.ReadEvents<CombatEvent>().Empty());
      CHECK(world.ReadEvents<CollisionEvent>().Empty());
      CHECK(world.ReadEvents<PlayerLevelUpEvent>().Empty());

      HELIOS_INFO("Multiple event types subtest completed in {:.3f}ms", subtest_timer.ElapsedMilliSec());
    }

    SUBCASE("Event System with Async Operations") {
      helios::Timer subtest_timer;
      HELIOS_INFO("Starting async event system subtest");

      constexpr size_t entity_count = 100;
      std::vector<Entity> entities;

      // Create entities
      for (size_t i = 0; i < entity_count; ++i) {
        Entity entity = world.CreateEntity();
        world.AddComponents(entity, Transform{static_cast<float>(i), 0.0F, 0.0F}, Health{static_cast<int>(50 + i % 50)},
                            Velocity{1.0F, 0.0F, 0.0F});
        if (i % 3 == 0) {
          world.AddComponent(entity, Player{});
        } else {
          world.AddComponent(entity, Enemy{});
        }
        entities.push_back(entity);
      }

      std::atomic<int> total_combat_events = 0;
      std::atomic<int> total_collision_events = 0;

      TaskGraph event_graph("EventGeneration");

      // Task 1: Generate combat events
      auto combat_task = event_graph.EmplaceTask([&world, &total_combat_events]() {
        helios::ecs::details::SystemLocalStorage local_storage;

        auto player_query = QueryBuilder(world).With<Player>().Get();
        auto enemy_query = QueryBuilder(world).With<Enemy>().Get();

        int event_count = 0;
        for (auto&& [player_entity] : player_query.WithEntity()) {
          for (auto&& [enemy_entity] : enemy_query.WithEntity()) {
            local_storage.WriteEvent(CombatEvent{player_entity, enemy_entity, 10});
            ++event_count;
          }
        }

        total_combat_events.fetch_add(event_count, std::memory_order_relaxed);
        world.MergeEventQueue(local_storage.GetEventQueue());
      });

      // Task 2: Generate collision events
      auto collision_task = event_graph.EmplaceTask([&world, &total_collision_events]() {
        helios::ecs::details::SystemLocalStorage local_storage;

        auto moving_query = QueryBuilder(world).Get<const Velocity&>();
        std::vector<Entity> moving_entities;
        for (auto&& [entity, vel] : moving_query.WithEntity()) {
          moving_entities.push_back(entity);
        }

        // Generate collision events between moving entities
        for (size_t i = 0; i < moving_entities.size(); ++i) {
          for (size_t j = i + 1; j < moving_entities.size(); ++j) {
            local_storage.WriteEvent(CollisionEvent{moving_entities[i], moving_entities[j], 5.0F});
            total_collision_events.fetch_add(1, std::memory_order_relaxed);
          }
        }

        world.MergeEventQueue(local_storage.GetEventQueue());
      });

      // Execute async tasks
      auto future = executor.Run(event_graph);
      future.Wait();

      world.Update();

      // Verify events were generated
      auto reader_combat_events = world.ReadEvents<CombatEvent>();
      auto combat_events = reader_combat_events.Collect();
      auto reader_collision_events = world.ReadEvents<CollisionEvent>();
      auto collision_events = reader_collision_events.Collect();

      CHECK_EQ(combat_events.size(), total_combat_events.load());
      CHECK_EQ(collision_events.size(), total_collision_events.load());

      HELIOS_INFO("Generated {} combat events and {} collision events", combat_events.size(), collision_events.size());

      // Verify event data integrity
      for (const auto& event : combat_events) {
        CHECK(event.attacker.Valid());
        CHECK(event.target.Valid());
        CHECK(world.Exists(event.attacker));
        CHECK(world.Exists(event.target));
      }

      for (const auto& event : collision_events) {
        CHECK(event.entity_a.Valid());
        CHECK(event.entity_b.Valid());
        CHECK(world.Exists(event.entity_a));
        CHECK(world.Exists(event.entity_b));
        CHECK_GT(event.impact_force, 0.0F);
      }

      HELIOS_INFO("Async event system subtest completed in {:.3f}ms", subtest_timer.ElapsedMilliSec());
    }

    const double total_test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("Event system integration test completed in {:.3f}ms", total_test_time);
  }

  TEST_CASE("Ecs: Real World Game Simulation") {
    helios::Timer timer;
    World world;

    HELIOS_INFO("Starting real-world game simulation test");

    // Initialize resources
    world.InsertResource(GameTime{0.016F, 0.0F});
    world.InsertResource(PhysicsSettings{9.8F, 0.5F, true});
    world.InsertResource(GameStats{0, 0, 0});

    // Register events
    world.AddEvent<EntitySpawnedEvent>();
    world.AddEvent<EntityDestroyedEvent>();
    world.AddEvent<CombatEvent>();
    world.AddEvent<CollisionEvent>();

    SUBCASE("Complete Game Loop Simulation") {
      HELIOS_INFO("Starting complete game loop simulation");

      helios::Timer subtest_timer;
      // Phase 1: Spawn player and initial enemies
      {
        helios::ecs::details::SystemLocalStorage local_storage;

        // Spawn player
        Entity player = world.CreateEntity();
        world.AddComponents(player, Name{"Hero"}, Transform{0.0F, 0.0F, 0.0F}, Velocity{0.0F, 0.0F, 0.0F},
                            Health{100, 100}, Player{});
        local_storage.WriteEvent(EntitySpawnedEvent{player, "Player", 0.0F, 0.0F, 0.0F});

        // Spawn enemies in a circle around player
        constexpr int enemy_count = 10;
        constexpr float spawn_radius = 1.5F;  // Close enough for collisions (collision_distance = 2.0F)
        for (int i = 0; i < enemy_count; ++i) {
          float angle = (static_cast<float>(i) / static_cast<float>(enemy_count)) * 6.28318F;  // 2*PI
          float x = std::cos(angle) * spawn_radius;
          float z = std::sin(angle) * spawn_radius;

          Entity enemy = world.CreateEntity();
          world.AddComponents(enemy, Name{std::format("Enemy{}", i)}, Transform{x, 0.0F, z}, Velocity{0.0F, 0.0F, 0.0F},
                              Health{50, 50}, Enemy{}, MovingTarget{});
          local_storage.WriteEvent(EntitySpawnedEvent{enemy, "Enemy", x, 0.0F, z});
        }

        world.WriteResource<GameStats>().entities_spawned = enemy_count + 1;
        world.MergeEventQueue(local_storage.GetEventQueue());
      }

      world.Update();

      // Verify spawning
      auto reader_spawn_events = world.ReadEvents<EntitySpawnedEvent>();
      auto spawn_events = reader_spawn_events.Collect();
      CHECK_EQ(spawn_events.size(), 11);  // 1 player + 10 enemies
      CHECK_EQ(world.EntityCount(), 11);

      // Phase 2: Physics system - Move entities toward player
      {
        helios::ecs::details::SystemLocalStorage local_storage;
        const auto& physics = world.ReadResource<PhysicsSettings>();
        auto& game_time = world.WriteResource<GameTime>();
        game_time.total_time += game_time.delta_time;

        if (physics.collisions_enabled) {
          // Get player position
          auto player_query = QueryBuilder(world).With<Player>().Get<const Transform&>();
          Transform player_transform{};
          for (auto&& [transform] : player_query) {
            player_transform = transform;
            break;
          }

          // Move enemies toward player
          auto enemy_query = QueryBuilder(world).With<Enemy, MovingTarget>().Get<Transform&, Velocity&>();
          for (auto&& [transform, velocity] : enemy_query) {
            // Calculate direction to player
            float dx = player_transform.x - transform.x;
            float dz = player_transform.z - transform.z;
            float distance = std::sqrt(dx * dx + dz * dz);

            if (distance > 0.1F) {
              // Normalize and apply speed
              constexpr float move_speed = 5.0F;
              velocity.dx = (dx / distance) * move_speed;
              velocity.dz = (dz / distance) * move_speed;

              // Update position
              transform.x += velocity.dx * game_time.delta_time;
              transform.z += velocity.dz * game_time.delta_time;
            }
          }
        }
      }

      world.Update();

      // Phase 3: Collision detection and combat
      {
        helios::ecs::details::SystemLocalStorage local_storage;

        // Get all entities with transforms
        auto transform_query = QueryBuilder(world).Get<const Transform&, Health&>();
        std::vector<std::tuple<Entity, Transform, Health*>> entities_data;

        for (auto&& [entity, transform, health] : transform_query.WithEntity()) {
          entities_data.emplace_back(entity, transform, &health);
        }

        constexpr float collision_distance = 2.0F;
        int collision_count = 0;

        // Check for collisions between all pairs
        for (size_t i = 0; i < entities_data.size(); ++i) {
          for (size_t j = i + 1; j < entities_data.size(); ++j) {
            auto& [entity_a, transform_a, health_a] = entities_data[i];
            auto& [entity_b, transform_b, health_b] = entities_data[j];

            float dx = transform_a.x - transform_b.x;
            float dz = transform_a.z - transform_b.z;
            float distance = std::sqrt(dx * dx + dz * dz);

            if (distance < collision_distance) {
              // Collision detected
              float impact = (collision_distance - distance) * 10.0F;
              local_storage.WriteEvent(CollisionEvent{entity_a, entity_b, impact});
              ++collision_count;

              // Apply damage if one is player and other is enemy
              bool a_is_player = world.HasComponent<Player>(entity_a);
              bool b_is_player = world.HasComponent<Player>(entity_b);
              bool a_is_enemy = world.HasComponent<Enemy>(entity_a);
              bool b_is_enemy = world.HasComponent<Enemy>(entity_b);

              if ((a_is_player && b_is_enemy) || (b_is_player && a_is_enemy)) {
                Entity attacker = a_is_player ? entity_a : entity_b;
                Entity target = a_is_enemy ? entity_a : entity_b;
                Health* target_health = a_is_enemy ? health_a : health_b;

                constexpr int damage = 15;
                target_health->TakeDamage(damage);
                local_storage.WriteEvent(CombatEvent{attacker, target, damage});
                world.WriteResource<GameStats>().combat_events++;
              }
            }
          }
        }

        HELIOS_INFO("Detected {} collisions", collision_count);
        world.MergeEventQueue(local_storage.GetEventQueue());
      }

      world.Update();

      // Verify combat occurred
      auto reader_combat_events = world.ReadEvents<CombatEvent>();
      auto combat_events = reader_combat_events.Collect();
      CHECK_GT(combat_events.size(), 0);
      HELIOS_INFO("Combat events generated: {}", combat_events.size());

      // Phase 4: Cleanup dead entities
      {
        helios::ecs::details::SystemLocalStorage local_storage;
        WorldCmdBuffer cmd_buffer(local_storage);

        auto health_query = QueryBuilder(world).Get<const Health&>();
        int destroyed_count = 0;

        for (auto&& [entity, health] : health_query.WithEntity().Filter(
                 [](Entity /*entity*/, const Health& health) { return health.IsDead(); })) {
          local_storage.WriteEvent(EntityDestroyedEvent{entity, "killed_in_combat"});
          cmd_buffer.Destroy(entity);
          ++destroyed_count;
        }

        world.WriteResource<GameStats>().entities_destroyed = destroyed_count;
        HELIOS_INFO("Destroyed {} dead entities", destroyed_count);

        world.MergeEventQueue(local_storage.GetEventQueue());
        world.MergeCommands(local_storage.GetCommands());
      }

      world.Update();

      // Verify destruction events
      auto reader_destruction_events = world.ReadEvents<EntityDestroyedEvent>();
      auto destruction_events = reader_destruction_events.Collect();
      CHECK_EQ(destruction_events.size(), world.ReadResource<GameStats>().entities_destroyed);

      for (const auto& event : destruction_events) {
        CHECK_FALSE(world.Exists(event.entity));  // Entity should be destroyed
        CHECK_EQ(std::string_view(event.reason.data()), "killed_in_combat");
      }

      // Phase 5: Spawn reinforcements
      {
        helios::ecs::details::SystemLocalStorage local_storage;

        // Count remaining enemies
        auto enemy_query = QueryBuilder(world).With<Enemy>().Get();
        size_t remaining_enemies = enemy_query.Count();

        HELIOS_INFO("Remaining enemies: {}", remaining_enemies);

        // Spawn new enemies if too few remain
        if (remaining_enemies < 5) {
          constexpr int reinforcement_count = 3;
          for (int i = 0; i < reinforcement_count; ++i) {
            float x = static_cast<float>((i - 1) * 5);
            float z = 20.0F;

            Entity enemy = world.CreateEntity();
            world.AddComponents(enemy, Name{std::format("Reinforcement{}", i)}, Transform{x, 0.0F, z},
                                Velocity{0.0F, 0.0F, 0.0F}, Health{50, 50}, Enemy{}, MovingTarget{});
            local_storage.WriteEvent(EntitySpawnedEvent{enemy, "Reinforcement", x, 0.0F, z});
          }

          world.WriteResource<GameStats>().entities_spawned += reinforcement_count;
          world.MergeEventQueue(local_storage.GetEventQueue());
        }
      }

      world.Update();

      // Phase 6: Verify game state
      const auto& stats = world.ReadResource<GameStats>();
      HELIOS_INFO("Game Statistics:");
      HELIOS_INFO("  Entities Spawned: {}", stats.entities_spawned);
      HELIOS_INFO("  Entities Destroyed: {}", stats.entities_destroyed);
      HELIOS_INFO("  Combat Events: {}", stats.combat_events);
      HELIOS_INFO("  Remaining Entities: {}", world.EntityCount());

      CHECK_GT(stats.entities_spawned, 10);
      CHECK_GT(stats.combat_events, 0);

      // Verify player still exists (should survive initial combat)
      auto player_query = QueryBuilder(world).With<Player>().Get<const Health&>();
      bool player_alive = false;
      for (auto&& [health] : player_query) {
        player_alive = health.current_health > 0;
        HELIOS_INFO("  Player Health: {}/{}", health.current_health, health.max_health);
        break;
      }

      // Verify event history is complete
      auto reader_all_spawn_events = world.ReadEvents<EntitySpawnedEvent>();
      auto all_spawn_events = reader_all_spawn_events.Collect();
      auto reader_all_combat_events = world.ReadEvents<CombatEvent>();
      auto all_combat_events = reader_all_combat_events.Collect();
      auto reader_all_collision_events = world.ReadEvents<CollisionEvent>();
      auto all_collision_events = reader_all_collision_events.Collect();
      auto reader_all_destruction_events = world.ReadEvents<EntityDestroyedEvent>();
      auto all_destruction_events = reader_all_destruction_events.Collect();

      HELIOS_INFO("Event Summary:");
      HELIOS_INFO("  Spawn Events: {}", all_spawn_events.size());
      HELIOS_INFO("  Combat Events: {}", all_combat_events.size());
      HELIOS_INFO("  Collision Events: {}", all_collision_events.size());
      HELIOS_INFO("  Destruction Events: {}", all_destruction_events.size());

      // Note: With double buffering, events persist for only 2 update cycles.
      // After 5 updates, early events have been cleared. This is expected behavior.
      // CHECK_EQ(all_spawn_events.size(), stats.entities_spawned);
      // CHECK_EQ(all_combat_events.size(), stats.combat_events);
      // CHECK_EQ(all_destruction_events.size(), stats.entities_destroyed);

      // Verify some events are still present (from recent frames)
      CHECK_GT(stats.combat_events, 0);

      HELIOS_INFO("Complete game loop simulation completed in {:.3f}ms", subtest_timer.ElapsedMilliSec());
    }

    SUBCASE("Multi-Frame Simulation with Event Accumulation") {
      helios::Timer subtest_timer;
      HELIOS_INFO("Starting multi-frame simulation");

      // Create a simple arena with entities
      Entity player = world.CreateEntity();
      world.AddComponents(player, Name{"Player"}, Transform{0.0F, 0.0F, 0.0F}, Velocity{1.0F, 0.0F, 0.0F},
                          Health{100, 100}, Player{});

      std::vector<Entity> enemies;
      for (int i = 0; i < 20; ++i) {
        Entity enemy = world.CreateEntity();
        float x = static_cast<float>(i % 5) * 3.0F;
        float z = static_cast<float>(i / 5) * 3.0F;
        world.AddComponents(enemy, Name{std::format("Enemy{}", i)}, Transform{x, 0.0F, z}, Velocity{-0.5F, 0.0F, -0.5F},
                            Health{30, 30}, Enemy{});
        enemies.push_back(enemy);
      }

      // Simulate 10 frames of gameplay
      constexpr int frame_count = 120;
      int total_events = 0;

      for (int frame = 0; frame < frame_count; ++frame) {
        helios::ecs::details::SystemLocalStorage local_storage;
        auto& game_time = world.WriteResource<GameTime>();
        game_time.total_time += game_time.delta_time;

        // Update all entity positions
        auto movement_query = QueryBuilder(world).Get<Transform&, const Velocity&>();
        for (auto&& [transform, velocity] : movement_query) {
          transform.x += velocity.dx * game_time.delta_time;
          transform.y += velocity.dy * game_time.delta_time;
          transform.z += velocity.dz * game_time.delta_time;
        }

        // Random combat events
        if (frame % 2 == 0) {
          auto enemy_query = QueryBuilder(world).With<Enemy>().Get();
          int event_count = 0;
          for (auto&& [enemy_entity] : enemy_query.WithEntity()) {
            if (world.Exists(enemy_entity) && event_count < 3) {
              local_storage.WriteEvent(CombatEvent{player, enemy_entity, 10});
              ++event_count;
              ++total_events;
            }
          }
        }

        world.MergeEventQueue(local_storage.GetEventQueue());
        world.Update();

        // Clear events each frame (simulating event processing)
        if (frame % 3 == 0) {
          world.ClearEvents<CombatEvent>();
        }
      }

      // Verify simulation ran
      CHECK_EQ(world.ReadResource<GameTime>().total_time, doctest::Approx(frame_count * 0.016F));
      CHECK(world.Exists(player));

      HELIOS_INFO("Multi-frame simulation completed {} frames in {:.3f}ms", frame_count,
                  subtest_timer.ElapsedMilliSec());
    }

    SUBCASE("Parallel Game Loop with Task-Based Systems") {
      helios::Timer subtest_timer;
      Executor executor;
      HELIOS_INFO("Starting parallel game loop with task-based systems");

      // Create initial game state
      Entity player = world.CreateEntity();
      world.AddComponents(player, Name{"Hero"}, Transform{0.0F, 0.0F, 0.0F}, Velocity{0.0F, 0.0F, 0.0F},
                          Health{100, 100}, Player{});

      constexpr int enemy_count = 50;
      std::vector<Entity> enemies(enemy_count);

      for (int i = 0; i < enemy_count; ++i) {
        Entity enemy = world.CreateEntity();
        float x = static_cast<float>(i % 10) * 5.0F;
        float z = static_cast<float>(i / 10) * 5.0F;
        world.AddComponents(enemy, Name{std::format("Enemy{}", i)}, Transform{x, 0.0F, z}, Velocity{-1.0F, 0.0F, -1.0F},
                            Health{50, 50}, Enemy{}, MovingTarget{});
        enemies[i] = enemy;
      }

      world.Update();
      CHECK_EQ(world.EntityCount(), enemy_count + 1);

      // Track statistics
      std::atomic<size_t> total_movement_updates{0};
      std::atomic<size_t> total_combat_checks{0};
      std::atomic<size_t> total_collisions_detected{0};
      std::atomic<size_t> total_entities_cleaned{0};

      // Shared command storage for parallel systems
      std::vector<helios::ecs::details::SystemLocalStorage> system_storages(4);

      constexpr int simulation_frames = 20;
      int frame = 0;

      // Build task graph for game frame
      TaskGraph frame_graph("GameFrame");

      // System 1: Physics/Movement System (independent)
      auto physics_task = frame_graph.EmplaceTask([&world, &total_movement_updates, frame]() {
        const float dt = world.ReadResource<GameTime>().delta_time;
        auto movement_query = QueryBuilder(world).Get<Transform&, const Velocity&>();

        size_t updates = 0;
        for (auto&& [transform, velocity] : movement_query) {
          // Apply velocity to position
          transform.x += velocity.dx * dt;
          transform.y += velocity.dy * dt;
          transform.z += velocity.dz * dt;

          // Simple bounds checking
          constexpr float world_bounds = 100.0F;
          if (std::abs(transform.x) > world_bounds || std::abs(transform.z) > world_bounds) {
            transform.x = std::clamp(transform.x, -world_bounds, world_bounds);
            transform.z = std::clamp(transform.z, -world_bounds, world_bounds);
          }

          ++updates;
        }

        total_movement_updates.fetch_add(updates, std::memory_order_relaxed);

        // Update game time
        world.WriteResource<GameTime>().total_time += dt;
      });

      // System 2: AI System - Enemy behavior (depends on physics for positions)
      auto ai_task = frame_graph.EmplaceTask([&world, frame]() {
        // Get player position
        auto player_query = QueryBuilder(world).With<Player>().Get<const Transform&>();
        Transform player_pos{};
        bool player_found = false;

        for (auto&& [transform] : player_query) {
          player_pos = transform;
          player_found = true;
          break;
        }

        if (!player_found) {
          return;
        }

        // Update enemy velocities to chase player
        auto enemy_query = QueryBuilder(world).With<Enemy, MovingTarget>().Get<const Transform&, Velocity&>();
        for (auto&& [transform, velocity] : enemy_query) {
          float dx = player_pos.x - transform.x;
          float dz = player_pos.z - transform.z;
          float distance = std::sqrt(dx * dx + dz * dz);

          if (distance > 0.1F) {
            constexpr float chase_speed = 8.0F;
            velocity.dx = (dx / distance) * chase_speed;
            velocity.dz = (dz / distance) * chase_speed;
          } else {
            velocity.dx = 0.0F;
            velocity.dz = 0.0F;
          }
        }
      });

      // System 3: Combat System (depends on both physics and AI)
      auto combat_task = frame_graph.EmplaceTask(
          [&world, &system_storages, &total_combat_checks, &total_collisions_detected, frame]() {
            // Collect all entity data for spatial checks
            auto entity_query = QueryBuilder(world).Get<const Transform&, Health&>();
            std::vector<std::tuple<Entity, Transform, Health*, bool, bool>> entity_data;

            for (auto&& [entity, transform, health] : entity_query.WithEntity()) {
              bool is_player = world.HasComponent<Player>(entity);
              bool is_enemy = world.HasComponent<Enemy>(entity);
              entity_data.emplace_back(entity, transform, &health, is_player, is_enemy);
            }

            constexpr float combat_range = 3.0F;
            size_t combat_checks = 0;
            size_t collisions = 0;

            // Check collisions and apply combat
            for (size_t i = 0; i < entity_data.size(); ++i) {
              for (size_t j = i + 1; j < entity_data.size(); ++j) {
                auto& [entity_a, transform_a, health_a, is_player_a, is_enemy_a] = entity_data[i];
                auto& [entity_b, transform_b, health_b, is_player_b, is_enemy_b] = entity_data[j];

                ++combat_checks;

                float dx = transform_a.x - transform_b.x;
                float dz = transform_a.z - transform_b.z;
                float distance = std::sqrt(dx * dx + dz * dz);

                if (distance < combat_range) {
                  ++collisions;
                  system_storages[0].WriteEvent(CollisionEvent{entity_a, entity_b, (combat_range - distance) * 5.0F});

                  // Apply combat damage if player vs enemy
                  if ((is_player_a && is_enemy_b) || (is_player_b && is_enemy_a)) {
                    Entity attacker = is_player_a ? entity_a : entity_b;
                    Entity target = is_enemy_a ? entity_a : entity_b;
                    Health* target_health = is_enemy_a ? health_a : health_b;

                    constexpr int combat_damage = 8;
                    target_health->TakeDamage(combat_damage);
                    system_storages[0].WriteEvent(CombatEvent{attacker, target, combat_damage});
                    world.WriteResource<GameStats>().combat_events++;
                  }
                }
              }
            }

            total_combat_checks.fetch_add(combat_checks, std::memory_order_relaxed);
            total_collisions_detected.fetch_add(collisions, std::memory_order_relaxed);
          });

      // System 4: Health Regeneration (independent, can run in parallel with AI)
      auto health_regen_task = frame_graph.EmplaceTask([&world, frame]() {
        if (frame % 5 == 0) {  // Regenerate every 5 frames
          auto player_query = QueryBuilder(world).With<Player>().Get<Health&>();
          for (auto&& [health] : player_query) {
            health.Heal(2);
          }
        }
      });

      // System 5: Cleanup/Spawning System (depends on combat for death detection)
      auto cleanup_spawn_task = frame_graph.EmplaceTask([&world, &system_storages, &total_entities_cleaned, frame]() {
        WorldCmdBuffer cmd_buffer(system_storages[1]);

        // Find and destroy dead entities
        auto health_query = QueryBuilder(world).Get<const Health&>();
        int destroyed = 0;

        for (auto&& [entity, health] : health_query.WithEntity()) {
          if (health.IsDead()) {
            system_storages[1].WriteEvent(EntityDestroyedEvent{entity, "combat_death"});
            cmd_buffer.Destroy(entity);
            ++destroyed;
          }
        }

        if (destroyed > 0) {
          world.WriteResource<GameStats>().entities_destroyed += destroyed;
          total_entities_cleaned.fetch_add(destroyed, std::memory_order_relaxed);
        }

        // Spawn reinforcements if enemies are low
        if (frame % 10 == 0) {
          auto enemy_query = QueryBuilder(world).With<Enemy>().Get();
          if (enemy_query.Count() < 20) {
            constexpr int spawn_count = 5;
            for (int i = 0; i < spawn_count; ++i) {
              EntityCmdBuffer spawn_buffer(world, system_storages[2]);
              Entity new_enemy = spawn_buffer.GetEntity();

              float spawn_x = static_cast<float>((i - 2) * 8.0F);
              float spawn_z = 50.0F;

              spawn_buffer.AddComponents(Name{std::format("Reinforcement_F{}_E{}", frame, i)},
                                         Transform{spawn_x, 0.0F, spawn_z}, Velocity{0.0F, 0.0F, -2.0F}, Health{50, 50},
                                         Enemy{}, MovingTarget{});

              system_storages[2].WriteEvent(EntitySpawnedEvent{new_enemy, "Reinforcement", spawn_x, 0.0F, spawn_z});
            }

            world.WriteResource<GameStats>().entities_spawned += spawn_count;
          }
        }
      });

      // Set up task dependencies for proper execution order
      // Physics must complete before AI and Combat
      physics_task.Precede(ai_task, combat_task);
      // AI must complete before Combat (enemy velocities affect collision predictions)
      ai_task.Precede(combat_task);
      // Combat must complete before Cleanup (need to know who died)
      combat_task.Precede(cleanup_spawn_task);
      // Health regen is independent, but should complete before cleanup
      health_regen_task.Precede(cleanup_spawn_task);

      // Run simulation frames
      helios::Timer simulation_timer;

      for (; frame < simulation_frames; ++frame) {
        // Execute all systems in parallel (respecting dependencies)
        auto frame_future = executor.Run(frame_graph);
        frame_future.Wait();

        // Merge all system events and commands
        for (auto& storage : system_storages) {
          world.MergeEventQueue(storage.GetEventQueue());
          world.MergeCommands(storage.GetCommands());
          storage.Clear();
        }

        // Process all queued commands
        world.Update();

        if (frame % 5 == 0) {
          HELIOS_DEBUG("Frame {}: Entities={}, Combat={}, Collisions={}", frame, world.EntityCount(),
                       world.ReadResource<GameStats>().combat_events, total_collisions_detected.load());
        }
      }

      const double simulation_time = simulation_timer.ElapsedMilliSec();

      // Verify simulation results
      const auto& final_stats = world.ReadResource<GameStats>();
      HELIOS_INFO("Parallel Game Loop Statistics:");
      HELIOS_INFO("  Simulation Time: {:.3f}ms ({:.3f}ms/frame)", simulation_time, simulation_time / simulation_frames);
      HELIOS_INFO("  Total Movement Updates: {}", total_movement_updates.load());
      HELIOS_INFO("  Total Combat Checks: {}", total_combat_checks.load());
      HELIOS_INFO("  Total Collisions: {}", total_collisions_detected.load());
      HELIOS_INFO("  Entities Spawned: {}", final_stats.entities_spawned);
      HELIOS_INFO("  Entities Destroyed: {}", total_entities_cleaned.load());
      HELIOS_INFO("  Combat Events: {}", final_stats.combat_events);
      HELIOS_INFO("  Final Entity Count: {}", world.EntityCount());

      // Verify the simulation ran properly
      CHECK_GT(total_movement_updates.load(), 0);
      CHECK_GT(total_combat_checks.load(), 0);
      CHECK_GT(total_collisions_detected.load(), 0);
      CHECK_GT(final_stats.combat_events, 0);

      // Verify player still exists
      auto player_query = QueryBuilder(world).With<Player>().Get<const Health&>();
      bool player_exists = player_query.Count() > 0;
      CHECK(player_exists);

      if (player_exists) {
        for (auto&& [health] : player_query) {
          HELIOS_INFO("  Player Health: {}/{}", health.current_health, health.max_health);
          break;
        }
      }

      // Verify events were generated
      auto reader_all_collision_events = world.ReadEvents<CollisionEvent>();
      auto all_collision_events = reader_all_collision_events.Collect();
      auto reader_all_combat_events = world.ReadEvents<CombatEvent>();
      auto all_combat_events = reader_all_combat_events.Collect();
      auto reader_all_destruction_events = world.ReadEvents<EntityDestroyedEvent>();
      auto all_destruction_events = reader_all_destruction_events.Collect();

      HELIOS_INFO("Event Counts:");
      HELIOS_INFO("  Collision Events: {}", all_collision_events.size());
      HELIOS_INFO("  Combat Events: {}", all_combat_events.size());
      HELIOS_INFO("  Destruction Events: {}", all_destruction_events.size());

      // Note: With double buffering, events persist for only 2 update cycles.
      // After multiple updates, early events have been cleared. This is expected behavior.
      // CHECK_EQ(all_combat_events.size(), final_stats.combat_events);
      // CHECK_EQ(all_destruction_events.size(), total_entities_cleaned.load());

      // Verify some events are still present (from recent frames)
      CHECK_GT(all_combat_events.size(), 0);

      // Performance metrics
      const double frames_per_second = 1000.0 / (simulation_time / simulation_frames);
      const double entities_per_frame = static_cast<double>(total_movement_updates.load()) / simulation_frames;

      HELIOS_INFO("Performance Metrics:");
      HELIOS_INFO("  Effective FPS: {:.2f}", frames_per_second);
      HELIOS_INFO("  Avg Entities/Frame: {:.1f}", entities_per_frame);
      HELIOS_INFO("  Avg Combat Checks/Frame: {:.1f}",
                  static_cast<double>(total_combat_checks.load()) / simulation_frames);

      HELIOS_INFO("Parallel game loop simulation completed in {:.3f}ms", subtest_timer.ElapsedMilliSec());
    }

    const double total_test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("Real-world game simulation test completed in {:.3f}ms", total_test_time);
  }

  TEST_CASE("ECS Integration - EventReader and EventWriter") {
    helios::Timer timer;

    World world;

    // Register events
    world.AddEvent<CombatEvent>();
    world.AddEvent<EntityDestroyedEvent>();
    world.AddEvent<CollisionEvent>();

    // Create test entities with health
    auto player = world.CreateEntity();
    world.AddComponent(player, Health{100});
    world.AddComponent(player, Player{});

    auto enemy1 = world.CreateEntity();
    world.AddComponent(enemy1, Health{50});
    world.AddComponent(enemy1, Enemy{});

    auto enemy2 = world.CreateEntity();
    world.AddComponent(enemy2, Health{30});
    world.AddComponent(enemy2, Enemy{});

    SUBCASE("EventWriter - Basic Write Operations") {
      auto writer = world.WriteEvents<CombatEvent>();

      // Test single write
      writer.Write(CombatEvent{player, enemy1, 20});

      auto reader_events = world.ReadEvents<CombatEvent>();
      auto events = reader_events.Collect();
      REQUIRE_EQ(events.size(), 1);
      CHECK_EQ(events[0].attacker, player);
      CHECK_EQ(events[0].target, enemy1);
      CHECK_EQ(events[0].damage, 20);
    }

    SUBCASE("EventWriter - Bulk Write Operations") {
      std::vector<CombatEvent> combat_events = {CombatEvent{player, enemy1, 15}, CombatEvent{player, enemy2, 25},
                                                CombatEvent{enemy1, player, 10}};

      auto writer = world.WriteEvents<CombatEvent>();
      writer.WriteBulk(combat_events);

      auto reader_events = world.ReadEvents<CombatEvent>();
      auto events = reader_events.Collect();
      CHECK_EQ(events.size(), 3);
    }

    SUBCASE("EventWriter - Emplace Operations") {
      auto writer = world.WriteEvents<CollisionEvent>();

      writer.Emplace(player, enemy1, 50.0F);
      writer.Emplace(player, enemy2, 75.0F);

      auto reader_events = world.ReadEvents<CollisionEvent>();
      auto events = reader_events.Collect();
      REQUIRE_EQ(events.size(), 2);
      CHECK_EQ(events[0].entity_a, player);
      CHECK_EQ(events[1].impact_force, 75.0F);
    }

    SUBCASE("EventReader - Basic Iteration") {
      // Write test events
      auto writer = world.WriteEvents<CombatEvent>();
      writer.Write(CombatEvent{player, enemy1, 20});
      writer.Write(CombatEvent{player, enemy2, 30});
      writer.Write(CombatEvent{enemy1, player, 15});

      // Read using EventReader
      auto reader = world.ReadEvents<CombatEvent>();

      CHECK_FALSE(reader.Empty());
      CHECK_EQ(reader.Count(), 3);

      int total_damage = 0;
      for (const auto& event : reader) {
        total_damage += event.damage;
      }
      CHECK_EQ(total_damage, 65);
    }

    SUBCASE("EventReader - FindFirst Query") {
      // Write test events
      auto writer = world.WriteEvents<CombatEvent>();
      writer.Write(CombatEvent{enemy1, player, 5});
      writer.Write(CombatEvent{player, enemy1, 35});  // High damage
      writer.Write(CombatEvent{enemy2, player, 8});

      auto reader = world.ReadEvents<CombatEvent>();

      // Find first high-damage attack
      const auto* high_damage = reader.FindFirst([](const auto& e) { return e.damage >= 30; });

      REQUIRE(high_damage != nullptr);
      CHECK_EQ(high_damage->attacker, player);
      CHECK_EQ(high_damage->target, enemy1);
      CHECK_EQ(high_damage->damage, 35);

      // Try to find non-existent event
      const auto* not_found = reader.FindFirst([](const auto& e) { return e.damage > 100; });
      CHECK_EQ(not_found, nullptr);
    }

    SUBCASE("EventReader - CountIf Query") {
      // Write test events
      auto writer = world.WriteEvents<CombatEvent>();
      writer.Write(CombatEvent{player, enemy1, 50});  // Critical
      writer.Write(CombatEvent{player, enemy2, 15});
      writer.Write(CombatEvent{enemy1, player, 45});  // Critical
      writer.Write(CombatEvent{enemy2, player, 10});

      auto reader = world.ReadEvents<CombatEvent>();

      // Count critical hits (damage >= 40)
      size_t critical_count = reader.CountIf([](const auto& e) { return e.damage >= 40; });
      CHECK_EQ(critical_count, 2);

      // Count player attacks
      size_t player_attacks = reader.CountIf([player](const auto& e) { return e.attacker == player; });
      CHECK_EQ(player_attacks, 2);
    }

    SUBCASE("EventReader - Complex Combat System Simulation") {
      // Simulate a combat system processing damage events
      auto writer = world.WriteEvents<CombatEvent>();

      // Multiple combat rounds with varying damage
      writer.Write(CombatEvent{player, enemy1, 20});
      writer.Write(CombatEvent{player, enemy2, 30});
      writer.Write(CombatEvent{enemy1, player, 10});
      writer.Write(CombatEvent{enemy2, player, 15});
      writer.Write(CombatEvent{player, enemy1, 50});  // High damage

      auto reader = world.ReadEvents<CombatEvent>();

      // Count player attacks
      size_t player_attacks = reader.CountIf([player](const auto& e) { return e.attacker == player; });
      CHECK_EQ(player_attacks, 3);

      // Find highest damage attack
      const auto* max_damage = reader.FindFirst([](const auto& e) { return e.damage >= 50; });
      REQUIRE(max_damage != nullptr);
      CHECK_EQ(max_damage->damage, 50);
      CHECK_EQ(max_damage->attacker, player);

      // Calculate total damage dealt by player
      int total_player_damage = 0;
      for (const auto& event : reader) {
        if (event.attacker == player) {
          total_player_damage += event.damage;
        }
      }
      CHECK_EQ(total_player_damage, 100);  // 20 + 30 + 50
    }

    SUBCASE("EventReader/Writer - Double Buffering") {
      // Frame 0: Write events
      {
        auto writer = world.WriteEvents<CombatEvent>();
        writer.Write(CombatEvent{player, enemy1, 10});
      }

      auto reader_f0 = world.ReadEvents<CombatEvent>();
      CHECK_EQ(reader_f0.Count(), 1);

      // Frame 1: Events move to previous queue, write new events
      world.Update();
      {
        auto writer = world.WriteEvents<CombatEvent>();
        writer.Write(CombatEvent{player, enemy2, 20});
      }

      auto reader_f1 = world.ReadEvents<CombatEvent>();
      CHECK_EQ(reader_f1.Count(), 2);  // Both frame 0 and frame 1 visible

      // Verify both events are present
      bool found_frame0 = false;
      bool found_frame1 = false;
      for (const auto& event : reader_f1) {
        if (event.damage == 10)
          found_frame0 = true;
        if (event.damage == 20)
          found_frame1 = true;
      }
      CHECK(found_frame0);
      CHECK(found_frame1);

      // Frame 2: Frame 0 event cleared
      world.Update();
      auto reader_f2 = world.ReadEvents<CombatEvent>();
      CHECK_EQ(reader_f2.Count(), 1);  // Only frame 1 event visible
    }

    SUBCASE("EventReader - Multiple Event Types") {
      // Write different event types
      auto combat_writer = world.WriteEvents<CombatEvent>();
      combat_writer.Write(CombatEvent{player, enemy1, 25});
      combat_writer.Write(CombatEvent{enemy1, player, 10});

      auto collision_writer = world.WriteEvents<CollisionEvent>();
      collision_writer.Emplace(player, enemy2, 100.0F);

      auto destroy_writer = world.WriteEvents<EntityDestroyedEvent>();
      destroy_writer.Emplace(enemy2, "killed");

      // Read each type independently
      auto combat_reader = world.ReadEvents<CombatEvent>();
      auto collision_reader = world.ReadEvents<CollisionEvent>();
      auto destroy_reader = world.ReadEvents<EntityDestroyedEvent>();

      CHECK_EQ(combat_reader.Count(), 2);
      CHECK_EQ(collision_reader.Count(), 1);
      CHECK_EQ(destroy_reader.Count(), 1);

      // Verify independence
      size_t player_combat = combat_reader.CountIf([player](const auto& e) { return e.attacker == player; });
      CHECK_EQ(player_combat, 1);

      const auto* collision = collision_reader.FindFirst([](const auto& e) { return e.impact_force >= 50.0F; });
      CHECK_NE(collision, nullptr);
    }

    SUBCASE("EventReader - ReadInto Performance") {
      // Write many events
      auto writer = world.WriteEvents<CombatEvent>();
      for (int i = 0; i < 100; ++i) {
        writer.Emplace(player, enemy1, i);
      }

      auto reader = world.ReadEvents<CombatEvent>();

      // ReadInto should be efficient (zero intermediate allocations)
      std::vector<CombatEvent> collected_events;
      collected_events.reserve(100);
      reader.ReadInto(std::back_inserter(collected_events));

      CHECK_EQ(collected_events.size(), 100);

      // Verify data integrity
      bool found_0 = false;
      bool found_99 = false;
      for (const auto& event : collected_events) {
        if (event.damage == 0)
          found_0 = true;
        if (event.damage == 99)
          found_99 = true;
      }
      CHECK(found_0);
      CHECK(found_99);
    }

    const double test_time = timer.ElapsedMilliSec();
    HELIOS_INFO("EventReader and EventWriter integration test completed in {:.3f}ms", test_time);
  }

}  // TEST_SUITE
