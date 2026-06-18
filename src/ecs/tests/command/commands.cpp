#include <doctest/doctest.h>

#include <helios/ecs/command/commands.hpp>
#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/world.hpp>

#include <memory_resource>
#include <vector>

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

struct SimpleCommand {
  int* counter = nullptr;

  void Execute(World& /*world*/) const {
    if (counter) {
      ++(*counter);
    }
  }
};

}  // namespace

TEST_SUITE("helios::ecs::Commands") {
  TEST_CASE("ecs::Commands::Spawn") {
    SUBCASE("Returns a buffer for the new entity") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      const auto buf = cmds.Spawn();

      CHECK(buf.GetEntity().Valid());
    }

    SUBCASE("Each Spawn call reserves a different entity") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      const auto buf1 = cmds.Spawn();
      const auto buf2 = cmds.Spawn();

      CHECK_NE(buf1.GetEntity(), buf2.GetEntity());
    }

    SUBCASE("Entity is accessible in world after queue execution") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      const Entity entity = [&cmds] {
        auto buf = cmds.Spawn();
        return buf.GetEntity();
      }();

      world.Update();
      queue.ExecuteAll(world);

      CHECK(world.Exists(entity));
    }

    SUBCASE("Spawn and add component lands in world after execution") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      const Entity entity = [&cmds] {
        auto buf = cmds.Spawn();
        buf.AddComponents(Position{1.0F, 2.0F});
        return buf.GetEntity();
      }();

      world.Update();
      queue.ExecuteAll(world);

      CHECK_EQ(world.ReadComponent<Position>(entity).x, doctest::Approx(1.0F));
    }
  }

  TEST_CASE("ecs::Commands::Despawn") {
    SUBCASE("Enqueues destroy command; entity removed after execution") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      cmds.Despawn(entity);
      queue.ExecuteAll(world);

      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("Multiple entities can be despawned independently") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      cmds.Despawn(e1);
      cmds.Despawn(e2);
      queue.ExecuteAll(world);

      CHECK_FALSE(world.Exists(e1));
      CHECK_FALSE(world.Exists(e2));
    }
  }

  TEST_CASE("ecs::Commands::Entity") {
    SUBCASE("Returns buffer for existing entity") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      const auto buf = cmds.Entity(entity);

      CHECK_EQ(buf.GetEntity(), entity);
    }

    SUBCASE("Enqueued operations execute on the correct entity") {
      World world;
      const Entity entity = world.CreateEntity();
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      {
        auto buf = cmds.Entity(entity);
        buf.AddComponents(Velocity{3.0F, 4.0F});
      }

      queue.ExecuteAll(world);

      CHECK_EQ(world.ReadComponent<Velocity>(entity).dx, doctest::Approx(3.0F));
    }
  }

  TEST_CASE("ecs::Commands::World") {
    SUBCASE("Returns a world command buffer") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      {
        auto buf = cmds.World();
        buf.InsertResource(Score{42});
      }

      queue.ExecuteAll(world);

      CHECK_EQ(world.ReadResource<Score>().value, 42);
    }

    SUBCASE("Multiple World() calls each return independent buffers") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      {
        auto buf1 = cmds.World();
        buf1.InsertResource(Score{1});
      }

      {
        auto buf2 = cmds.World();
        buf2.InsertResource(Health{});
      }

      queue.ExecuteAll(world);

      CHECK(world.HasResource<Score>());
      CHECK(world.HasResource<Health>());
    }
  }

  TEST_CASE("ecs::Commands::Enqueue") {
    SUBCASE("Enqueues a single command that executes against the world") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      int counter = 0;
      cmds.Enqueue(SimpleCommand{&counter});
      queue.ExecuteAll(world);

      CHECK_EQ(counter, 1);
    }

    SUBCASE("Multiple Enqueue calls preserve order") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      int counter = 0;
      cmds.Enqueue(SimpleCommand{&counter});
      cmds.Enqueue(SimpleCommand{&counter});
      cmds.Enqueue(SimpleCommand{&counter});
      queue.ExecuteAll(world);

      CHECK_EQ(counter, 3);
    }
  }

  TEST_CASE("ecs::Commands::EnqueueBulk") {
    SUBCASE("Enqueues all commands in the range") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      int counter = 0;
      const std::vector<SimpleCommand> commands = {
          {&counter}, {&counter}, {&counter}, {&counter}};

      cmds.EnqueueBulk(commands);
      queue.ExecuteAll(world);

      CHECK_EQ(counter, 4);
    }

    SUBCASE("Empty range is a no-op") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      const std::vector<SimpleCommand> empty;
      cmds.EnqueueBulk(empty);
      queue.ExecuteAll(world);
    }

    SUBCASE("Rvalue range is accepted") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());
      Commands cmds(queue, world);

      int counter = 0;
      cmds.EnqueueBulk(std::vector<SimpleCommand>{{&counter}, {&counter}});
      queue.ExecuteAll(world);

      CHECK_EQ(counter, 2);
    }
  }
}
