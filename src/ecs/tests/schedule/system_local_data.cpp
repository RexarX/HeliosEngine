#include <doctest/doctest.h>

#include <helios/ecs/schedule/local_arena.hpp>
#include <helios/ecs/schedule/system_local_data.hpp>
#include <helios/ecs/world.hpp>

#include <cstddef>
#include <memory_resource>
#include <string_view>

using namespace helios::ecs;

namespace {

struct CountCommand {
  int* counter = nullptr;

  void Execute(World& /*world*/) const {
    if (counter != nullptr) {
      ++(*counter);
    }
  }
};

struct ProbeMsg {
  static constexpr std::string_view kName = "ProbeMsg";

  int value = 0;
};

}  // namespace

TEST_SUITE("helios::ecs::SystemLocalDataOptions") {
  TEST_CASE("helios::ecs::SystemLocalDataOptions::ctor") {
    SUBCASE("Default-constructed options have preallocated size of 1024") {
      constexpr SystemLocalDataOptions options;
      CHECK_EQ(options.preallocated_size,
               SystemLocalDataOptions::kDefaultPreallocatedSize);
    }

    SUBCASE("Custom preallocated size can be set") {
      constexpr SystemLocalDataOptions options{.preallocated_size = 2048};
      CHECK_EQ(options.preallocated_size, 2048);
    }
  }
}

