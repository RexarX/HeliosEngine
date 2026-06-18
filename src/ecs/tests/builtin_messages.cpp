#include <doctest/doctest.h>

#include <helios/ecs/builtin_messages.hpp>
#include <helios/ecs/world.hpp>

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

struct Health {
  int value = 100;
};

struct Score {
  int value = 0;
};

template <MessageTrait T>
[[nodiscard]] size_t MessageCount(const World& world) noexcept {
  return world.ReadMessages<T>().Count();
}

}  // namespace

TEST_SUITE("helios::ecs::EntityAddedMsg") {
  TEST_CASE("ecs::EntityAddedMsg::GetEntity") {
    SUBCASE("Stores and returns the entity it was constructed with") {
      World world;
      const Entity entity = world.CreateEntity();

      const EntityAddedMsg msg(entity);

      CHECK_EQ(msg.GetEntity(), entity);
    }

    SUBCASE("Copy constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      const EntityAddedMsg original(entity);
      const EntityAddedMsg copy = original;  // NOLINT

      CHECK_EQ(copy.GetEntity(), entity);
    }

    SUBCASE("Move constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      EntityAddedMsg original(entity);
      const EntityAddedMsg moved = std::move(original);

      CHECK_EQ(moved.GetEntity(), entity);
    }
  }

  TEST_CASE("ecs::EntityAddedMsg: emitted by World::CreateEntity") {
    SUBCASE("One message is emitted per created entity") {
      World world;
      world.AddMessage<EntityAddedMsg>();

      const Entity entity = world.CreateEntity();
      world.Update();

      const auto reader = world.ReadMessages<EntityAddedMsg>();
      CHECK_EQ(reader.Count(), 1);
      CHECK_EQ(reader.PreviousMessages()[0].GetEntity(), entity);
    }

    SUBCASE("Two messages are emitted when two entities are created") {
      World world;
      world.AddMessage<EntityAddedMsg>();

      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      world.Update();

      const auto reader = world.ReadMessages<EntityAddedMsg>();
      CHECK_EQ(reader.Count(), 2);

      const auto messages = reader.Collect();
      const bool found_e1 = std::ranges::any_of(
          messages,
          [e1](const auto& message) { return message.GetEntity() == e1; });
      const bool found_e2 = std::ranges::any_of(
          messages,
          [e2](const auto& message) { return message.GetEntity() == e2; });

      CHECK(found_e1);
      CHECK(found_e2);
    }

    SUBCASE("Messages are cleared after the next Update") {
      World world;
      world.AddMessage<EntityAddedMsg>();

      [[maybe_unused]] const Entity entity = world.CreateEntity();
      world.Update();  // messages land in previous slot
      world.Update();  // previous slot is cleared

      CHECK_EQ(MessageCount<EntityAddedMsg>(world), 0);
    }
  }

  TEST_CASE("ecs::EntityAddedMsg: static trait members") {
    SUBCASE("kName is EntityAddedMsg") {
      CHECK_EQ(EntityAddedMsg::kName, "EntityAddedMsg");
    }

    SUBCASE("kClearPolicy is automatic") {
      CHECK_EQ(EntityAddedMsg::kClearPolicy, MessageClearPolicy::kAutomatic);
    }

    SUBCASE("kConsumable is false") {
      CHECK_FALSE(EntityAddedMsg::kConsumable);
    }

    SUBCASE("kAsync is false") {
      CHECK_FALSE(EntityAddedMsg::kAsync);
    }
  }
}

