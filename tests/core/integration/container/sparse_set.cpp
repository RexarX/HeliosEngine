#include <doctest/doctest.h>

#include <helios/core/container/sparse_set.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/uuid.hpp>

#include <algorithm>
#include <chrono>
#include <random>
#include <ranges>
#include <thread>
#include <unordered_map>
#include <vector>

namespace {

// Mock entity ID type for ECS simulation
using EntityId = std::uint32_t;
using ComponentTypeId = std::uint16_t;

// Mock component types for ECS integration testing
struct Transform {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  constexpr bool operator==(const Transform& other) const noexcept = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;

  constexpr bool operator==(const Velocity& other) const noexcept = default;
};

struct Health {
  int current = 100;
  int maximum = 100;

  constexpr bool operator==(const Health& other) const noexcept = default;
};

// ECS Component Registry using SparseSet
template <typename Component>
class ComponentRegistry {
public:
  using ComponentSet = helios::container::SparseSet<Component, EntityId>;

  void AddComponent(EntityId entity, const Component& component) { entities_.Insert(entity, component); }

  void RemoveComponent(EntityId entity) {
    if (entities_.Contains(entity)) {
      entities_.Remove(entity);
    }
  }

  bool HasComponent(EntityId entity) const { return entities_.Contains(entity); }

  Component* GetComponent(EntityId entity) {
    if (!entities_.Contains(entity)) {
      return nullptr;
    }
    return &entities_.Get(entity);
  }

  const Component* GetComponent(EntityId entity) const {
    if (!entities_.Contains(entity)) {
      return nullptr;
    }
    return &entities_.Get(entity);
  }

  const ComponentSet& GetEntities() const { return entities_; }

  size_t Size() const { return entities_.Size(); }

  void Clear() { entities_.Clear(); }

  // Iterator support for components
  auto begin() { return entities_.begin(); }
  auto end() { return entities_.end(); }
  auto begin() const { return entities_.begin(); }
  auto end() const { return entities_.end(); }

private:
  ComponentSet entities_;
};

// Simple ECS World for integration testing
class ECSWorld {
public:
  static constexpr EntityId INVALID_ENTITY = std::numeric_limits<EntityId>::max();

  EntityId CreateEntity() {
    EntityId id = next_entity_id_++;
    entities_.Insert(id, id);
    return id;
  }

  void DestroyEntity(EntityId entity) {
    if (!IsValidEntity(entity)) {
      return;
    }

    transforms_.RemoveComponent(entity);
    velocities_.RemoveComponent(entity);
    healths_.RemoveComponent(entity);
    entities_.Remove(entity);
  }

  bool IsValidEntity(EntityId entity) const { return entities_.Contains(entity); }

  template <typename Component>
  void AddComponent(EntityId entity, const Component& component);

  template <typename Component>
  void RemoveComponent(EntityId entity);

  template <typename Component>
  bool HasComponent(EntityId entity) const;

  template <typename Component>
  Component* GetComponent(EntityId entity);

  void UpdateMovementSystem() {
    // Simple movement system: add velocity to transform
    for (EntityId entity = 0; entity < next_entity_id_; ++entity) {
      if (transforms_.HasComponent(entity) && velocities_.HasComponent(entity)) {
        auto* trans = transforms_.GetComponent(entity);
        auto* vel = velocities_.GetComponent(entity);
        if (trans && vel) {
          trans->x += vel->dx;
          trans->y += vel->dy;
          trans->z += vel->dz;
        }
      }
    }
  }

  void UpdateHealthSystem() {
    // Simple health system: regenerate health
    for (EntityId entity = 0; entity < next_entity_id_; ++entity) {
      if (healths_.HasComponent(entity)) {
        auto* health = healths_.GetComponent(entity);
        if (health && health->current < health->maximum) {
          health->current = std::min(health->current + 1, health->maximum);
        }
      }
    }
  }

  size_t GetEntityCount() const { return entities_.Size(); }
  size_t GetTransformCount() const { return transforms_.Size(); }
  size_t GetVelocityCount() const { return velocities_.Size(); }
  size_t GetHealthCount() const { return healths_.Size(); }

  void Clear() {
    entities_.Clear();
    transforms_.Clear();
    velocities_.Clear();
    healths_.Clear();
    next_entity_id_ = 0;
  }

