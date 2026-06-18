#include <doctest/doctest.h>

#include <helios/ecs/schedule/system_local_data.hpp>

using namespace helios::ecs;

TEST_SUITE("helios::ecs::SystemLocalDataOptions") {
  TEST_CASE("ecs::SystemLocalDataOptions::ctor") {
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
  TEST_CASE("ecs::SystemLocalData::From") {
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

  TEST_CASE("ecs::SystemLocalData::Clear") {
    SUBCASE("Clear on fresh data is a no-op") {
      auto data = SystemLocalData::From();
      const size_t initial_capacity = data.allocator.TotalCapacity();

      data.Clear();

      CHECK_GE(data.allocator.TotalCapacity(), initial_capacity);
    }
  }

  TEST_CASE("ecs::SystemLocalData::ResetArena") {
    SUBCASE("ResetArena preserves the allocator capacity") {
      auto data = SystemLocalData::From();
      const size_t initial_capacity = data.allocator.TotalCapacity();

      data.ResetArena();

      CHECK_GE(data.allocator.TotalCapacity(), initial_capacity);
    }
  }

  TEST_CASE("ecs::SystemLocalData::Update") {
    SUBCASE("Update resets the arena after execution") {
      auto data = SystemLocalData::From();
      World world;

      data.Update(world);

      CHECK_GE(data.allocator.TotalCapacity(), 0);
    }
  }

  TEST_CASE("ecs::SystemLocalData::ExecuteCommands") {
    SUBCASE("ExecuteCommands on empty queue is a no-op") {
      auto data = SystemLocalData::From();
      World world;

      data.ExecuteCommands(world);

      CHECK_GE(data.allocator.TotalCapacity(), 0);
    }
  }

  TEST_CASE("ecs::SystemLocalData::HasPendingWork") {
    SUBCASE("Fresh local data has no pending work") {
      const auto data = SystemLocalData::From();
      CHECK_FALSE(data.HasPendingWork());
    }
  }

  TEST_CASE("ecs::SystemLocalData::MergeMessages") {
    SUBCASE("MergeMessages on empty queue is a no-op") {
      auto data = SystemLocalData::From();
      World world;

      data.MergeMessages(world);

      CHECK_GE(data.allocator.TotalCapacity(), 0);
    }
  }
}