TEST_SUITE("ecs::EntityDestroyedMsg") {
  TEST_CASE("EntityDestroyedMsg::GetEntity") {
    SUBCASE("Stores and returns the entity it was constructed with") {
      World world;
      const Entity entity = world.CreateEntity();

      const EntityDestroyedMsg msg(entity);

      CHECK_EQ(msg.GetEntity(), entity);
    }

    SUBCASE("Copy constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      const EntityDestroyedMsg original(entity);
      const EntityDestroyedMsg copy = original;  // NOLINT

      CHECK_EQ(copy.GetEntity(), entity);
    }

    SUBCASE("Move constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      EntityDestroyedMsg original(entity);
      const EntityDestroyedMsg moved = std::move(original);

      CHECK_EQ(moved.GetEntity(), entity);
    }
  }

  TEST_CASE("ecs::EntityDestroyedMsg: emitted by World::DestroyEntity") {
    SUBCASE("One message is emitted per destroyed entity") {
      World world;
      const Entity entity = world.CreateEntity();
      world.Update();

      world.DestroyEntity(entity);
      world.Update();

      const auto reader = world.ReadMessages<EntityDestroyedMsg>();
      CHECK_EQ(reader.Count(), 1);
      CHECK_EQ(reader.PreviousMessages()[0].GetEntity(), entity);
    }

    SUBCASE("Two messages are emitted when two entities are destroyed") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      world.Update();

      world.DestroyEntity(e1);
      world.DestroyEntity(e2);
      world.Update();

      CHECK_EQ(MessageCount<EntityDestroyedMsg>(world), 2);
    }

    SUBCASE("Messages are cleared after the next Update") {
      World world;
      const Entity entity = world.CreateEntity();
      world.Update();

      world.DestroyEntity(entity);
      world.Update();  // messages land in previous slot
      world.Update();  // previous slot is cleared

      CHECK_EQ(MessageCount<EntityDestroyedMsg>(world), 0);
    }
  }

  TEST_CASE("ecs::EntityDestroyedMsg: static trait members") {
    SUBCASE("kName is EntityDestroyedMsg") {
      CHECK_EQ(EntityDestroyedMsg::kName, "EntityDestroyedMsg");
    }

    SUBCASE("kClearPolicy is automatic") {
      CHECK_EQ(EntityDestroyedMsg::kClearPolicy,
               MessageClearPolicy::kAutomatic);
    }

    SUBCASE("kConsumable is false") {
      CHECK_FALSE(EntityDestroyedMsg::kConsumable);
    }

    SUBCASE("kAsync is false") {
      CHECK_FALSE(EntityDestroyedMsg::kAsync);
    }
  }
}

TEST_SUITE("helios::ecs::ComponentAddedMsg") {
  TEST_CASE("ecs::ComponentAddedMsg::GetEntity") {
    SUBCASE("Stores and returns the entity it was constructed with") {
      World world;
      const Entity entity = world.CreateEntity();

      const ComponentAddedMsg<Position> msg(entity);

      CHECK_EQ(msg.GetEntity(), entity);
    }

    SUBCASE("Copy constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      ComponentAddedMsg<Position> original{entity};
      ComponentAddedMsg<Position> copy = original;  // NOLINT

      CHECK_EQ(copy.GetEntity(), entity);
    }

    SUBCASE("Move constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      ComponentAddedMsg<Position> original(entity);
      const ComponentAddedMsg<Position> moved = std::move(original);

      CHECK_EQ(moved.GetEntity(), entity);
    }
  }

  TEST_CASE("ecs::ComponentAddedMsg: emitted by World::AddComponent") {
    SUBCASE("One message is emitted when a component is added") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddMessage<ComponentAddedMsg<Position>>();

      world.AddComponents(entity, Position{});
      world.Update();

      const auto reader = world.ReadMessages<ComponentAddedMsg<Position>>();
      CHECK_EQ(reader.Count(), 1);
      CHECK_EQ(reader.PreviousMessages()[0].GetEntity(), entity);
    }

    SUBCASE("Component types are tracked independently") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddMessage<ComponentAddedMsg<Position>>();
      world.AddMessage<ComponentAddedMsg<Velocity>>();

      world.AddComponents(entity, Position{});
      world.AddComponents(entity, Velocity{});
      world.Update();

      CHECK_EQ(MessageCount<ComponentAddedMsg<Position>>(world), 1);
      CHECK_EQ(MessageCount<ComponentAddedMsg<Velocity>>(world), 1);
    }

    SUBCASE("Messages are cleared after the next Update") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddMessage<ComponentAddedMsg<Position>>();

      world.AddComponents(entity, Position{});
      world.Update();  // messages land in previous slot
      world.Update();  // previous slot is cleared

      CHECK_EQ(MessageCount<ComponentAddedMsg<Position>>(world), 0);
    }
  }

  TEST_CASE("ecs::ComponentAddedMsg: static trait members") {
    SUBCASE("kName is ComponentAddedMsg") {
      CHECK_EQ(ComponentAddedMsg<Position>::kName, "ComponentAddedMsg");
    }

    SUBCASE("kClearPolicy is automatic") {
      CHECK_EQ(ComponentAddedMsg<Position>::kClearPolicy,
               MessageClearPolicy::kAutomatic);
    }

    SUBCASE("kConsumable is false") {
      CHECK_FALSE(ComponentAddedMsg<Position>::kConsumable);
    }

    SUBCASE("kAsync is false") {
      CHECK_FALSE(ComponentAddedMsg<Position>::kAsync);
    }
  }
}