  // Query entities with specific components
  std::vector<EntityId> QueryEntitiesWithTransform() const {
    std::vector<EntityId> result;
    for (EntityId entity = 0; entity < next_entity_id_; ++entity) {
      if (transforms_.HasComponent(entity)) {
        result.push_back(entity);
      }
    }
    return result;
  }

  std::vector<EntityId> QueryEntitiesWithTransformAndVelocity() const {
    std::vector<EntityId> result;
    for (EntityId entity = 0; entity < next_entity_id_; ++entity) {
      if (transforms_.HasComponent(entity) && velocities_.HasComponent(entity)) {
        result.push_back(entity);
      }
    }
    return result;
  }

private:
  EntityId next_entity_id_ = 0;
  helios::container::SparseSet<EntityId> entities_;

  ComponentRegistry<Transform> transforms_;
  ComponentRegistry<Velocity> velocities_;
  ComponentRegistry<Health> healths_;
};

// Template specializations for component access
template <>
void ECSWorld::AddComponent<Transform>(EntityId entity, const Transform& component) {
  transforms_.AddComponent(entity, component);
}

template <>
void ECSWorld::AddComponent<Velocity>(EntityId entity, const Velocity& component) {
  velocities_.AddComponent(entity, component);
}

template <>
void ECSWorld::AddComponent<Health>(EntityId entity, const Health& component) {
  healths_.AddComponent(entity, component);
}

template <>
void ECSWorld::RemoveComponent<Transform>(EntityId entity) {
  transforms_.RemoveComponent(entity);
}

template <>
void ECSWorld::RemoveComponent<Velocity>(EntityId entity) {
  velocities_.RemoveComponent(entity);
}

template <>
void ECSWorld::RemoveComponent<Health>(EntityId entity) {
  healths_.RemoveComponent(entity);
}

template <>
bool ECSWorld::HasComponent<Transform>(EntityId entity) const {
  return transforms_.HasComponent(entity);
}

template <>
bool ECSWorld::HasComponent<Velocity>(EntityId entity) const {
  return velocities_.HasComponent(entity);
}

template <>
bool ECSWorld::HasComponent<Health>(EntityId entity) const {
  return healths_.HasComponent(entity);
}

template <>
Transform* ECSWorld::GetComponent<Transform>(EntityId entity) {
  return transforms_.GetComponent(entity);
}

template <>
Velocity* ECSWorld::GetComponent<Velocity>(EntityId entity) {
  return velocities_.GetComponent(entity);
}

template <>
Health* ECSWorld::GetComponent<Health>(EntityId entity) {
  return healths_.GetComponent(entity);
}

}  // namespace