TEST_SUITE("helios::ecs::SystemLocalData") {
  TEST_CASE("helios::ecs::SystemLocalData::From") {
    SUBCASE("From with default options creates valid data") {
      const auto data = SystemLocalData::From();

      CHECK_GT(data.allocator.TotalCapacity(), 0);
    }

    SUBCASE("From with custom options uses custom size") {
      constexpr SystemLocalDataOptions options{.preallocated_size = 4096};
      const auto data = SystemLocalData::From(options);

      CHECK_GE(data.allocator.TotalCapacity(), 4096);
    }
  }

  TEST_CASE("helios::ecs::SystemLocalData::Clear") {
    SUBCASE("Clear on fresh data is a no-op") {
      auto data = SystemLocalData::From();
      const size_t initial_capacity = data.allocator.TotalCapacity();

      data.Clear();

      CHECK_GE(data.allocator.TotalCapacity(), initial_capacity);
    }
  }

  TEST_CASE("helios::ecs::SystemLocalData::ResetArena") {
    SUBCASE("ResetArena preserves the allocator capacity") {
      auto data = SystemLocalData::From();
      const size_t initial_capacity = data.allocator.TotalCapacity();

      data.ResetArena();

      CHECK_GE(data.allocator.TotalCapacity(), initial_capacity);
    }
  }

  TEST_CASE("helios::ecs::SystemLocalData::Apply") {
    SUBCASE("Apply commands only leaves messages pending") {
      auto data = SystemLocalData::From();
      World world;
      world.AddMessages<ProbeMsg>();
      data.message_queue.Register<ProbeMsg>();
      int count = 0;

      data.cmd_queue.Enqueue(CountCommand{&count});
      data.message_queue.Enqueue(ProbeMsg{.value = 3});
      CHECK(data.HasPendingWork());

      data.Apply(world, true, false);

      CHECK_EQ(count, 1);
      CHECK(data.HasPendingWork());
      CHECK(world.Messages().CurrentMessages<ProbeMsg>().empty());
    }

    SUBCASE("Apply messages only leaves commands pending") {
      auto data = SystemLocalData::From();
      World world;
      world.AddMessages<ProbeMsg>();
      data.message_queue.Register<ProbeMsg>();
      int count = 0;

      data.cmd_queue.Enqueue(CountCommand{&count});
      data.message_queue.Enqueue(ProbeMsg{.value = 3});

      data.Apply(world, false, true);

      CHECK_EQ(count, 0);
      CHECK(data.HasPendingWork());
      CHECK_EQ(world.Messages().CurrentMessages<ProbeMsg>().size(), 1);
      CHECK_EQ(world.Messages().CurrentMessages<ProbeMsg>()[0].value, 3);
    }

    SUBCASE("Apply both clears pending work and resets arena") {
      auto data = SystemLocalData::From();
      World world;
      world.AddMessages<ProbeMsg>();
      data.message_queue.Register<ProbeMsg>();
      int count = 0;

      data.cmd_queue.Enqueue(CountCommand{&count});
      data.message_queue.Enqueue(ProbeMsg{.value = 3});

      data.Apply(world, true, true);

      CHECK_EQ(count, 1);
      CHECK_FALSE(data.HasPendingWork());
      CHECK_EQ(world.Messages().CurrentMessages<ProbeMsg>().size(), 1);
    }
  }

  TEST_CASE("helios::ecs::SystemLocalData::Update") {
    SUBCASE("Update resets the arena after execution") {
      auto data = SystemLocalData::From();
      World world;

      data.Update(world);

      CHECK_GE(data.allocator.TotalCapacity(), 0);
    }

    SUBCASE("Update resets the arena after executing enqueued commands") {
      auto data = SystemLocalData::From();
      World world;
      int count = 0;

      data.cmd_queue.Enqueue(CountCommand{&count});
      CHECK(data.HasPendingWork());

      data.Update(world);

      CHECK_EQ(count, 1);
      CHECK_FALSE(data.HasPendingWork());
      CHECK(data.resource_manager.Has<LocalArena>());
    }
  }

  TEST_CASE("helios::ecs::SystemLocalData::ExecuteCommands") {
    SUBCASE("ExecuteCommands on empty queue is a no-op") {
      auto data = SystemLocalData::From();
      World world;

      data.ExecuteCommands(world);

      CHECK_GE(data.allocator.TotalCapacity(), 0);
    }
  }

  TEST_CASE("helios::ecs::SystemLocalData::HasPendingWork") {
    SUBCASE("Fresh local data has no pending work") {
      const auto data = SystemLocalData::From();
      CHECK_FALSE(data.HasPendingWork());
    }
  }

  TEST_CASE("helios::ecs::SystemLocalData::MergeMessages") {
    SUBCASE("MergeMessages on empty queue is a no-op") {
      auto data = SystemLocalData::From();
      World world;

      data.MergeMessages(world);

      CHECK_GE(data.allocator.TotalCapacity(), 0);
    }

    SUBCASE("MergeMessages does not advance world message lifecycle") {
      World world;
      world.AddMessages<ProbeMsg>();
      world.WriteMessages<ProbeMsg>().Write({.value = 7});

      auto data = SystemLocalData::From();
      data.MergeMessages(world);

      CHECK_EQ(world.Messages().CurrentMessages<ProbeMsg>().size(), 1);
      CHECK(world.Messages().PreviousMessages<ProbeMsg>().empty());

      world.Update();
      CHECK(world.Messages().CurrentMessages<ProbeMsg>().empty());
      CHECK_EQ(world.Messages().PreviousMessages<ProbeMsg>().size(), 1);
    }
  }

  TEST_CASE("helios::ecs::SystemLocalData::LocalArena") {
    SUBCASE("From inserts LocalArena referencing the system allocator") {
      const auto data = SystemLocalData::From();

      CHECK(data.resource_manager.Has<LocalArena>());
      CHECK_EQ(data.resource_manager.Get<LocalArena>().GetPtr(),
               &data.allocator);
    }

    SUBCASE(
        "ResetArena preserves LocalArena referencing the system allocator") {
      auto data = SystemLocalData::From();
      std::pmr::polymorphic_allocator<std::byte> alloc(&data.allocator);
      [[maybe_unused]] void* scratch = alloc.allocate(128);

      data.ResetArena();

      CHECK(data.resource_manager.Has<LocalArena>());
      CHECK_EQ(data.resource_manager.Get<LocalArena>().GetPtr(),
               &data.allocator);
    }

    SUBCASE("Update preserves LocalArena after arena reset") {
      auto data = SystemLocalData::From();
      World world;
      std::pmr::polymorphic_allocator<std::byte> alloc(&data.allocator);
      [[maybe_unused]] void* scratch = alloc.allocate(128);

      data.Update(world);

      CHECK(data.resource_manager.Has<LocalArena>());
      CHECK_EQ(data.resource_manager.Get<LocalArena>().GetPtr(),
               &data.allocator);
    }

    SUBCASE(
        "Move construction rewires LocalArena to the destination allocator") {
      auto source = SystemLocalData::From();
      const size_t source_capacity = source.allocator.TotalCapacity();

      auto destination = SystemLocalData{std::move(source)};

      CHECK(destination.resource_manager.Has<LocalArena>());
      CHECK_EQ(destination.resource_manager.Get<LocalArena>().GetPtr(),
               &destination.allocator);
      CHECK_GE(destination.allocator.TotalCapacity(), source_capacity);
    }
  }
}