TEST_SUITE("helios::ecs::ComponentRemovedMsg") {
  TEST_CASE("ecs::ComponentRemovedMsg::GetEntity") {
    SUBCASE("Stores and returns the entity it was constructed with") {
      World world;
      const Entity entity = world.CreateEntity();

      const ComponentRemovedMsg<Position> msg(entity);

      CHECK_EQ(msg.GetEntity(), entity);
    }

    SUBCASE("Copy constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      const ComponentRemovedMsg<Position> original(entity);
      const ComponentRemovedMsg<Position> copy = original;  // NOLINT

      CHECK_EQ(copy.GetEntity(), entity);
    }

    SUBCASE("Move constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      ComponentRemovedMsg<Position> original(entity);
      const ComponentRemovedMsg<Position> moved = std::move(original);

      CHECK_EQ(moved.GetEntity(), entity);
    }
  }

  TEST_CASE("ecs::ComponentRemovedMsg: emitted by World::RemoveComponent") {
    SUBCASE("One message is emitted when a component is removed") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      world.AddMessage<ComponentRemovedMsg<Position>>();

      world.RemoveComponents<Position>(entity);
      world.Update();

      const auto reader = world.ReadMessages<ComponentRemovedMsg<Position>>();
      CHECK_EQ(reader.Count(), 1);
      CHECK_EQ(reader.PreviousMessages()[0].GetEntity(), entity);
    }

    SUBCASE("Component types are tracked independently") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{});
      world.AddMessage<ComponentRemovedMsg<Position>>();
      world.AddMessage<ComponentRemovedMsg<Velocity>>();

      world.RemoveComponents<Position>(entity);
      world.RemoveComponents<Velocity>(entity);
      world.Update();

      CHECK_EQ(MessageCount<ComponentRemovedMsg<Position>>(world), 1);
      CHECK_EQ(MessageCount<ComponentRemovedMsg<Velocity>>(world), 1);
    }

    SUBCASE("Messages are cleared after the next Update") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      world.AddMessage<ComponentRemovedMsg<Position>>();

      world.RemoveComponents<Position>(entity);
      world.Update();  // messages land in previous slot
      world.Update();  // previous slot is cleared

      CHECK_EQ(MessageCount<ComponentRemovedMsg<Position>>(world), 0);
    }
  }

  TEST_CASE("ecs::ComponentRemovedMsg: static trait members") {
    SUBCASE("kName is ComponentRemovedMsg") {
      CHECK_EQ(ComponentRemovedMsg<Position>::kName, "ComponentRemovedMsg");
    }

    SUBCASE("kClearPolicy is automatic") {
      CHECK_EQ(ComponentRemovedMsg<Position>::kClearPolicy,
               MessageClearPolicy::kAutomatic);
    }

    SUBCASE("kConsumable is false") {
      CHECK_FALSE(ComponentRemovedMsg<Position>::kConsumable);
    }

    SUBCASE("kAsync is false") {
      CHECK_FALSE(ComponentRemovedMsg<Position>::kAsync);
    }
  }
}

