#include <doctest/doctest.h>

#include <helios/ecs/entity/manager.hpp>

#include <algorithm>
#include <array>
#include <barrier>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory_resource>
#include <random>
#include <thread>
#include <unordered_set>
#include <vector>

using namespace helios::ecs;

namespace {

class CountingResource final : public std::pmr::memory_resource {
public:
  size_t bytes_allocated = 0;

protected:
  auto do_allocate(size_t bytes, size_t alignment) -> void* override {
    bytes_allocated += bytes;
    return std::pmr::new_delete_resource()->allocate(bytes, alignment);
  }

  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override {
    std::pmr::new_delete_resource()->deallocate(ptr, bytes, alignment);
  }

  [[nodiscard]] auto do_is_equal(
      const std::pmr::memory_resource& other) const noexcept -> bool override {
    return this == &other;
  }
};

}  // namespace

TEST_SUITE("helios::ecs::EntityManager") {
  TEST_CASE("helios::ecs::EntityManager::ctor") {
    SUBCASE("Default ctor") {
      const EntityManager manager;
      CHECK_EQ(manager.Count(), 0);
      CHECK_EQ(manager.GetMemoryResource(), std::pmr::get_default_resource());
    }

    SUBCASE("Memory resource ctor") {
      CountingResource resource;
      EntityManager manager{&resource};

      CHECK_EQ(manager.GetMemoryResource(), &resource);

      std::vector<Entity> entities;
      entities.reserve(256);
      manager.Create(256, std::back_inserter(entities));

      CHECK_EQ(manager.Count(), 256);
      CHECK_GT(resource.bytes_allocated, 0);
    }

    SUBCASE("Nullptr ctor is deleted") {
      CHECK_FALSE(std::constructible_from<EntityManager, std::nullptr_t>);
    }

    SUBCASE("Copy ctor") {
      EntityManager original;
      [[maybe_unused]] auto e1 = original.Create();
      [[maybe_unused]] auto e2 = original.Create();

      const EntityManager copy(original);

      CHECK_EQ(copy.Count(), original.Count());
      CHECK_EQ(copy.GetMemoryResource(), original.GetMemoryResource());
    }

    SUBCASE("Copy ctor preserves source memory resource") {
      CountingResource resource;
      EntityManager original{&resource};
      const auto entity = original.Create();

      const EntityManager copy(original);

      CHECK_EQ(copy.GetMemoryResource(), &resource);
      CHECK(copy.Validate(entity));
    }

    SUBCASE("Move ctor") {
      EntityManager original;
      const auto entity1 = original.Create();
      const auto entity2 = original.Create();
      const auto original_count = original.Count();

      const EntityManager moved(std::move(original));

      CHECK_EQ(moved.Count(), original_count);
      CHECK(moved.Validate(entity1));
      CHECK(moved.Validate(entity2));
    }
  }

  TEST_CASE("helios::ecs::EntityManager::assignment") {
    SUBCASE("Copy assignment") {
      EntityManager original;
      [[maybe_unused]] auto e1 = original.Create();
      [[maybe_unused]] auto e2 = original.Create();

      EntityManager assigned;
      assigned = original;

      CHECK_EQ(assigned.Count(), original.Count());
    }

    SUBCASE("Move assignment") {
      EntityManager original;
      const auto entity = original.Create();
      const auto original_count = original.Count();

      EntityManager assigned;
      assigned = std::move(original);

      CHECK_EQ(assigned.Count(), original_count);
      CHECK(assigned.Validate(entity));
    }

    SUBCASE("Self assignment") {
      EntityManager manager;
      [[maybe_unused]] auto _ = manager.Create();

      manager = manager;

      CHECK_EQ(manager.Count(), 1);
    }
  }

  TEST_CASE("helios::ecs::EntityManager::Clear") {
    SUBCASE("Clear with no entities") {
      EntityManager manager;
      manager.Clear();
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Clear with entities") {
      EntityManager manager;
      const auto e1 = manager.Create();
      const auto e2 = manager.Create();
      const auto e3 = manager.Create();

      manager.Clear();

      CHECK_EQ(manager.Count(), 0);
      CHECK_FALSE(manager.Validate(e1));
      CHECK_FALSE(manager.Validate(e2));
      CHECK_FALSE(manager.Validate(e3));
    }

    SUBCASE("Clear and recreate") {
      EntityManager manager;
      [[maybe_unused]] auto e1 = manager.Create();
      [[maybe_unused]] auto e2 = manager.Create();

      manager.Clear();
      const auto entity = manager.Create();

      CHECK_EQ(manager.Count(), 1);
      CHECK(manager.Validate(entity));
    }
  }

  TEST_CASE("helios::ecs::EntityManager::Flush") {
    SUBCASE("Flush with no reservations is a no-op") {
      EntityManager manager;
      manager.Flush();
      CHECK_EQ(manager.Count(), 0);
      CHECK_FALSE(manager.NeedsFlush());
    }

    SUBCASE("Flush materializes fresh reservation") {
      EntityManager manager;
      const auto reserved = manager.ReserveEntity();

      Entity flushed{};
      manager.Flush([&](Entity entity) { flushed = entity; });

      CHECK_EQ(flushed, reserved);
      CHECK(manager.Validate(reserved));
      CHECK_EQ(manager.Count(), 1);
      CHECK_FALSE(manager.NeedsFlush());
    }

    SUBCASE("Flush materializes recycled reservation with matching handle") {
      EntityManager manager;
      const auto original = manager.Create();
      manager.Destroy(original);

      const auto reserved = manager.ReserveEntity();
      CHECK_FALSE(manager.Validate(reserved));

      Entity flushed{};
      manager.Flush([&](Entity entity) { flushed = entity; });

      CHECK_EQ(flushed, reserved);
      CHECK(manager.Validate(reserved));
      CHECK_FALSE(manager.Validate(original));
      CHECK_EQ(manager.Count(), 1);
    }
  }

  TEST_CASE("helios::ecs::EntityManager::Reserve") {
    SUBCASE("Reserve capacity") {
      EntityManager manager;
      manager.Reserve(100);
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Reserve and create") {
      EntityManager manager;
      manager.Reserve(100);

      for (int i = 0; i < 100; ++i) {
        [[maybe_unused]] auto _ = manager.Create();
      }

      CHECK_EQ(manager.Count(), 100);
    }
  }

  TEST_CASE("helios::ecs::EntityManager::ReserveEntity") {
    SUBCASE("Reserve single entity") {
      EntityManager manager;
      const auto reserved = manager.ReserveEntity();
      CHECK(reserved.Valid());
      CHECK_EQ(reserved.Generation(), Entity::kInitialAliveGeneration);
      CHECK(reserved.Alive());
    }

    SUBCASE("Reserve multiple entities") {
      EntityManager manager;

      const auto r1 = manager.ReserveEntity();
      const auto r2 = manager.ReserveEntity();
      const auto r3 = manager.ReserveEntity();

      CHECK(r1.Valid());
      CHECK(r2.Valid());
      CHECK(r3.Valid());

      CHECK_NE(r1, r2);
      CHECK_NE(r2, r3);
    }

    SUBCASE("Fresh reservation is invalid before Flush") {
      EntityManager manager;
      const auto reserved = manager.ReserveEntity();
      CHECK_FALSE(manager.Validate(reserved));
      manager.Flush();
      CHECK(manager.Validate(reserved));
    }

    SUBCASE("Recycled reservation is invalid before Flush") {
      EntityManager manager;

      const auto original = manager.Create();
      const auto original_gen = original.Generation();
      manager.Destroy(original);

      const auto reserved = manager.ReserveEntity();
      CHECK_EQ(reserved.Index(), original.Index());
      CHECK_EQ(reserved.Generation(),
               NextGeneration(NextGeneration(original_gen, /*alive=*/false),
                              /*alive=*/true));
      CHECK(manager.NeedsFlush());
      CHECK_FALSE(manager.Validate(reserved));
      CHECK_FALSE(manager.Validate(original));

      const auto stored_free = manager.GetGeneration(reserved.Index());
      CHECK_FALSE(IsAliveGeneration(stored_free));
      CHECK_EQ(reserved.Generation(),
               NextGeneration(stored_free, /*alive=*/true));

      manager.Flush();
      CHECK(manager.Validate(reserved));
      CHECK_FALSE(manager.Validate(original));
      CHECK_FALSE(manager.NeedsFlush());
    }

    SUBCASE("Concurrent reservations during stable phase are unique") {
      EntityManager manager;
      manager.Reserve(256);

      std::vector<Entity> recycled_seed;
      recycled_seed.reserve(64);
      manager.Create(64, std::back_inserter(recycled_seed));
      manager.Destroy(recycled_seed);

      constexpr size_t kThreads = 8;
      constexpr size_t kPerThread = 16;
      std::barrier start(static_cast<std::ptrdiff_t>(kThreads));
      std::vector<std::vector<Entity>> per_thread(kThreads);
      std::vector<std::thread> threads;
      threads.reserve(kThreads);

      for (size_t t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
          per_thread[t].reserve(kPerThread);
          start.arrive_and_wait();
          for (size_t i = 0; i < kPerThread; ++i) {
            const Entity reserved = manager.ReserveEntity();
            CHECK_FALSE(manager.Validate(reserved));
            per_thread[t].push_back(reserved);
          }
        });
      }

      for (auto& thread : threads) {
        thread.join();
      }

      std::unordered_set<Entity::IndexType> indices;
      std::vector<Entity> all;
      all.reserve(kThreads * kPerThread);
      for (const auto& batch : per_thread) {
        for (const Entity entity : batch) {
          CHECK(indices.insert(entity.Index()).second);
          all.push_back(entity);
        }
      }
      CHECK_EQ(indices.size(), kThreads * kPerThread);

      manager.Flush();
      CHECK_EQ(manager.Count(), kThreads * kPerThread);
      for (const Entity entity : all) {
        CHECK(manager.Validate(entity));
      }
    }
  }

  TEST_CASE("helios::ecs::EntityManager::Create") {
    SUBCASE("Create single entity") {
      EntityManager manager;

      const auto entity = manager.Create();

      CHECK(entity.Valid());
      CHECK_EQ(entity.Generation(), Entity::kInitialAliveGeneration);
      CHECK_EQ(manager.Count(), 1);
      CHECK(manager.Validate(entity));
    }

    SUBCASE("Create multiple entities") {
      EntityManager manager;

      const auto entity1 = manager.Create();
      const auto entity2 = manager.Create();
      const auto entity3 = manager.Create();

      CHECK(entity1.Valid());
      CHECK(entity2.Valid());
      CHECK(entity3.Valid());

      CHECK_NE(entity1, entity2);
      CHECK_NE(entity2, entity3);
      CHECK_NE(entity1, entity3);

      CHECK_EQ(manager.Count(), 3);
    }

    SUBCASE("Created entities have unique indices") {
      EntityManager manager;

      std::vector<Entity> entities(100);
      for (auto& entity : entities) {
        entity = manager.Create();
      }

      std::vector<Entity::IndexType> indices;
      indices.reserve(100);
      for (const auto& entity : entities) {
        indices.push_back(entity.Index());
      }

      std::ranges::sort(indices);
      const auto last = std::ranges::unique(indices);
      CHECK_EQ(last.begin(), indices.end());
    }

    SUBCASE("Immediate reuse performs free to alive transition") {
      EntityManager manager;
      const auto original = manager.Create();
      const auto original_gen = original.Generation();
      manager.Destroy(original);

      const auto reused = manager.Create();
      CHECK_EQ(reused.Index(), original.Index());
      CHECK_EQ(reused.Generation(),
               NextGeneration(NextGeneration(original_gen, /*alive=*/false),
                              /*alive=*/true));
      CHECK(manager.Validate(reused));
      CHECK_FALSE(manager.Validate(original));
    }
  }

  TEST_CASE("helios::ecs::EntityManager::Create/batch") {
    SUBCASE("Create multiple entities with back_inserter") {
      EntityManager manager;
      std::vector<Entity> entities;

      manager.Create(10, std::back_inserter(entities));

      CHECK_EQ(entities.size(), 10);
      CHECK_EQ(manager.Count(), 10);

      const bool all_valid =
          std::ranges::all_of(entities, [&manager](const Entity& entity) {
            return entity.Valid() && manager.Validate(entity);
          });
      CHECK(all_valid);
    }

    SUBCASE("Create entities into array") {
      EntityManager manager;
      std::array<Entity, 5> entities = {};

      manager.Create(5, entities.begin());

      CHECK_EQ(manager.Count(), 5);

      const bool all_valid =
          std::ranges::all_of(entities, [&manager](const Entity& entity) {
            return entity.Valid() && manager.Validate(entity);
          });
      CHECK(all_valid);
    }

    SUBCASE("Create zero entities") {
      EntityManager manager;
      std::vector<Entity> entities;

      manager.Create(0, std::back_inserter(entities));

      CHECK(entities.empty());
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Create large batch of entities") {
      EntityManager manager;
      std::vector<Entity> entities;

      manager.Create(1000, std::back_inserter(entities));

      CHECK_EQ(entities.size(), 1000);
      CHECK_EQ(manager.Count(), 1000);

      const bool all_valid =
          std::ranges::all_of(entities, [&manager](const Entity& entity) {
            return entity.Valid() && manager.Validate(entity);
          });
      CHECK(all_valid);
    }

    SUBCASE("Batch create reuses free list with alive generations") {
      EntityManager manager;
      std::vector<Entity> originals;
      originals.reserve(4);
      manager.Create(4, std::back_inserter(originals));
      manager.Destroy(originals);

      std::vector<Entity> reused;
      reused.reserve(4);
      manager.Create(4, std::back_inserter(reused));

      CHECK_EQ(reused.size(), 4);
      CHECK_EQ(manager.Count(), 4);
      for (size_t i = 0; i < reused.size(); ++i) {
        CHECK(manager.Validate(reused[i]));
        CHECK_FALSE(manager.Validate(originals[i]));
        CHECK(IsAliveGeneration(reused[i].Generation()));
      }
    }
  }

  TEST_CASE("helios::ecs::EntityManager::Destroy") {
    SUBCASE("Destroy single entity") {
      EntityManager manager;
      const auto entity = manager.Create();

      manager.Destroy(entity);

      CHECK_FALSE(manager.Validate(entity));
      CHECK_EQ(manager.Count(), 0);
      CHECK_FALSE(IsAliveGeneration(manager.GetGeneration(entity.Index())));
    }

    SUBCASE("Destroy multiple entities") {
      EntityManager manager;
      const auto entity1 = manager.Create();
      const auto entity2 = manager.Create();
      const auto entity3 = manager.Create();

      manager.Destroy(entity1);
      manager.Destroy(entity2);

      CHECK_FALSE(manager.Validate(entity1));
      CHECK_FALSE(manager.Validate(entity2));
      CHECK(manager.Validate(entity3));
      CHECK_EQ(manager.Count(), 1);
    }

    SUBCASE("Destroy all entities") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(10);
      manager.Create(10, std::back_inserter(entities));

      for (const auto& entity : entities) {
        manager.Destroy(entity);
      }

      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Destroy and recreate entity") {
      EntityManager manager;

      const auto entity1 = manager.Create();
      manager.Destroy(entity1);
      const auto entity2 = manager.Create();

      CHECK(entity2.Valid());
      CHECK_NE(entity1, entity2);
      CHECK_EQ(entity1.Index(), entity2.Index());
      CHECK_NE(entity1.Generation(), entity2.Generation());
    }

    SUBCASE("Destroyed entity is not alive") {
      EntityManager manager;
      const auto entity = manager.Create();

      manager.Destroy(entity);

      CHECK_FALSE(manager.Validate(entity));
    }

    SUBCASE("Destroy range of entities") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(10);
      manager.Create(10, std::back_inserter(entities));

      manager.Destroy(entities);

      CHECK_EQ(manager.Count(), 0);

      for (const auto& entity : entities) {
        CHECK_FALSE(manager.Validate(entity));
      }
    }

    SUBCASE("Destroy partial range") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(10);
      manager.Create(10, std::back_inserter(entities));

      std::vector<Entity> to_destroy(entities.begin(), entities.begin() + 5);
      manager.Destroy(to_destroy);

      CHECK_EQ(manager.Count(), 5);

      const bool not_valid = std::all_of(entities.begin(), entities.begin() + 5,
                                         [&manager](const Entity& entity) {
                                           return !manager.Validate(entity);
                                         });
      CHECK(not_valid);
      const bool valid = std::all_of(entities.begin() + 5, entities.end(),
                                     [&manager](const Entity& entity) {
                                       return manager.Validate(entity);
                                     });
      CHECK(valid);
    }
  }

  TEST_CASE("helios::ecs::EntityManager::Validate") {
    SUBCASE("Validate created entity") {
      EntityManager manager;
      const auto entity = manager.Create();
      CHECK(manager.Validate(entity));
    }

    SUBCASE("Validate destroyed entity") {
      EntityManager manager;

      const auto entity = manager.Create();
      manager.Destroy(entity);

      CHECK_FALSE(manager.Validate(entity));
    }

    SUBCASE("Validate invalid entity") {
      const EntityManager manager;
      const Entity invalid;
      CHECK_FALSE(manager.Validate(invalid));
    }

    SUBCASE("Validate entity from different manager") {
      EntityManager manager1;
      const EntityManager manager2;

      const auto entity = manager1.Create();

      CHECK(manager1.Validate(entity));
      CHECK_FALSE(manager2.Validate(entity));
    }

    SUBCASE("Validate rejects pending recycled reservation") {
      EntityManager manager;
      const auto original = manager.Create();
      manager.Destroy(original);
      const auto reserved = manager.ReserveEntity();

      CHECK_FALSE(manager.Validate(reserved));
      manager.Flush();
      CHECK(manager.Validate(reserved));
    }
  }

  TEST_CASE("helios::ecs::EntityManager::NeedsFlush") {
    SUBCASE("NeedsFlush is false initially") {
      const EntityManager manager;
      CHECK_FALSE(manager.NeedsFlush());
    }

    SUBCASE("NeedsFlush is true after ReserveEntity") {
      EntityManager manager;
      [[maybe_unused]] const auto reserved = manager.ReserveEntity();
      CHECK(manager.NeedsFlush());
    }

    SUBCASE("NeedsFlush is false after Flush") {
      EntityManager manager;
      [[maybe_unused]] const auto reserved = manager.ReserveEntity();
      manager.Flush();
      CHECK_FALSE(manager.NeedsFlush());
    }

    SUBCASE("NeedsFlush is false after Create and Destroy") {
      EntityManager manager;
      const auto entity = manager.Create();
      CHECK_FALSE(manager.NeedsFlush());
      manager.Destroy(entity);
      CHECK_FALSE(manager.NeedsFlush());
    }
  }

  TEST_CASE("helios::ecs::EntityManager::Count") {
    SUBCASE("Count is zero initially") {
      const EntityManager manager;
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Count increases with creation") {
      EntityManager manager;

      [[maybe_unused]] auto e1 = manager.Create();
      CHECK_EQ(manager.Count(), 1);

      [[maybe_unused]] auto e2 = manager.Create();
      CHECK_EQ(manager.Count(), 2);

      [[maybe_unused]] auto e3 = manager.Create();
      CHECK_EQ(manager.Count(), 3);
    }

    SUBCASE("Count decreases with destruction") {
      EntityManager manager;
      const auto e1 = manager.Create();
      const auto e2 = manager.Create();
      const auto e3 = manager.Create();

      CHECK_EQ(manager.Count(), 3);

      manager.Destroy(e1);
      CHECK_EQ(manager.Count(), 2);

      manager.Destroy(e2);
      CHECK_EQ(manager.Count(), 1);

      manager.Destroy(e3);
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Count with batch operations") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(10);

      manager.Create(10, std::back_inserter(entities));
      CHECK_EQ(manager.Count(), 10);

      manager.Destroy(entities);
      CHECK_EQ(manager.Count(), 0);
    }
  }

  TEST_CASE("helios::ecs::EntityManager::GetGeneration") {
    SUBCASE("Out of range index returns invalid generation") {
      const EntityManager manager;
      CHECK_EQ(manager.GetGeneration(0), Entity::kInvalidGeneration);
    }

    SUBCASE("Fresh creation stores initial alive generation") {
      EntityManager manager;
      const auto entity = manager.Create();
      CHECK_EQ(manager.GetGeneration(entity.Index()),
               Entity::kInitialAliveGeneration);
      CHECK_EQ(manager.GetGeneration(entity.Index()), entity.Generation());
    }

    SUBCASE("Destroy stores free generation encoding") {
      EntityManager manager;
      const auto entity = manager.Create();
      const auto expected_free =
          NextGeneration(entity.Generation(), /*alive=*/false);
      manager.Destroy(entity);

      const auto stored = manager.GetGeneration(entity.Index());
      CHECK_EQ(stored, expected_free);
      CHECK_FALSE(IsAliveGeneration(stored));
    }

    SUBCASE("Recycled reservation leaves free generation until Flush") {
      EntityManager manager;
      const auto original = manager.Create();
      manager.Destroy(original);
      const auto reserved = manager.ReserveEntity();

      CHECK_EQ(manager.GetGeneration(reserved.Index()),
               NextGeneration(original.Generation(), /*alive=*/false));
      CHECK_NE(manager.GetGeneration(reserved.Index()), reserved.Generation());

      manager.Flush();
      CHECK_EQ(manager.GetGeneration(reserved.Index()), reserved.Generation());
    }
  }

  TEST_CASE("helios::ecs::EntityManager::generation") {
    SUBCASE("Entity generation advances after destroy and recreate") {
      EntityManager manager;

      const auto entity1 = manager.Create();
      const auto gen1 = entity1.Generation();

      manager.Destroy(entity1);

      const auto entity2 = manager.Create();
      const auto gen2 = entity2.Generation();

      CHECK_EQ(entity1.Index(), entity2.Index());
      CHECK_EQ(gen2, NextGeneration(NextGeneration(gen1, /*alive=*/false),
                                    /*alive=*/true));
      CHECK_GT(gen2 & Entity::kCounterMask, gen1 & Entity::kCounterMask);
    }

    SUBCASE("Different entities have independent generations") {
      EntityManager manager;

      const auto e1 = manager.Create();
      const auto e2 = manager.Create();
      const auto e3 = manager.Create();

      manager.Destroy(e2);

      const auto e4 = manager.Create();

      CHECK_EQ(e4.Index(), e2.Index());
      CHECK_EQ(e4.Generation(),
               NextGeneration(NextGeneration(e2.Generation(), /*alive=*/false),
                              /*alive=*/true));
      CHECK(manager.Validate(e1));
      CHECK(manager.Validate(e3));
    }
  }

  TEST_CASE("helios::ecs::EntityManager::edge_cases") {
    SUBCASE("Create and destroy many entities") {
      EntityManager manager;
      std::vector<Entity> entities;
      entities.reserve(100);
      manager.Create(100, std::back_inserter(entities));

      CHECK_EQ(manager.Count(), 100);

      for (const auto& entity : entities) {
        manager.Destroy(entity);
      }

      CHECK_EQ(manager.Count(), 0);
      const bool all_invalid =
          std::ranges::all_of(entities, [&manager](const Entity& entity) {
            return !manager.Validate(entity);
          });
      CHECK(all_invalid);
    }

    SUBCASE("Interleaved create and destroy") {
      EntityManager manager;

      const auto e1 = manager.Create();
      const auto e2 = manager.Create();
      manager.Destroy(e1);
      const auto e3 = manager.Create();
      manager.Destroy(e2);
      const auto e4 = manager.Create();

      CHECK_EQ(manager.Count(), 2);
      CHECK_FALSE(manager.Validate(e1));
      CHECK_FALSE(manager.Validate(e2));
      CHECK(manager.Validate(e3));
      CHECK(manager.Validate(e4));
    }

    SUBCASE("Reuse of entity indices") {
      EntityManager manager;

      const auto e1 = manager.Create();
      const auto index1 = e1.Index();
      manager.Destroy(e1);

      const auto e2 = manager.Create();
      const auto index2 = e2.Index();

      CHECK_EQ(index1, index2);
      CHECK_NE(e1.Generation(), e2.Generation());
    }

    SUBCASE("Copy preserves all entities") {
      EntityManager original;
      std::vector<Entity> entities;
      entities.reserve(10);
      original.Create(10, std::back_inserter(entities));

      EntityManager copy(original);

      CHECK_EQ(copy.Count(), original.Count());
      const bool all_valid = std::ranges::all_of(
          entities,
          [&copy](const Entity& entity) { return copy.Validate(entity); });
      CHECK(all_valid);
    }

    SUBCASE("Deterministic interleaved Create Destroy Reserve Flush stress") {
      EntityManager manager;
      std::mt19937 rng(42);
      std::uniform_int_distribution<int> op_dist(0, 3);

      std::vector<Entity> live;
      live.reserve(256);
      std::vector<Entity> pending;
      pending.reserve(64);

      auto flush_pending = [&] {
        if (pending.empty()) {
          return;
        }
        manager.Flush();
        for (const Entity entity : pending) {
          CHECK(manager.Validate(entity));
          live.push_back(entity);
        }
        pending.clear();
      };

      for (int step = 0; step < 1000; ++step) {
        const int op = op_dist(rng);
        if (op == 0 || live.empty()) {
          flush_pending();
          live.push_back(manager.Create());
        } else if (op == 1) {
          flush_pending();
          const size_t index =
              static_cast<size_t>(rng() % static_cast<uint32_t>(live.size()));
          manager.Destroy(live[index]);
          live.erase(live.begin() + static_cast<std::ptrdiff_t>(index));
        } else if (op == 2) {
          pending.push_back(manager.ReserveEntity());
          CHECK_FALSE(manager.Validate(pending.back()));
        } else {
          flush_pending();
        }

        CHECK_EQ(manager.Count(), live.size());
        for (const Entity entity : live) {
          CHECK(manager.Validate(entity));
        }
      }

      flush_pending();
      CHECK_EQ(manager.Count(), live.size());
    }
  }
}
