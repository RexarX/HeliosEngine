#include <doctest/doctest.h>

#include <helios/ecs/command/queue.hpp>
#include <helios/ecs/command/world_buffer.hpp>
#include <helios/ecs/world.hpp>

#include <memory_resource>

using namespace helios::ecs;

namespace {

struct Score {
  int value = 0;
};

struct Config {
  bool enabled = false;
};

struct Timer {
  float seconds = 0.0F;
};

}  // namespace

TEST_SUITE("helios::ecs::WorldCmdBuffer") {
  TEST_CASE("ecs::WorldCmdBuffer::ctor") {
    SUBCASE("Custom allocator ctor") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      CHECK(buf.Empty());
      CHECK_EQ(buf.Size(), 0);
    }

    SUBCASE("PMR resource ctor") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      PmrWorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      CHECK(buf.Empty());
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::Clear") {
    SUBCASE("Clear removes all pending commands") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      buf.InsertResource(Score{1});
      buf.InsertResource(Timer{});

      CHECK_EQ(buf.Size(), 2);

      buf.Clear();

      CHECK(buf.Empty());
      CHECK_EQ(buf.Size(), 0);
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::Reserve") {
    SUBCASE("Reserve does not add commands") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      buf.Reserve(10);

      CHECK(buf.Empty());
      CHECK_EQ(buf.Size(), 0);
    }

    SUBCASE("Can enqueue after reserve") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      buf.Reserve(5);
      buf.InsertResource(Score{});

      CHECK_EQ(buf.Size(), 1);
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::InsertResource") {
    SUBCASE("Inserts resource into world after execution") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.InsertResource(Score{42});
      }

      queue.ExecuteAll(world);

      CHECK_EQ(world.ReadResource<Score>().value, 42);
    }

    SUBCASE("Replaces existing resource") {
      World world;
      world.InsertResources(Score{1});
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.InsertResource(Score{99});
      }

      queue.ExecuteAll(world);

      CHECK_EQ(world.ReadResource<Score>().value, 99);
    }

    SUBCASE("Returns self for chaining") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      auto& ref = buf.InsertResource(Score{});

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::TryInsertResource") {
    SUBCASE("Inserts resource when absent") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.TryInsertResource(Score{10});
      }

      queue.ExecuteAll(world);

      CHECK_EQ(world.ReadResource<Score>().value, 10);
    }

    SUBCASE("Does not overwrite existing resource") {
      World world;
      world.InsertResources(Score{5});
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.TryInsertResource(Score{99});
      }

      queue.ExecuteAll(world);

      CHECK_EQ(world.ReadResource<Score>().value, 5);
    }

    SUBCASE("Returns self for chaining") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      auto& ref = buf.TryInsertResource(Score{});

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::RemoveResource") {
    SUBCASE("Removes existing resource after execution") {
      World world;
      world.InsertResources(Score{1});
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.RemoveResource<Score>();
      }

      queue.ExecuteAll(world);

      CHECK_FALSE(world.HasResource<Score>());
    }

    SUBCASE("Returns self for chaining") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      auto& ref = buf.RemoveResource<Score>();

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::TryRemoveResource") {
    SUBCASE("Removes resource when present") {
      World world;
      world.InsertResources(Score{1});
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.TryRemoveResource<Score>();
      }

      queue.ExecuteAll(world);

      CHECK_FALSE(world.HasResource<Score>());
    }

    SUBCASE("Does not assert when resource is absent") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.TryRemoveResource<Score>();
      }

      queue.ExecuteAll(world);

      CHECK_FALSE(world.HasResource<Score>());
    }

    SUBCASE("Returns self for chaining") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      auto& ref = buf.TryRemoveResource<Score>();

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::DeferredUpdate") {
    SUBCASE("Executes lambda with world reference after flush") {
      World world;
      bool called = false;
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.DeferredUpdate([&called](World& /*world*/) { called = true; });
      }

      queue.ExecuteAll(world);

      CHECK(called);
    }

    SUBCASE("Receives correct world reference") {
      World world;
      World* received = nullptr;
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.DeferredUpdate(
            [&received](World& captured) { received = &captured; });
      }

      queue.ExecuteAll(world);

      CHECK_EQ(received, &world);
    }

    SUBCASE("Returns self for chaining") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      auto& ref = buf.DeferredUpdate([](World& /*world*/) {});

      CHECK_EQ(&ref, &buf);
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::Empty") {
    SUBCASE("Buffer is empty initially") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      CHECK(buf.Empty());
    }

    SUBCASE("Buffer not empty after enqueue") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
      buf.InsertResource(Score{});

      CHECK_FALSE(buf.Empty());
    }

    SUBCASE("Buffer empty after clear") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
      buf.InsertResource(Score{});
      buf.Clear();

      CHECK(buf.Empty());
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::Size") {
    SUBCASE("Size is zero initially") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      CHECK_EQ(buf.Size(), 0);
    }

    SUBCASE("Size increases after each enqueue") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());

      buf.InsertResource(Score{});
      CHECK_EQ(buf.Size(), 1);

      buf.InsertResource(Timer{});
      CHECK_EQ(buf.Size(), 2);
    }

    SUBCASE("Size is zero after clear") {
      PmrCmdQueue queue(std::pmr::get_default_resource());
      WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
      buf.InsertResource(Score{});
      buf.InsertResource(Timer{});
      buf.Clear();

      CHECK_EQ(buf.Size(), 0);
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer::GetAllocator") {
    SUBCASE("Returns allocator used at construction") {
      auto* resource = std::pmr::get_default_resource();
      PmrCmdQueue queue(resource);
      PmrWorldCmdBuffer buf(queue, resource);

      const auto alloc = buf.GetAllocator();

      CHECK_EQ(alloc.resource(), resource);
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer: destructor flushes commands into queue") {
    SUBCASE("Commands are in queue after buffer is destroyed") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.InsertResource(Score{7});
        buf.InsertResource(Timer{});

        CHECK_EQ(queue.Size(), 0);
      }  // destructor merges buf into queue

      CHECK_EQ(queue.Size(), 2);
      queue.ExecuteAll(world);

      CHECK(world.HasResource<Score>());
      CHECK(world.HasResource<Timer>());
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer: commands execute in enqueue order") {
    SUBCASE(
        "InsertResource followed by RemoveResource results in absent "
        "resource") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.InsertResource(Score{1});
        buf.RemoveResource<Score>();
      }

      queue.ExecuteAll(world);

      CHECK_FALSE(world.HasResource<Score>());
    }
  }

  TEST_CASE("ecs::WorldCmdBuffer: method chaining") {
    SUBCASE("Multiple operations can be chained") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        WorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.InsertResource(Score{1})
            .InsertResource(Timer{2.0F})
            .TryInsertResource(Config{true});
      }

      queue.ExecuteAll(world);

      CHECK(world.HasResource<Score>());
      CHECK(world.HasResource<Timer>());
      CHECK(world.HasResource<Config>());
    }
  }

  TEST_CASE("ecs::PmrWorldCmdBuffer: alias works correctly") {
    SUBCASE("PmrWorldCmdBuffer inserts resource") {
      World world;
      PmrCmdQueue queue(std::pmr::get_default_resource());

      {
        PmrWorldCmdBuffer buf(queue, std::pmr::get_default_resource());
        buf.InsertResource(Score{55});
      }

      queue.ExecuteAll(world);

      CHECK_EQ(world.ReadResource<Score>().value, 55);
    }
  }
}