TEST_SUITE("helios::ecs::ComponentsClearedMsg") {
  TEST_CASE("ecs::ComponentsClearedMsg::GetEntity") {
    SUBCASE("Stores and returns the entity it was constructed with") {
      World world;
      const Entity entity = world.CreateEntity();

      const ComponentsClearedMsg msg(entity);

      CHECK_EQ(msg.GetEntity(), entity);
    }

    SUBCASE("Copy constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      const ComponentsClearedMsg original(entity);
      const ComponentsClearedMsg copy = original;  // NOLINT

      CHECK_EQ(copy.GetEntity(), entity);
    }

    SUBCASE("Move constructs and preserves entity") {
      World world;
      const Entity entity = world.CreateEntity();

      ComponentsClearedMsg original(entity);
      const ComponentsClearedMsg moved = std::move(original);

      CHECK_EQ(moved.GetEntity(), entity);
    }
  }

  TEST_CASE("ecs::ComponentsClearedMsg: emitted by World::ClearComponents") {
    SUBCASE("One message is emitted per cleared entity") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{});

      world.ClearComponents(entity);
      world.Update();

      const auto reader = world.ReadMessages<ComponentsClearedMsg>();
      CHECK_EQ(reader.Count(), 1);
      CHECK_EQ(reader.PreviousMessages()[0].GetEntity(), entity);
    }

    SUBCASE("Two messages are emitted when two entities are cleared") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      world.AddComponents(e1, Position{});
      world.AddComponents(e2, Velocity{});

      world.ClearComponents(e1);
      world.ClearComponents(e2);
      world.Update();

      CHECK_EQ(MessageCount<ComponentsClearedMsg>(world), 2);
    }

    SUBCASE("One message is emitted even when entity had no components") {
      World world;
      const Entity entity = world.CreateEntity();

      world.ClearComponents(entity);
      world.Update();

      CHECK_EQ(MessageCount<ComponentsClearedMsg>(world), 1);
    }

    SUBCASE("Messages are cleared after the next Update") {
      World world;
      const Entity entity = world.CreateEntity();

      world.ClearComponents(entity);
      world.Update();  // messages land in previous slot
      world.Update();  // previous slot is cleared

      CHECK_EQ(MessageCount<ComponentsClearedMsg>(world), 0);
    }
  }

  TEST_CASE("ecs::ComponentsClearedMsg: static trait members") {
    SUBCASE("kName is ComponentsClearedMsg") {
      CHECK_EQ(ComponentsClearedMsg::kName, "ComponentsClearedMsg");
    }

    SUBCASE("kClearPolicy is automatic") {
      CHECK_EQ(ComponentsClearedMsg::kClearPolicy,
               MessageClearPolicy::kAutomatic);
    }

    SUBCASE("kConsumable is false") {
      CHECK_FALSE(ComponentsClearedMsg::kConsumable);
    }

    SUBCASE("kAsync is false") {
      CHECK_FALSE(ComponentsClearedMsg::kAsync);
    }
  }
}

TEST_SUITE("helios::ecs::ResourceInsertedMsg") {
  TEST_CASE("ecs::ResourceInsertedMsg: emitted by World::InsertResource") {
    SUBCASE("One message is emitted when a resource is inserted") {
      World world;
      world.AddMessage<ResourceInsertedMsg<Score>>();

      world.InsertResources(Score{42});
      world.Update();

      CHECK_EQ(MessageCount<ResourceInsertedMsg<Score>>(world), 1);
    }

    SUBCASE("Replacing an existing resource also emits a message") {
      World world;
      world.AddMessage<ResourceInsertedMsg<Score>>();
      world.InsertResources(Score{1});
      world.Update();
      world.Update();  // flush first message out of view

      world.InsertResources(Score{2});
      world.Update();

      CHECK_EQ(MessageCount<ResourceInsertedMsg<Score>>(world), 1);
    }

    SUBCASE("Resource types are tracked independently") {
      World world;
      world.AddMessage<ResourceInsertedMsg<Score>>();
      world.AddMessage<ResourceInsertedMsg<Health>>();

      world.InsertResources(Score{});
      world.InsertResources(Health{});
      world.Update();

      CHECK_EQ(MessageCount<ResourceInsertedMsg<Score>>(world), 1);
      CHECK_EQ(MessageCount<ResourceInsertedMsg<Health>>(world), 1);
    }

    SUBCASE("Messages are cleared after the next Update") {
      World world;
      world.AddMessage<ResourceInsertedMsg<Score>>();

      world.InsertResources(Score{});
      world.Update();  // messages land in previous slot
      world.Update();  // previous slot is cleared

      CHECK_EQ(MessageCount<ResourceInsertedMsg<Score>>(world), 0);
    }
  }

  TEST_CASE("ecs::ResourceInsertedMsg: emitted by World::TryInsertResource") {
    SUBCASE("Message is emitted on successful insertion") {
      World world;
      world.AddMessage<ResourceInsertedMsg<Score>>();

      world.TryInsertResources(Score{});
      world.Update();

      CHECK_EQ(MessageCount<ResourceInsertedMsg<Score>>(world), 1);
    }

    SUBCASE("No message is emitted when resource already exists") {
      World world;
      world.AddMessage<ResourceInsertedMsg<Score>>();
      world.InsertResources(Score{1});
      world.Update();
      world.Update();  // flush first message out of view

      world.TryInsertResources(Score{99});
      world.Update();

      CHECK_EQ(MessageCount<ResourceInsertedMsg<Score>>(world), 0);
    }
  }

  TEST_CASE("ecs::ResourceInsertedMsg: static trait members") {
    SUBCASE("kName is ResourceInsertedMsg") {
      CHECK_EQ(ResourceInsertedMsg<Score>::kName, "ResourceInsertedMsg");
    }

    SUBCASE("kClearPolicy is automatic") {
      CHECK_EQ(ResourceInsertedMsg<Score>::kClearPolicy,
               MessageClearPolicy::kAutomatic);
    }

    SUBCASE("kConsumable is false") {
      CHECK_FALSE(ResourceInsertedMsg<Score>::kConsumable);
    }

    SUBCASE("kAsync is false") {
      CHECK_FALSE(ResourceInsertedMsg<Score>::kAsync);
    }
  }
}

