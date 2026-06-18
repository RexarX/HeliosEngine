#include <doctest/doctest.h>

#include <helios/ecs/command/entity_buffer.hpp>
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/entity/entity.hpp>
#include <helios/ecs/world.hpp>

#include <memory_resource>

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

}  // namespace

TEST_SUITE("helios::ecs::EntityCmdBuffer") {
  TEST_CASE("ecs::EntityCmdBuffer::ctor") {
    SUBCASE("Custom allocator ctor") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());

      CHECK(buf.Empty());
      CHECK_EQ(buf.Size(), 0);
      CHECK_EQ(buf.GetEntity(), entity);
    }

    SUBCASE("PMR resource ctor") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());

      CHECK(buf.Empty());
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::Clear") {
    SUBCASE("Clear removes all pending commands") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      buf.AddComponents(Position{});
      buf.AddComponents(Velocity{});

      CHECK_EQ(buf.Size(), 2);

      buf.Clear();

      CHECK(buf.Empty());
      CHECK_EQ(buf.Size(), 0);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::Reserve") {
    SUBCASE("Reserve does not add commands") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      buf.Reserve(10);

      CHECK(buf.Empty());
      CHECK_EQ(buf.Size(), 0);
    }

    SUBCASE("Can enqueue after reserve") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      buf.Reserve(5);
      buf.AddComponents(Position{});

      CHECK_EQ(buf.Size(), 1);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::Destroy") {
    SUBCASE("Enqueues destroy and entity is removed after execution") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.Destroy();
        CHECK_EQ(buf.Size(), 1);
      }  // destructor flushes buf into queue

      queue.ExecuteAll(world);

      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("Returns self for chaining") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      auto& ref = buf.Destroy();

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::TryDestroy") {
    SUBCASE("Enqueues try-destroy and entity is removed after execution") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.TryDestroy();
      }

      queue.ExecuteAll(world);

      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("Does not assert when entity does not exist in world") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.TryDestroy();
      }

      // Reserved entity never committed — TryDestroy must be a silent no-op
      queue.ExecuteAll(world);
    }

    SUBCASE("Returns self for chaining") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      auto& ref = buf.TryDestroy();

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::AddComponents") {
    SUBCASE("Adds multiple components after execution") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.AddComponents(Position{1.0F, 2.0F}, Velocity{3.0F, 4.0F});
      }

      queue.ExecuteAll(world);

      CHECK(world.HasComponent<Position>(entity));
      CHECK(world.HasComponent<Velocity>(entity));
      CHECK_EQ(world.ReadComponent<Velocity>(entity).dx, doctest::Approx(3.0F));
    }

    SUBCASE("Returns self for chaining") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      auto& ref = buf.AddComponents(Position{}, Velocity{});

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::TryAddComponents") {
    SUBCASE("Adds only missing components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{7.0F, 7.0F});
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.TryAddComponents(Position{0.0F, 0.0F}, Velocity{1.0F, 2.0F});
      }

      queue.ExecuteAll(world);

      CHECK_EQ(world.ReadComponent<Position>(entity).x, doctest::Approx(7.0F));
      CHECK(world.HasComponent<Velocity>(entity));
    }

    SUBCASE("Returns self for chaining") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      auto& ref = buf.TryAddComponents(Position{}, Velocity{});

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::RemoveComponents") {
    SUBCASE("Removes multiple components after execution") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{});
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.RemoveComponents<Position, Velocity>();
      }

      queue.ExecuteAll(world);

      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
    }

    SUBCASE("Returns self for chaining") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{});
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      auto& ref = buf.RemoveComponents<Position, Velocity>();

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::TryRemoveComponents") {
    SUBCASE("Removes present components, ignores absent ones") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.TryRemoveComponents<Position, Velocity>();
      }

      queue.ExecuteAll(world);

      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
    }

    SUBCASE("Returns self for chaining") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      auto& ref = buf.TryRemoveComponents<Position, Velocity>();

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::ClearComponents") {
    SUBCASE("Removes all components after execution") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{}, Health{100});
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.ClearComponents();
      }

      queue.ExecuteAll(world);

      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
      CHECK_FALSE(world.HasComponent<Health>(entity));
    }

    SUBCASE("Returns self for chaining") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      auto& ref = buf.ClearComponents();

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::Empty") {
    SUBCASE("Buffer is empty initially") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());

      CHECK(buf.Empty());
    }

    SUBCASE("Buffer not empty after enqueue") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      buf.AddComponents(Position{});

      CHECK_FALSE(buf.Empty());
    }

    SUBCASE("Buffer empty after clear") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      buf.AddComponents(Position{});
      buf.Clear();

      CHECK(buf.Empty());
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::GetEntity") {
    SUBCASE("Returns the entity passed at construction") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());

      CHECK_EQ(buf.GetEntity(), entity);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::Size") {
    SUBCASE("Size is zero initially") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());

      CHECK_EQ(buf.Size(), 0);
    }

    SUBCASE("Size increases after each enqueue") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      buf.AddComponents(Position{});
      CHECK_EQ(buf.Size(), 1);

      buf.AddComponents(Velocity{});
      CHECK_EQ(buf.Size(), 2);
    }

    SUBCASE("Size is zero after clear") {
      World world;
      const Entity entity = world.ReserveEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
      buf.AddComponents(Position{});
      buf.AddComponents(Velocity{});
      buf.Clear();

      CHECK_EQ(buf.Size(), 0);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer::GetAllocator") {
    SUBCASE("Returns the allocator used at construction") {
      World world;
      const Entity entity = world.ReserveEntity();
      auto* resource = std::pmr::get_default_resource();
      PmrCmdQueue queue(resource);

      EntityCmdBuffer buf(entity, queue, resource);
      const auto alloc = buf.GetAllocator();

      CHECK_EQ(alloc.resource(), resource);
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer: destructor flushes commands into queue") {
    SUBCASE("Commands are in queue after buffer is destroyed") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.AddComponents(Position{5.0F, 6.0F});
        buf.AddComponents(Velocity{1.0F, 2.0F});

        CHECK_EQ(queue.Size(), 0);
      }  // destructor merges buf into queue

      CHECK_EQ(queue.Size(), 2);
      queue.ExecuteAll(world);

      CHECK(world.HasComponent<Position>(entity));
      CHECK(world.HasComponent<Velocity>(entity));
    }
  }

  TEST_CASE("ecs::EntityCmdBuffer: method chaining") {
    SUBCASE("Multiple operations can be chained") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        EntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.AddComponents(Position{1.0F, 2.0F}, Velocity{3.0F, 4.0F})
            .TryAddComponents(Health{50});
      }

      queue.ExecuteAll(world);

      CHECK(world.HasComponent<Position>(entity));
      CHECK(world.HasComponent<Velocity>(entity));
      CHECK(world.HasComponent<Health>(entity));
    }
  }

  TEST_CASE("ecs::PmrEntityCmdBuffer: works with pmr resource") {
    SUBCASE("PMR alias operates correctly") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        PmrEntityCmdBuffer buf(entity, queue, std::pmr::get_default_resource());
        buf.AddComponents(Position{7.0F, 8.0F});
      }

      queue.ExecuteAll(world);

      CHECK(world.HasComponent<Position>(entity));
    }
  }
}
