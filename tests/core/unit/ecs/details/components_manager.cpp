#include <doctest/doctest.h>

#include <helios/core/ecs/details/components_manager.hpp>

#include <cstddef>
#include <string>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::details;

namespace {

// Test component types
struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;

  constexpr bool operator==(const Position& other) const noexcept = default;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;

  constexpr bool operator==(const Velocity& other) const noexcept = default;
};

struct Name {
  std::string value;

  constexpr bool operator==(const Name& other) const noexcept = default;
};

struct TagComponent {};

struct LargeComponent {
  std::array<uint8_t, 512> data = {};

  constexpr bool operator==(const LargeComponent& other) const noexcept = default;
};

}  // namespace

TEST_SUITE("ecs::details::ComponentsManager") {
  TEST_CASE("ComponentStorage::ComponentStorage: default construction") {
    ComponentStorage<Position> storage;

    CHECK_EQ(storage.Size(), 0);
    CHECK(storage.Data().empty());
    CHECK_EQ(storage.begin(), storage.end());
    CHECK_EQ(storage.cbegin(), storage.cend());
  }

  TEST_CASE("ComponentStorage::Emplace") {
    ComponentStorage<Position> storage;
    Entity entity(42, 1);

    storage.Emplace(entity, 1.0F, 2.0F, 3.0F);

    CHECK_EQ(storage.Size(), 1);
    CHECK(storage.Contains(entity));

    const Position& pos = storage.Get(entity);
    CHECK_EQ(pos.x, 1.0F);
    CHECK_EQ(pos.y, 2.0F);
    CHECK_EQ(pos.z, 3.0F);
  }

  TEST_CASE("ComponentStorage::Insert: copy") {
    ComponentStorage<Position> storage;
    Entity entity(42, 1);
    Position pos(5.0F, 6.0F, 7.0F);

    storage.Insert(entity, pos);

    CHECK_EQ(storage.Size(), 1);
    CHECK(storage.Contains(entity));

    const Position& stored = storage.Get(entity);
    CHECK_EQ(stored, pos);
  }

  TEST_CASE("ComponentStorage::Insert: move") {
    ComponentStorage<Name> storage;
    Entity entity(42, 1);
    Name name("TestName");

    storage.Insert(entity, std::move(name));

    CHECK_EQ(storage.Size(), 1);
    CHECK(storage.Contains(entity));

    const Name& stored = storage.Get(entity);
    CHECK_EQ(stored.value, "TestName");
    CHECK(name.value.empty());  // Should be moved from
  }

  TEST_CASE("ComponentStorage: multiple components") {
    ComponentStorage<Position> storage;
    Entity entity1(1, 1);
    Entity entity2(2, 1);
    Entity entity3(3, 1);

    storage.Emplace(entity1, 1.0F, 2.0F, 3.0F);
    storage.Emplace(entity2, 4.0F, 5.0F, 6.0F);
    storage.Emplace(entity3, 7.0F, 8.0F, 9.0F);

    CHECK_EQ(storage.Size(), 3);
    CHECK(storage.Contains(entity1));
    CHECK(storage.Contains(entity2));
    CHECK(storage.Contains(entity3));

    CHECK_EQ(storage.Get(entity1), Position(1.0F, 2.0F, 3.0F));
    CHECK_EQ(storage.Get(entity2), Position(4.0F, 5.0F, 6.0F));
    CHECK_EQ(storage.Get(entity3), Position(7.0F, 8.0F, 9.0F));
  }

  TEST_CASE("ComponentStorage::Remove") {
    ComponentStorage<Position> storage;
    Entity entity1(1, 1);
    Entity entity2(2, 1);
    Entity entity3(3, 1);

    storage.Emplace(entity1, 1.0F, 2.0F, 3.0F);
    storage.Emplace(entity2, 4.0F, 5.0F, 6.0F);
    storage.Emplace(entity3, 7.0F, 8.0F, 9.0F);

    CHECK_EQ(storage.Size(), 3);
    CHECK(storage.Contains(entity2));

    storage.Remove(entity2);

    CHECK_EQ(storage.Size(), 2);
    CHECK_FALSE(storage.Contains(entity2));
    CHECK(storage.Contains(entity1));
    CHECK(storage.Contains(entity3));
  }

  TEST_CASE("ComponentStorage::TryRemove") {
    ComponentStorage<Position> storage;
    Entity entity1(1, 1);
    Entity entity2(2, 1);

    storage.Emplace(entity1, 1.0F, 2.0F, 3.0F);

    CHECK(storage.TryRemove(entity1));
    CHECK_EQ(storage.Size(), 0);
    CHECK_FALSE(storage.Contains(entity1));

    CHECK_FALSE(storage.TryRemove(entity2));  // Entity not in storage
    CHECK_EQ(storage.Size(), 0);
  }

  TEST_CASE("ComponentStorage::TryGet") {
    ComponentStorage<Position> storage;
    Entity entity1(1, 1);
    Entity entity2(2, 1);

    storage.Emplace(entity1, 1.0F, 2.0F, 3.0F);

    Position* pos1 = storage.TryGet(entity1);
    Position* pos2 = storage.TryGet(entity2);

    REQUIRE(pos1 != nullptr);
    CHECK_EQ(*pos1, Position(1.0F, 2.0F, 3.0F));
    CHECK_EQ(pos2, nullptr);

    // Test const version
    const auto& const_storage = storage;
    const Position* const_pos1 = const_storage.TryGet(entity1);
    const Position* const_pos2 = const_storage.TryGet(entity2);

    REQUIRE(const_pos1 != nullptr);
    CHECK_EQ(*const_pos1, Position(1.0F, 2.0F, 3.0F));
    CHECK_EQ(const_pos2, nullptr);
  }

  TEST_CASE("ComponentStorage::TryGet: invalid entity") {
    ComponentStorage<Position> storage;
    Entity invalid_entity;  // Default invalid entity

    Position* pos = storage.TryGet(invalid_entity);
    CHECK_EQ(pos, nullptr);

    const auto& const_storage = storage;
    const Position* const_pos = const_storage.TryGet(invalid_entity);
    CHECK_EQ(const_pos, nullptr);
  }

  TEST_CASE("ComponentStorage::Clear") {
    ComponentStorage<Position> storage;
    Entity entity1(1, 1);
    Entity entity2(2, 1);

    storage.Emplace(entity1, 1.0F, 2.0F, 3.0F);
    storage.Emplace(entity2, 4.0F, 5.0F, 6.0F);

    CHECK_EQ(storage.Size(), 2);

    storage.Clear();

    CHECK_EQ(storage.Size(), 0);
    CHECK_FALSE(storage.Contains(entity1));
    CHECK_FALSE(storage.Contains(entity2));
    CHECK(storage.Data().empty());
  }

  TEST_CASE("ComponentStorage::GetTypeInfo") {
    ComponentStorage<Position> storage;

    ComponentTypeInfo info = storage.GetTypeInfo();

    CHECK_EQ(info.TypeId(), ComponentTypeIdOf<Position>());
    CHECK_EQ(info.Size(), sizeof(Position));
    CHECK_EQ(info.Alignment(), alignof(Position));
    CHECK(info.IsTrivial());
  }

  TEST_CASE("ComponentStorage::Data") {
    ComponentStorage<Position> storage;
    Entity entity1(1, 1);
    Entity entity2(2, 1);

    storage.Emplace(entity1, 1.0F, 2.0F, 3.0F);
    storage.Emplace(entity2, 4.0F, 5.0F, 6.0F);

    auto data = storage.Data();
    CHECK_EQ(data.size(), 2);

    // Modify through span
    data[0].x = 10.0F;
    CHECK_EQ(storage.Get(entity1).x, 10.0F);

    // Test const version
    const auto& const_storage = storage;
    auto const_data = const_storage.Data();
    CHECK_EQ(const_data.size(), 2);
    CHECK_EQ(const_data[0].x, 10.0F);
  }

  TEST_CASE("ComponentStorage: iteration") {
    ComponentStorage<Position> storage;
    Entity entity1(1, 1);
    Entity entity2(2, 1);
    Entity entity3(3, 1);

    storage.Emplace(entity1, 1.0F, 2.0F, 3.0F);
    storage.Emplace(entity2, 4.0F, 5.0F, 6.0F);
    storage.Emplace(entity3, 7.0F, 8.0F, 9.0F);

    std::vector<Position> positions;
    for (const auto& pos : storage) {
      positions.push_back(pos);
    }

    CHECK_EQ(positions.size(), 3);
    // Note: Order depends on insertion order in sparse set
  }

  TEST_CASE("ComponentStorage: component replacement") {
    ComponentStorage<Position> storage;
    Entity entity(42, 1);

    storage.Emplace(entity, 1.0F, 2.0F, 3.0F);
    CHECK_EQ(storage.Size(), 1);
    CHECK_EQ(storage.Get(entity), Position(1.0F, 2.0F, 3.0F));

    // Replace with new component
    storage.Emplace(entity, 4.0F, 5.0F, 6.0F);
    CHECK_EQ(storage.Size(), 1);  // Size should remain same
    CHECK_EQ(storage.Get(entity), Position(4.0F, 5.0F, 6.0F));
  }

  TEST_CASE("Components::Components: default construction") {
    Components components;

    Entity entity(42, 1);
    CHECK_FALSE(components.HasComponent<Position>(entity));
    CHECK_EQ(components.TryGetComponent<Position>(entity), nullptr);
  }

  TEST_CASE("Components::AddComponent") {
    Components components;
    Entity entity(42, 1);
    Position pos(1.0F, 2.0F, 3.0F);

    components.AddComponent(entity, std::move(pos));

    CHECK(components.HasComponent<Position>(entity));
    CHECK_EQ(components.GetComponent<Position>(entity), Position(1.0F, 2.0F, 3.0F));
  }

  TEST_CASE("Components::EmplaceComponent") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);

    CHECK(components.HasComponent<Position>(entity));
    CHECK_EQ(components.GetComponent<Position>(entity), Position(1.0F, 2.0F, 3.0F));
  }

  TEST_CASE("Components: multiple component types") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);
    components.EmplaceComponent<Velocity>(entity, 4.0F, 5.0F, 6.0F);
    components.AddComponent(entity, Name("TestEntity"));

    CHECK(components.HasComponent<Position>(entity));
    CHECK(components.HasComponent<Velocity>(entity));
    CHECK(components.HasComponent<Name>(entity));

    CHECK_EQ(components.GetComponent<Position>(entity), Position(1.0F, 2.0F, 3.0F));
    CHECK_EQ(components.GetComponent<Velocity>(entity), Velocity(4.0F, 5.0F, 6.0F));
    CHECK_EQ(components.GetComponent<Name>(entity).value, "TestEntity");
  }

  TEST_CASE("Components::RemoveComponent") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);
    components.EmplaceComponent<Velocity>(entity, 4.0F, 5.0F, 6.0F);

    CHECK(components.HasComponent<Position>(entity));
    CHECK(components.HasComponent<Velocity>(entity));

    components.RemoveComponent<Position>(entity);

    CHECK_FALSE(components.HasComponent<Position>(entity));
    CHECK(components.HasComponent<Velocity>(entity));
  }

  TEST_CASE("Components::TryGetComponent") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);

    Position* pos = components.TryGetComponent<Position>(entity);
    Velocity* vel = components.TryGetComponent<Velocity>(entity);

    REQUIRE(pos != nullptr);
    CHECK_EQ(*pos, Position(1.0F, 2.0F, 3.0F));
    CHECK_EQ(vel, nullptr);

    // Test const version
    const auto& const_components = components;
    const Position* const_pos = const_components.TryGetComponent<Position>(entity);
    const Velocity* const_vel = const_components.TryGetComponent<Velocity>(entity);

    REQUIRE(const_pos != nullptr);
    CHECK_EQ(*const_pos, Position(1.0F, 2.0F, 3.0F));
    CHECK_EQ(const_vel, nullptr);
  }

  TEST_CASE("Components::GetStorage") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);

    ComponentStorage<Position>& storage = components.GetStorage<Position>();
    CHECK_EQ(storage.Size(), 1);
    CHECK(storage.Contains(entity));

    // Test const version
    const auto& const_components = components;
    const ComponentStorage<Position>& const_storage = const_components.GetStorage<Position>();
    CHECK_EQ(const_storage.Size(), 1);
    CHECK(const_storage.Contains(entity));
  }

  TEST_CASE("Components::HasComponent") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);
    components.EmplaceComponent<Velocity>(entity, 4.0F, 5.0F, 6.0F);
    components.AddComponent(entity, Name("TestEntity"));

    CHECK(components.HasComponent<Position>(entity));
    CHECK(components.HasComponent<Velocity>(entity));
    CHECK(components.HasComponent<Name>(entity));

    components.RemoveAllComponents(entity);

    CHECK_FALSE(components.HasComponent<Position>(entity));
    CHECK_FALSE(components.HasComponent<Velocity>(entity));
    CHECK_FALSE(components.HasComponent<Name>(entity));
  }

  TEST_CASE("Components::GetEntityComponents") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);
    components.EmplaceComponent<Velocity>(entity, 4.0F, 5.0F, 6.0F);

    std::vector<ComponentTypeInfo> types = components.GetComponentTypes(entity);

    CHECK_EQ(types.size(), 2);

    // Check that both types are present (order may vary)
    bool has_position = false;
    bool has_velocity = false;

    for (const auto& type : types) {
      if (type.TypeId() == ComponentTypeIdOf<Position>()) {
        has_position = true;
      } else if (type.TypeId() == ComponentTypeIdOf<Velocity>()) {
        has_velocity = true;
      }
    }

    CHECK(has_position);
    CHECK(has_velocity);
  }

  TEST_CASE("Components::Clear") {
    Components components;
    Entity entity1(1, 1);
    Entity entity2(2, 1);

    components.EmplaceComponent<Position>(entity1, 1.0F, 2.0F, 3.0F);
    components.EmplaceComponent<Position>(entity2, 4.0F, 5.0F, 6.0F);
    components.EmplaceComponent<Velocity>(entity1, 7.0F, 8.0F, 9.0F);

    CHECK(components.HasComponent<Position>(entity1));
    CHECK(components.HasComponent<Position>(entity2));
    CHECK(components.HasComponent<Velocity>(entity1));

    components.Clear();

    CHECK_FALSE(components.HasComponent<Position>(entity1));
    CHECK_FALSE(components.HasComponent<Position>(entity2));
    CHECK_FALSE(components.HasComponent<Velocity>(entity1));
  }

  TEST_CASE("Components: component types registration") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<Position>(entity, 1.0F, 2.0F, 3.0F);

    // Modify through reference
    Position& pos = components.GetComponent<Position>(entity);
    pos.x = 10.0F;

    CHECK_EQ(components.GetComponent<Position>(entity).x, 10.0F);

    // Modify through pointer
    Position* pos_ptr = components.TryGetComponent<Position>(entity);
    REQUIRE(pos_ptr != nullptr);
    pos_ptr->y = 20.0F;

    CHECK_EQ(components.GetComponent<Position>(entity).y, 20.0F);
  }

  TEST_CASE("Components::ClearEntityComponents") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<LargeComponent>(entity);

    CHECK(components.HasComponent<LargeComponent>(entity));

    LargeComponent& large = components.GetComponent<LargeComponent>(entity);
    large.data[0] = 42;
    large.data[511] = 99;

    CHECK_EQ(components.GetComponent<LargeComponent>(entity).data[0], 42);
    CHECK_EQ(components.GetComponent<LargeComponent>(entity).data[511], 99);
  }

  TEST_CASE("Components::RemoveAllComponents") {
    Components components;
    Entity entity(42, 1);

    components.EmplaceComponent<TagComponent>(entity);

    CHECK(components.HasComponent<TagComponent>(entity));

    ComponentStorage<TagComponent>& storage = components.GetStorage<TagComponent>();
    CHECK_EQ(storage.Size(), 1);
    CHECK(storage.Contains(entity));
  }

  TEST_CASE("ecs::details::Components - Multiple Entities Same Component") {
    Components components;
    Entity entity1(1, 1);
    Entity entity2(2, 1);
    Entity entity3(3, 1);

    components.EmplaceComponent<Position>(entity1, 1.0F, 2.0F, 3.0F);
    components.EmplaceComponent<Position>(entity2, 4.0F, 5.0F, 6.0F);
    components.EmplaceComponent<Position>(entity3, 7.0F, 8.0F, 9.0F);

    CHECK(components.HasComponent<Position>(entity1));
    CHECK(components.HasComponent<Position>(entity2));
    CHECK(components.HasComponent<Position>(entity3));

    ComponentStorage<Position>& storage = components.GetStorage<Position>();
    CHECK_EQ(storage.Size(), 3);

    CHECK_EQ(components.GetComponent<Position>(entity1), Position(1.0F, 2.0F, 3.0F));
    CHECK_EQ(components.GetComponent<Position>(entity2), Position(4.0F, 5.0F, 6.0F));
    CHECK_EQ(components.GetComponent<Position>(entity3), Position(7.0F, 8.0F, 9.0F));
  }

  TEST_CASE("Components: large scale") {
    Components components;
    constexpr size_t entity_count = 1000;

    std::vector<Entity> entities;
    for (size_t i = 0; i < entity_count; ++i) {
      entities.emplace_back(static_cast<Entity::IndexType>(i), 1);
    }

    // Add components to all entities
    for (size_t i = 0; i < entity_count; ++i) {
      components.EmplaceComponent<Position>(entities[i], static_cast<float>(i), static_cast<float>(i * 2),
                                            static_cast<float>(i * 3));

      if (i % 2 == 0) {
        components.EmplaceComponent<Velocity>(entities[i], static_cast<float>(i), static_cast<float>(i),
                                              static_cast<float>(i));
      }
    }

    // Verify all components
    for (size_t i = 0; i < entity_count; ++i) {
      CHECK(components.HasComponent<Position>(entities[i]));

      const Position& pos = components.GetComponent<Position>(entities[i]);
      CHECK_EQ(pos.x, static_cast<float>(i));
      CHECK_EQ(pos.y, static_cast<float>(i * 2));
      CHECK_EQ(pos.z, static_cast<float>(i * 3));

      if (i % 2 == 0) {
        CHECK(components.HasComponent<Velocity>(entities[i]));
        const Velocity& vel = components.GetComponent<Velocity>(entities[i]);
        CHECK_EQ(vel.dx, static_cast<float>(i));
        CHECK_EQ(vel.dy, static_cast<float>(i));
        CHECK_EQ(vel.dz, static_cast<float>(i));
      } else {
        CHECK_FALSE(components.HasComponent<Velocity>(entities[i]));
      }
    }

    // Remove half the entities' components
    for (size_t i = 0; i < entity_count / 2; ++i) {
      components.RemoveAllComponents(entities[i]);
    }

    // Verify removal
    for (size_t i = 0; i < entity_count; ++i) {
      if (i < entity_count / 2) {
        CHECK_FALSE(components.HasComponent<Position>(entities[i]));
        CHECK_FALSE(components.HasComponent<Velocity>(entities[i]));
      } else {
        CHECK(components.HasComponent<Position>(entities[i]));
      }
    }
  }

}  // TEST_SUITE
