#include <doctest/doctest.h>

#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/world.hpp>
#include <helios/ecs/world_view.hpp>

using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
};

struct DeltaTime {
  float value = 0.0F;
};

struct GameMsg {
  int value = 0;
};

}  // namespace

TEST_SUITE("helios::ecs::WorldView") {
  TEST_CASE("ecs::WorldView::ctor") {
    SUBCASE("Constructor stores world reference") {
      const World world;
      const WorldView view(world);
      CHECK_EQ(view.EntityCount(), 0);
    }
  }

  TEST_CASE("ecs::WorldView::EntityExists") {
    SUBCASE("EntityExists returns true for a living entity") {
      World world;
      const Entity entity = world.CreateEntity();
      const WorldView view(world);
      CHECK(view.EntityExists(entity));
    }
  }

  TEST_CASE("ecs::WorldView::HasComponent") {
    SUBCASE("HasComponent returns true when entity has the component") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{1.0F, 2.0F});
      const WorldView view(world);
      CHECK(view.HasComponent<Position>(entity));
    }

    SUBCASE("HasComponent returns false when entity lacks the component") {
      World world;
      const Entity entity = world.CreateEntity();
      const WorldView view(world);
      CHECK_FALSE(view.HasComponent<Position>(entity));
    }

    SUBCASE("HasComponent returns false after component is removed") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      world.RemoveComponents<Position>(entity);
      const WorldView view(world);
      CHECK_FALSE(view.HasComponent<Position>(entity));
    }
  }

  TEST_CASE("ecs::WorldView::HasComponents") {
    SUBCASE(
        "HasComponents returns correct flags for present and absent types") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      const WorldView view(world);
      const auto result = view.HasComponents<Position, Velocity>(entity);
      CHECK(result[0]);
      CHECK_FALSE(result[1]);
    }

    SUBCASE("HasComponents returns all true when entity has all components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{});
      const WorldView view(world);
      const auto result = view.HasComponents<Position, Velocity>(entity);
      CHECK(result[0]);
      CHECK(result[1]);
    }
  }

  TEST_CASE("ecs::WorldView::HasResource") {
    SUBCASE("HasResource returns true when resource exists") {
      World world;
      world.InsertResources(DeltaTime{0.016F});
      const WorldView view(world);
      CHECK(view.HasResource<DeltaTime>());
    }

    SUBCASE("HasResource returns false when resource does not exist") {
      const World world;
      const WorldView view(world);
      CHECK_FALSE(view.HasResource<DeltaTime>());
    }

    SUBCASE("HasResource returns false after resource is removed") {
      World world;
      world.InsertResources(DeltaTime{});
      world.RemoveResources<DeltaTime>();
      const WorldView view(world);
      CHECK_FALSE(view.HasResource<DeltaTime>());
    }
  }

  TEST_CASE("ecs::WorldView::HasMessage") {
    SUBCASE("HasMessage returns true for a registered built-in message type") {
      const World world;
      const WorldView view(world);
      CHECK(view.HasMessage<EntityAddedMsg>());
    }

    SUBCASE("HasMessage returns true after registering a custom message") {
      World world;
      world.AddMessage<GameMsg>();
      const WorldView view(world);
      CHECK(view.HasMessage<GameMsg>());
    }

    SUBCASE("HasMessage returns false for an unregistered message type") {
      const World world;
      const WorldView view(world);
      CHECK_FALSE(view.HasMessage<GameMsg>());
    }
  }

  TEST_CASE("ecs::WorldView::HasMessages") {
    SUBCASE("HasMessages returns true when messages are in the queue") {
      World world;
      const WorldView view(world);
      world.AddMessage<GameMsg>();
      world.WriteMessages<GameMsg>().Write(GameMsg{42});
      world.Update();
      CHECK(view.HasMessages<GameMsg>());
    }

    SUBCASE("HasMessages returns false when no messages have been written") {
      World world;
      world.AddMessage<GameMsg>();
      world.Update();
      const WorldView view(world);
      CHECK_FALSE(view.HasMessages<GameMsg>());
    }
  }

  TEST_CASE("ecs::WorldView::EntityCount") {
    SUBCASE("EntityCount returns 0 for an empty world") {
      const World world;
      const WorldView view(world);
      CHECK_EQ(view.EntityCount(), 0);
    }

    SUBCASE("EntityCount increases after entity creation") {
      World world;
      [[maybe_unused]] const auto e1 = world.CreateEntity();
      [[maybe_unused]] const auto e2 = world.CreateEntity();
      const WorldView view(world);
      CHECK_EQ(view.EntityCount(), 2);
    }

    SUBCASE("EntityCount decreases after entity destruction") {
      World world;
      const Entity entity = world.CreateEntity();
      world.DestroyEntity(entity);
      const WorldView view(world);
      CHECK_EQ(view.EntityCount(), 0);
    }
  }

  TEST_CASE("ecs::WorldView::ResourceCount") {
    SUBCASE("ResourceCount returns 0 for an empty world") {
      const World world;
      const WorldView view(world);
      CHECK_EQ(view.ResourceCount(), 0);
    }

    SUBCASE("ResourceCount increases after resource insertion") {
      World world;
      world.InsertResources(DeltaTime{1.0F});
      const WorldView view(world);
      CHECK_EQ(view.ResourceCount(), 1);
    }

    SUBCASE("ResourceCount decreases after resource removal") {
      World world;
      world.InsertResources(DeltaTime{});
      world.RemoveResources<DeltaTime>();
      const WorldView view(world);
      CHECK_EQ(view.ResourceCount(), 0);
    }
  }
}