TEST_SUITE("helios::ecs::ResourceRemovedMsg") {
  TEST_CASE("ecs::ResourceRemovedMsg: emitted by World::RemoveResource") {
    SUBCASE("One message is emitted when a resource is removed") {
      World world;
      world.AddMessage<ResourceRemovedMsg<Score>>();
      world.InsertResources(Score{});
      world.Update();
      world.Update();  // flush insertion message out of view

      world.RemoveResources<Score>();
      world.Update();

      CHECK_EQ(MessageCount<ResourceRemovedMsg<Score>>(world), 1);
    }

    SUBCASE("Resource types are tracked independently") {
      World world;
      world.AddMessage<ResourceRemovedMsg<Score>>();
      world.AddMessage<ResourceRemovedMsg<Health>>();
      world.InsertResources(Score{});
      world.InsertResources(Health{});
      world.Update();
      world.Update();

      world.RemoveResources<Score>();
      world.RemoveResources<Health>();
      world.Update();

      CHECK_EQ(MessageCount<ResourceRemovedMsg<Score>>(world), 1);
      CHECK_EQ(MessageCount<ResourceRemovedMsg<Health>>(world), 1);
    }

    SUBCASE("Messages are cleared after the next Update") {
      World world;
      world.AddMessage<ResourceRemovedMsg<Score>>();
      world.InsertResources(Score{});
      world.Update();

      world.RemoveResources<Score>();
      world.Update();  // messages land in previous slot
      world.Update();  // previous slot is cleared

      CHECK_EQ(MessageCount<ResourceRemovedMsg<Score>>(world), 0);
    }
  }

  TEST_CASE("ecs::ResourceRemovedMsg: emitted by World::TryRemoveResource") {
    SUBCASE("Message is emitted on successful removal") {
      World world;
      world.AddMessage<ResourceRemovedMsg<Score>>();
      world.InsertResources(Score{});
      world.Update();
      world.Update();

      world.TryRemoveResources<Score>();
      world.Update();

      CHECK_EQ(MessageCount<ResourceRemovedMsg<Score>>(world), 1);
    }

    SUBCASE("No message is emitted when resource does not exist") {
      World world;
      world.AddMessage<ResourceRemovedMsg<Score>>();

      world.TryRemoveResources<Score>();
      world.Update();

      CHECK_EQ(MessageCount<ResourceRemovedMsg<Score>>(world), 0);
    }
  }

  TEST_CASE("ecs::ResourceRemovedMsg: static trait members") {
    SUBCASE("kName is ResourceRemovedMsg") {
      CHECK_EQ(ResourceRemovedMsg<Score>::kName, "ResourceRemovedMsg");
    }

    SUBCASE("kClearPolicy is automatic") {
      CHECK_EQ(ResourceRemovedMsg<Score>::kClearPolicy,
               MessageClearPolicy::kAutomatic);
    }

    SUBCASE("kConsumable is false") {
      CHECK_FALSE(ResourceRemovedMsg<Score>::kConsumable);
    }

    SUBCASE("kAsync is false") {
      CHECK_FALSE(ResourceRemovedMsg<Score>::kAsync);
    }
  }
}
