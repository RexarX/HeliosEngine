#include <doctest/doctest.h>

#include <helios/core/ecs/details/commands.hpp>
#include <helios/core/ecs/world.hpp>

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

struct Health {
  int points = 100;

  constexpr bool operator==(const Health& other) const noexcept = default;
};

struct TagComponent {};

}  // namespace

TEST_SUITE("ecs::details::Commands") {
  TEST_CASE("FunctionCmd: basic execution") {
    World world;

    bool executed = false;
    World* captured_world = nullptr;

    auto func = [&executed, &captured_world](World& w) {
      executed = true;
      captured_world = &w;
    };

    FunctionCmd<decltype(func)> cmd(func);
    cmd.Execute(world);

    CHECK(executed);
    CHECK_EQ(captured_world, &world);
  }

  TEST_CASE("FunctionCmd: lambda with captures") {
    World world;

    int captured_value = 42;
    int result = 0;

    auto func = [&result, captured_value](World&) { result = captured_value * 2; };

    FunctionCmd<decltype(func)> cmd(func);
    cmd.Execute(world);

    CHECK_EQ(result, 84);
  }

  TEST_CASE("FunctionCmd: copy and move semantics") {
    World world;

    int call_count = 0;

    auto func = [&call_count](World&) { ++call_count; };

    FunctionCmd<decltype(func)> cmd1(func);  // Copy
    FunctionCmd<decltype(func)> cmd2(func);  // Copy

    cmd1.Execute(world);
    cmd2.Execute(world);

    CHECK_EQ(call_count, 2);
  }

  TEST_CASE("DestroyEntityCmd: basic destruction") {
    World world;

    Entity entity = world.CreateEntity();
    CHECK(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 1);

    DestroyEntityCmd cmd(entity);
    cmd.Execute(world);

    CHECK_FALSE(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("DestroyEntityCmd: copy and move") {
    World world;

    Entity entity = world.CreateEntity();
    CHECK(world.Exists(entity));

    DestroyEntityCmd cmd1(entity);
    DestroyEntityCmd cmd2 = cmd1;             // Copy
    DestroyEntityCmd cmd3 = std::move(cmd1);  // Move

    // All commands should reference the same entity
    cmd2.Execute(world);  // This should destroy the entity
    CHECK_FALSE(world.Exists(entity));
  }

  TEST_CASE("DestroyEntitiesCmd: multiple entities") {
    World world;

    std::vector<Entity> entities;
    for (int i = 0; i < 5; ++i) {
      entities.push_back(world.CreateEntity());
    }
    CHECK_EQ(world.EntityCount(), 5);

    std::vector<Entity> to_destroy = {entities[1], entities[3]};
    DestroyEntitiesCmd cmd(to_destroy);
    cmd.Execute(world);

    CHECK_EQ(world.EntityCount(), 3);
    CHECK(world.Exists(entities[0]));
    CHECK_FALSE(world.Exists(entities[1]));
    CHECK(world.Exists(entities[2]));
    CHECK_FALSE(world.Exists(entities[3]));
    CHECK(world.Exists(entities[4]));
  }

  TEST_CASE("DestroyEntitiesCmd: empty range") {
    World world;

    Entity entity = world.CreateEntity();
    CHECK_EQ(world.EntityCount(), 1);

    std::vector<Entity> empty_range;
    DestroyEntitiesCmd cmd(empty_range);
    cmd.Execute(world);

    CHECK_EQ(world.EntityCount(), 1);  // Should remain unchanged
    CHECK(world.Exists(entity));
  }

  TEST_CASE("AddComponentCmd: basic addition") {
    World world;

    Entity entity = world.CreateEntity();
    CHECK_FALSE(world.HasComponent<Position>(entity));

    Position pos{1.0F, 2.0F, 3.0F};
    AddComponentCmd<Position> cmd(entity, pos);
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("AddComponentCmd: move construction") {
    World world;

    Entity entity = world.CreateEntity();

    Name name{"TestEntity"};
    AddComponentCmd<Name> cmd(entity, std::move(name));
    cmd.Execute(world);

    CHECK(world.HasComponent<Name>(entity));
    CHECK(name.value.empty());  // Should be moved from
  }

  TEST_CASE("AddComponentCmd: component replacement") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    CHECK(world.HasComponent<Position>(entity));

    AddComponentCmd<Position> cmd(entity, Position{4.0F, 5.0F, 6.0F});
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));
    // Component should be replaced, not duplicated
  }

  TEST_CASE("AddComponentsCmd: multiple components") {
    World world;

    Entity entity = world.CreateEntity();
    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK_FALSE(world.HasComponent<Health>(entity));

    Position pos{1.0F, 2.0F, 3.0F};
    Velocity vel{4.0F, 5.0F, 6.0F};
    Health health{100};

    AddComponentsCmd<Position, Velocity, Health> cmd(entity, std::move(pos), std::move(vel), std::move(health));
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
  }

  TEST_CASE("AddComponentsCmd: mixed types") {
    World world;

    Entity entity = world.CreateEntity();

    Position pos{7.0F, 8.0F, 9.0F};
    Name name{"MixedEntity"};
    TagComponent tag{};

    AddComponentsCmd<Position, Name, TagComponent> cmd(entity, std::move(pos), std::move(name), std::move(tag));
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Name>(entity));
    CHECK(world.HasComponent<TagComponent>(entity));
  }

  TEST_CASE("RemoveComponentCmd: basic removal") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));

    RemoveComponentCmd<Position> cmd(entity);
    cmd.Execute(world);

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));  // Should remain
  }

  TEST_CASE("RemoveComponentsCmd: multiple components") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity, Health{100});
    world.AddComponent(entity, Name{"TestEntity"});

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
    CHECK(world.HasComponent<Name>(entity));

    RemoveComponentsCmd<Position, Velocity> cmd(entity);
    cmd.Execute(world);

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));  // Should remain
    CHECK(world.HasComponent<Name>(entity));    // Should remain
  }

  TEST_CASE("ClearComponentsCmd: clear all components") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity, Health{100});
    world.AddComponent(entity, Name{"TestEntity"});
    world.AddComponent(entity, TagComponent{});

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
    CHECK(world.HasComponent<Name>(entity));
    CHECK(world.HasComponent<TagComponent>(entity));

    ClearComponentsCmd cmd(entity);
    cmd.Execute(world);

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK_FALSE(world.HasComponent<Health>(entity));
    CHECK_FALSE(world.HasComponent<Name>(entity));
    CHECK_FALSE(world.HasComponent<TagComponent>(entity));
    CHECK(world.Exists(entity));  // Entity should still exist
  }

  TEST_CASE("Commands: copy and move semantics") {
    World world;
    Entity entity = world.CreateEntity();

    // Test copy semantics
    AddComponentCmd<Position> cmd1(entity, Position{1.0F, 2.0F, 3.0F});
    AddComponentCmd<Position> cmd2 = cmd1;  // Copy constructor
    AddComponentCmd<Position> cmd3(entity, Position{4.0F, 5.0F, 6.0F});
    cmd3 = cmd1;  // Copy assignment

    cmd2.Execute(world);
    CHECK(world.HasComponent<Position>(entity));

    // Test move semantics
    AddComponentCmd<Velocity> cmd4(entity, Velocity{7.0F, 8.0F, 9.0F});
    AddComponentCmd<Velocity> cmd5 = std::move(cmd4);  // Move constructor
    AddComponentCmd<Velocity> cmd6(entity, Velocity{10.0F, 11.0F, 12.0F});
    cmd6 = std::move(cmd5);  // Move assignment

    cmd6.Execute(world);
    CHECK(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("Commands: constexpr and noexcept") {
    World world;
    Entity entity = world.CreateEntity();

    // Test that commands can be constructed as constexpr where applicable
    constexpr Entity test_entity(42, 1);

    // Test noexcept specifications
    CHECK(std::is_nothrow_constructible_v<DestroyEntityCmd, Entity>);
    CHECK(std::is_nothrow_move_constructible_v<DestroyEntityCmd>);
    CHECK(std::is_nothrow_copy_constructible_v<DestroyEntityCmd>);

    // Tag component commands should be noexcept
    CHECK(std::is_nothrow_constructible_v<AddComponentCmd<TagComponent>, Entity, TagComponent>);
  }

  TEST_CASE("Commands: complex scenarios") {
    World world;

    // Create multiple entities
    std::vector<Entity> entities;
    for (int i = 0; i < 5; ++i) {
      entities.push_back(world.CreateEntity());
    }

    // Add different components to different entities using commands
    AddComponentCmd<Position> pos_cmd1(entities[0], Position{1.0F, 2.0F, 3.0F});
    AddComponentCmd<Position> pos_cmd2(entities[1], Position{4.0F, 5.0F, 6.0F});
    AddComponentsCmd<Position, Velocity> multi_cmd(entities[2], Position{7.0F, 8.0F, 9.0F}, Velocity{1.0F, 1.0F, 1.0F});

    pos_cmd1.Execute(world);
    pos_cmd2.Execute(world);
    multi_cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entities[0]));
    CHECK(world.HasComponent<Position>(entities[1]));
    CHECK(world.HasComponent<Position>(entities[2]));
    CHECK_FALSE(world.HasComponent<Velocity>(entities[0]));
    CHECK_FALSE(world.HasComponent<Velocity>(entities[1]));
    CHECK(world.HasComponent<Velocity>(entities[2]));

    // Remove components from some entities
    RemoveComponentCmd<Position> remove_cmd(entities[1]);
    remove_cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entities[0]));
    CHECK_FALSE(world.HasComponent<Position>(entities[1]));
    CHECK(world.HasComponent<Position>(entities[2]));

    // Clear all components from one entity
    ClearComponentsCmd clear_cmd(entities[2]);
    clear_cmd.Execute(world);

    CHECK_FALSE(world.HasComponent<Position>(entities[2]));
    CHECK_FALSE(world.HasComponent<Velocity>(entities[2]));

    // Destroy some entities
    std::vector<Entity> to_destroy = {entities[3], entities[4]};
    DestroyEntitiesCmd destroy_cmd(to_destroy);
    destroy_cmd.Execute(world);

    CHECK(world.Exists(entities[0]));
    CHECK(world.Exists(entities[1]));
    CHECK(world.Exists(entities[2]));
    CHECK_FALSE(world.Exists(entities[3]));
    CHECK_FALSE(world.Exists(entities[4]));
  }

  TEST_CASE("TryDestroyEntityCmd: basic destruction") {
    World world;

    Entity entity = world.CreateEntity();
    CHECK(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 1);

    TryDestroyEntityCmd cmd{entity};
    cmd.Execute(world);

    CHECK_FALSE(world.Exists(entity));
    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("TryDestroyEntityCmd: non-existent entity") {
    World world;

    Entity entity = world.CreateEntity();
    world.DestroyEntity(entity);  // Destroy the entity first
    CHECK_FALSE(world.Exists(entity));

    // This should not crash or assert
    TryDestroyEntityCmd cmd{entity};
    cmd.Execute(world);  // Should be a no-op

    CHECK_EQ(world.EntityCount(), 0);
  }

  TEST_CASE("TryDestroyEntitiesCmd: mixed existing and non-existent") {
    World world;

    std::vector<Entity> entities;
    for (int i = 0; i < 5; ++i) {
      entities.push_back(world.CreateEntity());
    }
    CHECK_EQ(world.EntityCount(), 5);

    // Destroy one entity beforehand
    world.DestroyEntity(entities[2]);
    CHECK_EQ(world.EntityCount(), 4);

    // Try to destroy a mix of existing and non-existing entities
    std::vector<Entity> to_destroy = {entities[1], entities[2], entities[3]};
    TryDestroyEntitiesCmd cmd{to_destroy};
    cmd.Execute(world);

    CHECK_EQ(world.EntityCount(), 2);  // Only entities 0 and 4 should remain
    CHECK(world.Exists(entities[0]));
    CHECK_FALSE(world.Exists(entities[1]));
    CHECK_FALSE(world.Exists(entities[2]));
    CHECK_FALSE(world.Exists(entities[3]));
    CHECK(world.Exists(entities[4]));
  }

  TEST_CASE("TryAddComponentCmd: basic addition") {
    World world;

    Entity entity = world.CreateEntity();
    CHECK_FALSE(world.HasComponent<Position>(entity));

    Position pos{1.0F, 2.0F, 3.0F};
    TryAddComponentCmd<Position> cmd{entity, pos};
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("TryAddComponentCmd: component already exists") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    CHECK(world.HasComponent<Position>(entity));

    // Try to add the same component type - should be a no-op
    Position new_pos{4.0F, 5.0F, 6.0F};
    TryAddComponentCmd<Position> cmd{entity, new_pos};
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));
    // Original component should remain (not replaced)
  }

  TEST_CASE("TryAddComponentCmd: in-place construction") {
    World world;

    Entity entity = world.CreateEntity();

    TryAddComponentCmd<Position> cmd{entity, Position{4.0F, 5.0F, 6.0F}};
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));
  }

  TEST_CASE("TryAddComponentsCmd: multiple components") {
    World world;

    Entity entity = world.CreateEntity();
    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK_FALSE(world.HasComponent<Health>(entity));

    Position pos{1.0F, 2.0F, 3.0F};
    Velocity vel{4.0F, 5.0F, 6.0F};
    Health health{100};

    TryAddComponentsCmd<Position, Velocity, Health> cmd{entity, std::move(pos), std::move(vel), std::move(health)};
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
  }

  TEST_CASE("TryAddComponentsCmd: some components exist") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});  // Already has Position
    CHECK(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));

    Position pos{7.0F, 8.0F, 9.0F};
    Velocity vel{4.0F, 5.0F, 6.0F};

    TryAddComponentsCmd<Position, Velocity> cmd{entity, std::move(pos), std::move(vel)};
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));  // Should still have Position (original one)
    CHECK(world.HasComponent<Velocity>(entity));  // Should have added Velocity
  }

  TEST_CASE("TryRemoveComponentCmd: basic removal") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));

    TryRemoveComponentCmd<Position> cmd{entity};
    cmd.Execute(world);

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));  // Should remain
  }

  TEST_CASE("TryRemoveComponentCmd: component does not exist") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});

    CHECK(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));

    // Try to remove a component that doesn't exist - should be a no-op
    TryRemoveComponentCmd<Velocity> cmd{entity};
    cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entity));  // Should remain unchanged
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
  }

  TEST_CASE("TryRemoveComponentsCmd: multiple components") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Velocity{4.0F, 5.0F, 6.0F});
    world.AddComponent(entity, Health{100});
    world.AddComponent(entity, Name{"TestEntity"});

    CHECK(world.HasComponent<Position>(entity));
    CHECK(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));
    CHECK(world.HasComponent<Name>(entity));

    TryRemoveComponentsCmd<Position, Velocity> cmd{entity};
    cmd.Execute(world);

    CHECK_FALSE(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));
    CHECK(world.HasComponent<Health>(entity));  // Should remain
    CHECK(world.HasComponent<Name>(entity));    // Should remain
  }

  TEST_CASE("TryRemoveComponentsCmd: some components don't exist") {
    World world;

    Entity entity = world.CreateEntity();
    world.AddComponent(entity, Position{1.0F, 2.0F, 3.0F});
    world.AddComponent(entity, Name{"TestEntity"});

    CHECK(world.HasComponent<Position>(entity));
    CHECK_FALSE(world.HasComponent<Velocity>(entity));  // Doesn't exist
    CHECK_FALSE(world.HasComponent<Health>(entity));    // Doesn't exist
    CHECK(world.HasComponent<Name>(entity));

    TryRemoveComponentsCmd<Position, Velocity, Health> cmd{entity};
    cmd.Execute(world);

    CHECK_FALSE(world.HasComponent<Position>(entity));  // Should be removed
    CHECK_FALSE(world.HasComponent<Velocity>(entity));  // Still doesn't exist
    CHECK_FALSE(world.HasComponent<Health>(entity));    // Still doesn't exist
    CHECK(world.HasComponent<Name>(entity));            // Should remain
  }

  TEST_CASE("TryCommands: copy and move semantics") {
    World world;
    Entity entity = world.CreateEntity();

    // Test copy semantics for TryAddComponentCmd
    TryAddComponentCmd<Position> cmd1{entity, Position{1.0F, 2.0F, 3.0F}};
    TryAddComponentCmd<Position> cmd2 = cmd1;  // Copy constructor
    TryAddComponentCmd<Position> cmd3{entity, Position{4.0F, 5.0F, 6.0F}};
    cmd3 = cmd1;  // Copy assignment

    cmd2.Execute(world);
    CHECK(world.HasComponent<Position>(entity));

    // Test move semantics for TryRemoveComponentCmd
    TryRemoveComponentCmd<Position> cmd4{entity};
    TryRemoveComponentCmd<Position> cmd5 = std::move(cmd4);  // Move constructor
    TryRemoveComponentCmd<Position> cmd6{entity};
    cmd6 = std::move(cmd5);  // Move assignment

    cmd6.Execute(world);
    CHECK_FALSE(world.HasComponent<Position>(entity));
  }

  TEST_CASE("TryCommands: complex scenarios") {
    World world;

    // Create multiple entities
    std::vector<Entity> entities;
    for (int i = 0; i < 5; ++i) {
      entities.push_back(world.CreateEntity());
    }

    // Add components using try commands
    TryAddComponentCmd<Position> try_pos_cmd1{entities[0], Position{1.0F, 2.0F, 3.0F}};
    TryAddComponentCmd<Position> try_pos_cmd2{entities[1], Position{4.0F, 5.0F, 6.0F}};
    TryAddComponentsCmd<Position, Velocity> try_multi_cmd{entities[2], Position{7.0F, 8.0F, 9.0F},
                                                          Velocity{1.0F, 1.0F, 1.0F}};

    try_pos_cmd1.Execute(world);
    try_pos_cmd2.Execute(world);
    try_multi_cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entities[0]));
    CHECK(world.HasComponent<Position>(entities[1]));
    CHECK(world.HasComponent<Position>(entities[2]));
    CHECK_FALSE(world.HasComponent<Velocity>(entities[0]));
    CHECK_FALSE(world.HasComponent<Velocity>(entities[1]));
    CHECK(world.HasComponent<Velocity>(entities[2]));

    // Try to add existing component - should be no-op
    TryAddComponentCmd<Position> try_existing_cmd{entities[0], Position{10.0F, 11.0F, 12.0F}};
    try_existing_cmd.Execute(world);
    CHECK(world.HasComponent<Position>(entities[0]));  // Should still have the original

    // Remove components using try commands
    TryRemoveComponentCmd<Position> try_remove_cmd{entities[1]};
    try_remove_cmd.Execute(world);

    CHECK(world.HasComponent<Position>(entities[0]));
    CHECK_FALSE(world.HasComponent<Position>(entities[1]));
    CHECK(world.HasComponent<Position>(entities[2]));

    // Try to remove non-existent component - should be no-op
    TryRemoveComponentCmd<Velocity> try_remove_nonexistent{entities[1]};
    try_remove_nonexistent.Execute(world);  // Should not crash

    // Try destroy entities
    TryDestroyEntitiesCmd try_destroy_cmd{std::vector<Entity>{entities[3], entities[4]}};
    try_destroy_cmd.Execute(world);

    CHECK(world.Exists(entities[0]));
    CHECK(world.Exists(entities[1]));
    CHECK(world.Exists(entities[2]));
    CHECK_FALSE(world.Exists(entities[3]));
    CHECK_FALSE(world.Exists(entities[4]));
  }

}  // TEST_SUITE