TEST_SUITE("container::ContainerIntegration") {
  TEST_CASE("SparseSet: ECS World Basic Operations") {
    ECSWorld world;

    // Create entities
    auto entity1 = world.CreateEntity();
    auto entity2 = world.CreateEntity();
    auto entity3 = world.CreateEntity();

    CHECK_EQ(world.GetEntityCount(), 3);
    CHECK(world.IsValidEntity(entity1));
    CHECK(world.IsValidEntity(entity2));
    CHECK(world.IsValidEntity(entity3));

    // Add components
    world.AddComponent(entity1, Transform{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity1, Velocity{0.1f, 0.2f, 0.3f});
    world.AddComponent(entity1, Health{100, 120});

    world.AddComponent(entity2, Transform{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity2, Health{80, 100});

    world.AddComponent(entity3, Velocity{-0.1f, -0.2f, -0.3f});

    // Check component counts
    CHECK_EQ(world.GetTransformCount(), 2);
    CHECK_EQ(world.GetVelocityCount(), 2);
    CHECK_EQ(world.GetHealthCount(), 2);

    // Check component presence
    CHECK(world.HasComponent<Transform>(entity1));
    CHECK(world.HasComponent<Velocity>(entity1));
    CHECK(world.HasComponent<Health>(entity1));

    CHECK(world.HasComponent<Transform>(entity2));
    CHECK_FALSE(world.HasComponent<Velocity>(entity2));
    CHECK(world.HasComponent<Health>(entity2));

    CHECK_FALSE(world.HasComponent<Transform>(entity3));
    CHECK(world.HasComponent<Velocity>(entity3));
    CHECK_FALSE(world.HasComponent<Health>(entity3));

    // Check component values
    auto* transform1 = world.GetComponent<Transform>(entity1);
    REQUIRE(transform1 != nullptr);
    CHECK_EQ(transform1->x, 1.0F);
    CHECK_EQ(transform1->y, 2.0F);
    CHECK_EQ(transform1->z, 3.0F);

    auto* velocity3 = world.GetComponent<Velocity>(entity3);
    REQUIRE(velocity3 != nullptr);
    CHECK_EQ(velocity3->dx, -0.1f);
    CHECK_EQ(velocity3->dy, -0.2f);
    CHECK_EQ(velocity3->dz, -0.3f);
  }

  TEST_CASE("SparseSet: ECS Component Removal") {
    ECSWorld world;

    auto entity = world.CreateEntity();

    // Add all component types
    world.AddComponent(entity, Transform{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{0.1f, 0.2f, 0.3f});
    world.AddComponent(entity, Health{100, 120});

    CHECK(world.HasComponent<Transform>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));

    // Remove specific components
    world.RemoveComponent<Velocity>(entity);
    CHECK(world.HasComponent<Transform>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));

    world.RemoveComponent<Transform>(entity);
    CHECK_FALSE(world.HasComponent<Transform>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));

    world.RemoveComponent<Health>(entity);
    CHECK_FALSE(world.HasComponent<Transform>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK_FALSE(world.HasComponent<Health>(entity));

    // Entity should still exist
    CHECK(world.IsValidEntity(entity));
    CHECK_EQ(world.GetEntityCount(), 1);
  }

  TEST_CASE("SparseSet: ECS Entity Destruction") {
    ECSWorld world;

    auto entity1 = world.CreateEntity();
    auto entity2 = world.CreateEntity();

    // Add components to both entities
    world.AddComponent(entity1, Transform{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity1, Velocity{0.1f, 0.2f, 0.3f});

    world.AddComponent(entity2, Transform{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity2, Health{100, 120});

    CHECK_EQ(world.GetEntityCount(), 2);
    CHECK_EQ(world.GetTransformCount(), 2);
    CHECK_EQ(world.GetVelocityCount(), 1);
    CHECK_EQ(world.GetHealthCount(), 1);

    // Destroy entity1
    world.DestroyEntity(entity1);

    CHECK_EQ(world.GetEntityCount(), 1);
    CHECK_EQ(world.GetTransformCount(), 1);
    CHECK_EQ(world.GetVelocityCount(), 0);
    CHECK_EQ(world.GetHealthCount(), 1);

    CHECK_FALSE(world.IsValidEntity(entity1));
    CHECK(world.IsValidEntity(entity2));

    // Entity2's components should still be accessible
    CHECK(world.HasComponent<Transform>(entity2));
    CHECK(world.HasComponent<Health>(entity2));

    auto* transform2 = world.GetComponent<Transform>(entity2);
    REQUIRE(transform2 != nullptr);
    CHECK_EQ(transform2->x, 4.0F);
  }

  TEST_CASE("SparseSet: ECS System Updates") {
    ECSWorld world;

    // Create entities with movement
    auto moving_entity = world.CreateEntity();
    world.AddComponent(moving_entity, Transform{0.0F, 0.0F, 0.0F});
    world.AddComponent(moving_entity, Velocity{1.0F, 2.0F, 3.0F});

    auto static_entity = world.CreateEntity();
    world.AddComponent(static_entity, Transform{10.0F, 20.0F, 30.0F});

    auto healing_entity = world.CreateEntity();
    world.AddComponent(healing_entity, Health{50, 100});

    // Initial state check
    auto* initial_transform = world.GetComponent<Transform>(moving_entity);
    CHECK_EQ(initial_transform->x, 0.0F);
    CHECK_EQ(initial_transform->y, 0.0F);
    CHECK_EQ(initial_transform->z, 0.0F);

    auto* initial_health = world.GetComponent<Health>(healing_entity);
    CHECK_EQ(initial_health->current, 50);

    // Update systems once
    world.UpdateMovementSystem();
    world.UpdateHealthSystem();

    // Check movement system results - should be moved by velocity amounts
    auto* moved_transform = world.GetComponent<Transform>(moving_entity);
    CHECK_EQ(moved_transform->x, 1.0F);
    CHECK_EQ(moved_transform->y, 2.0F);
    CHECK_EQ(moved_transform->z, 3.0F);

    // Static entity should be unchanged
    auto* static_transform = world.GetComponent<Transform>(static_entity);
    CHECK_EQ(static_transform->x, 10.0F);
    CHECK_EQ(static_transform->y, 20.0F);
    CHECK_EQ(static_transform->z, 30.0F);

    // Check health system results
    auto* healed_health = world.GetComponent<Health>(healing_entity);
    CHECK_EQ(healed_health->current, 51);  // Should have healed by 1
  }

  TEST_CASE("SparseSet: ECS Query Systems") {
    ECSWorld world;

    // Create entities with different component combinations
    auto entity1 = world.CreateEntity();
    world.AddComponent(entity1, Transform{1.0F, 1.0F, 1.0F});
    world.AddComponent(entity1, Velocity{0.1f, 0.1f, 0.1f});

    auto entity2 = world.CreateEntity();
    world.AddComponent(entity2, Transform{2.0F, 2.0F, 2.0F});

    auto entity3 = world.CreateEntity();
    world.AddComponent(entity3, Velocity{0.3f, 0.3f, 0.3f});

    auto entity4 = world.CreateEntity();
    world.AddComponent(entity4, Transform{4.0F, 4.0F, 4.0F});
    world.AddComponent(entity4, Velocity{0.4f, 0.4f, 0.4f});
    world.AddComponent(entity4, Health{100, 100});

    // Query entities with Transform
    auto transform_entities = world.QueryEntitiesWithTransform();
    CHECK_EQ(transform_entities.size(), 3);

    std::sort(transform_entities.begin(), transform_entities.end());
    CHECK_EQ(transform_entities[0], entity1);
    CHECK_EQ(transform_entities[1], entity2);
    CHECK_EQ(transform_entities[2], entity4);

    // Query entities with both Transform and Velocity
    auto moving_entities = world.QueryEntitiesWithTransformAndVelocity();
    CHECK_EQ(moving_entities.size(), 2);

    std::sort(moving_entities.begin(), moving_entities.end());
    CHECK_EQ(moving_entities[0], entity1);
    CHECK_EQ(moving_entities[1], entity4);
  }

  TEST_CASE("SparseSet: Performance Stress Test") {
    ECSWorld world;
    constexpr size_t num_entities = 10000;

    auto timer_start = std::chrono::high_resolution_clock::now();

    // Create many entities with components
    timer_start = std::chrono::high_resolution_clock::now();
    std::vector<EntityId> entities;
    entities.reserve(num_entities);

    for (size_t i = 0; i < num_entities; ++i) {
      auto entity = world.CreateEntity();
      entities.push_back(entity);

      // Add components to some entities
      if (i % 2 == 0) {
        world.AddComponent(entity,
                           Transform{static_cast<float>(i), static_cast<float>(i + 1), static_cast<float>(i + 2)});
      }

      if (i % 3 == 0) {
        world.AddComponent(
            entity, Velocity{static_cast<float>(i * 0.1), static_cast<float>(i * 0.2), static_cast<float>(i * 0.3)});
      }

      if (i % 5 == 0) {
        world.AddComponent(entity, Health{static_cast<int>(100 - i % 50), 100});
      }
    }

    auto timer_end = std::chrono::high_resolution_clock::now();
    auto creation_time = std::chrono::duration_cast<std::chrono::microseconds>(timer_end - timer_start).count();
    HELIOS_INFO("Created {} entities with components in {} μs", num_entities, creation_time);

    CHECK_EQ(world.GetEntityCount(), num_entities);
    CHECK_EQ(world.GetTransformCount(), num_entities / 2);       // Every 2nd entity
    CHECK_EQ(world.GetVelocityCount(), (num_entities + 2) / 3);  // Every 3rd entity (rounded up)
    CHECK_EQ(world.GetHealthCount(), (num_entities + 4) / 5);    // Every 5th entity (rounded up)

    // Test component access performance
    timer_start = std::chrono::high_resolution_clock::now();
    size_t transform_count = 0;
    for (const auto& entity : entities) {
      if (world.HasComponent<Transform>(entity)) {
        auto* transform = world.GetComponent<Transform>(entity);
        if (transform) {
          transform_count++;
        }
      }
    }

    timer_end = std::chrono::high_resolution_clock::now();
    auto access_time = std::chrono::duration_cast<std::chrono::microseconds>(timer_end - timer_start).count();
    HELIOS_INFO("Accessed {} transforms in {} μs", transform_count, access_time);

    CHECK_EQ(transform_count, world.GetTransformCount());

    // Test removal performance
    timer_start = std::chrono::high_resolution_clock::now();
    size_t removed_count = 0;
    for (size_t i = 0; i < entities.size(); i += 4) {
      world.DestroyEntity(entities[i]);
      removed_count++;
    }

    timer_end = std::chrono::high_resolution_clock::now();
    auto removal_time = std::chrono::duration_cast<std::chrono::microseconds>(timer_end - timer_start).count();
    HELIOS_INFO("Removed {} entities in {} μs", removed_count, removal_time);

    CHECK_EQ(world.GetEntityCount(), num_entities - removed_count);
  }

  TEST_CASE("SparseSet: Memory Efficiency Test") {
    helios::container::SparseSet<Transform> sparse_components;
    std::unordered_map<EntityId, Transform> dense_components;

    // Test with sparse entity IDs (large gaps)
    std::vector<EntityId> sparse_entities = {0, 1000, 50000, 100000, 999999};

    // Add to both containers
    for (const auto& entity : sparse_entities) {
      Transform transform{static_cast<float>(entity), static_cast<float>(entity + 1), static_cast<float>(entity + 2)};

      sparse_components.Insert(entity, transform);
      dense_components[entity] = transform;
    }

    CHECK_EQ(sparse_components.Size(), sparse_entities.size());
    CHECK_EQ(dense_components.size(), sparse_entities.size());

    // Verify data integrity
    for (const auto& entity : sparse_entities) {
      CHECK(sparse_components.Contains(entity));

      const auto& sparse_transform = sparse_components.Get(entity);
      const auto& dense_transform = dense_components[entity];

      CHECK_EQ(sparse_transform.x, dense_transform.x);
      CHECK_EQ(sparse_transform.y, dense_transform.y);
      CHECK_EQ(sparse_transform.z, dense_transform.z);
    }

    // SparseSet should have larger sparse capacity but same dense size
    CHECK_GE(sparse_components.SparseCapacity(), 1000000);
    CHECK_EQ(sparse_components.Size(), 5);

    // Test iteration performance - SparseSet should be more cache-friendly
    auto timer_start = std::chrono::high_resolution_clock::now();

    timer_start = std::chrono::high_resolution_clock::now();
    float sum_sparse = 0.0F;
    for (const auto& transform : sparse_components) {
      sum_sparse += transform.x + transform.y + transform.z;
    }
    auto timer_end = std::chrono::high_resolution_clock::now();
    auto sparse_time = std::chrono::duration_cast<std::chrono::microseconds>(timer_end - timer_start).count();

    timer_start = std::chrono::high_resolution_clock::now();
    float sum_dense = 0.0F;
    for (const auto& [entity, transform] : dense_components) {
      sum_dense += transform.x + transform.y + transform.z;
    }
    timer_end = std::chrono::high_resolution_clock::now();
    auto dense_time = std::chrono::duration_cast<std::chrono::microseconds>(timer_end - timer_start).count();

    CHECK_EQ(sum_sparse, sum_dense);
    HELIOS_INFO("SparseSet iteration: {} μs, std::unordered_map iteration: {} μs", sparse_time, dense_time);
  }

  TEST_CASE("SparseSet: Thread Safety Documentation") {
    // This test documents thread safety requirements and expectations
    helios::container::SparseSet<int> set;

    // Single-threaded operations are safe
    set.Insert(1, 100);
    set.Insert(2, 200);
    CHECK_EQ(set.Size(), 2);

    // Multi-threaded access requires external synchronization
    // The SparseSet is not inherently thread-safe
    std::mutex set_mutex;
    std::vector<std::thread> threads;
    std::atomic<int> successful_insertions{0};

    // Create threads that attempt to insert with proper synchronization
    for (int i = 0; i < 4; ++i) {
      threads.emplace_back([&, i]() {
        for (int j = 0; j < 100; ++j) {
          {
            std::lock_guard<std::mutex> lock(set_mutex);
            // Use unique indices for each thread to avoid conflicts
            set.Insert(i * 1000 + j + 100, (i * 1000 + j + 100) * 10);
            successful_insertions++;
          }
        }
      });
    }

    for (auto& thread : threads) {
      thread.join();
    }

    CHECK_EQ(successful_insertions.load(), 400);
    CHECK_EQ(set.Size(), 402);  // 400 new + 2 original
  }

  TEST_CASE("SparseSet: Real-world ECS Performance") {
    ECSWorld world;
    constexpr size_t num_entities = 1000;
    constexpr size_t num_frames = 60;

    // Create a realistic game scenario
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> pos_dist(-100.0F, 100.0F);
    std::uniform_real_distribution<float> vel_dist(-5.0F, 5.0F);
    std::uniform_int_distribution<int> health_dist(50, 150);

    // Setup entities
    std::vector<EntityId> entities;
    for (size_t i = 0; i < num_entities; ++i) {
      auto entity = world.CreateEntity();
      entities.push_back(entity);

      // 80% have transforms
      if (gen() % 10 < 8) {
        world.AddComponent(entity, Transform{pos_dist(gen), pos_dist(gen), pos_dist(gen)});
      }

      // 60% have velocity (subset of those with transforms)
      if (gen() % 10 < 6 && world.HasComponent<Transform>(entity)) {
        world.AddComponent(entity, Velocity{vel_dist(gen), vel_dist(gen), vel_dist(gen)});
      }

      // 40% have health
      if (gen() % 10 < 4) {
        world.AddComponent(entity, Health{health_dist(gen), 100});
      }
    }

    HELIOS_INFO("Created ECS world with {} entities:", num_entities);
    HELIOS_INFO("  Transforms: {}", world.GetTransformCount());
    HELIOS_INFO("  Velocities: {}", world.GetVelocityCount());
    HELIOS_INFO("  Health: {}", world.GetHealthCount());

    // Simulate game loop
    auto frame_timer_start = std::chrono::high_resolution_clock::now();
    double total_update_time = 0.0;

    for (size_t frame = 0; frame < num_frames; ++frame) {
      frame_timer_start = std::chrono::high_resolution_clock::now();

      world.UpdateMovementSystem();
      world.UpdateHealthSystem();

      auto frame_timer_end = std::chrono::high_resolution_clock::now();
      auto frame_time =
          std::chrono::duration_cast<std::chrono::microseconds>(frame_timer_end - frame_timer_start).count();
      total_update_time += frame_time;

      // Occasionally spawn/destroy entities to test dynamic scenarios
      if (frame % 10 == 0) {
        // Destroy some random entities
        std::uniform_int_distribution<size_t> entity_dist(0, entities.size() - 1);
        for (int i = 0; i < 5; ++i) {
          if (!entities.empty()) {
            auto idx = entity_dist(gen) % entities.size();
            world.DestroyEntity(entities[idx]);
            entities.erase(entities.begin() + idx);
          }
        }

        // Create some new entities
        for (int i = 0; i < 3; ++i) {
          auto entity = world.CreateEntity();
          entities.push_back(entity);

          world.AddComponent(entity, Transform{pos_dist(gen), pos_dist(gen), pos_dist(gen)});
          world.AddComponent(entity, Velocity{vel_dist(gen), vel_dist(gen), vel_dist(gen)});
        }
      }
    }

    double avg_frame_time = total_update_time / num_frames;
    HELIOS_INFO("Average frame update time: {:.2f} μs ({:.2f} ms)", avg_frame_time, avg_frame_time / 1000.0);

    // Performance should be reasonable for this scale (relaxed for complex ECS scenario)
    CHECK_LT(avg_frame_time, 1000.0);     // Less than 1ms per frame
    CHECK_GT(world.GetEntityCount(), 0);  // Should still have entities
  }

}  // TEST_SUITE
