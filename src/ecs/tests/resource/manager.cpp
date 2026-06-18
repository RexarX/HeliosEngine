#include <doctest/doctest.h>

#include <helios/ecs/resource/manager.hpp>

#include <string>

using namespace helios::ecs;

namespace {

struct Counter {
  int value = 0;
};

struct Config {
  std::string name;
  int version = 0;
};

struct Physics {
  float gravity = 9.81F;
};

struct Timer {
  double elapsed = 0.0;
};

// Tracks construction and destruction counts to verify resource lifetimes.
struct LifetimeTracker {
  inline static int constructions = 0;
  inline static int destructions = 0;

  constexpr LifetimeTracker() noexcept { ++constructions; }
  constexpr LifetimeTracker(const LifetimeTracker& /*other*/) noexcept {
    ++constructions;
  }
  constexpr LifetimeTracker(LifetimeTracker&& /*other*/) noexcept {
    ++constructions;
  }
  constexpr ~LifetimeTracker() noexcept { ++destructions; }

  constexpr LifetimeTracker& operator=(
      const LifetimeTracker& /*other*/) noexcept = default;
  constexpr LifetimeTracker& operator=(LifetimeTracker&& /*other*/) noexcept =
      default;

  static constexpr void Reset() noexcept {
    constructions = 0;
    destructions = 0;
  }
};

// Resource with explicit multi-arg constructor for Emplace tests.
struct Vec3 {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

}  // namespace

TEST_SUITE("helios::ecs::ResourceManager") {
  TEST_CASE("ecs::ResourceManager::ctor") {
    SUBCASE("Default-constructed manager is empty") {
      const ResourceManager manager;
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Copy ctor duplicates all resources") {
      ResourceManager original;
      original.Emplace<Counter>(42);
      original.Emplace<Physics>();

      const ResourceManager copy(original);

      CHECK_EQ(copy.Count(), 2);
      CHECK_EQ(copy.Get<Counter>().value, 42);
      CHECK(copy.Has<Physics>());
    }

    SUBCASE("Move ctor transfers all resources") {
      ResourceManager original;
      original.Emplace<Counter>(7);

      const ResourceManager moved(std::move(original));

      CHECK_EQ(moved.Count(), 1);
      CHECK_EQ(moved.Get<Counter>().value, 7);
    }
  }

  TEST_CASE("ecs::ResourceManager::operator=") {
    SUBCASE("Copy assignment replaces content with source resources") {
      ResourceManager source;
      source.Emplace<Counter>(99);

      ResourceManager target;
      target.Emplace<Physics>();
      target = source;

      CHECK_EQ(target.Get<Counter>().value, 99);
    }

    SUBCASE("Move assignment transfers resources and leaves source empty") {
      ResourceManager source;
      source.Emplace<Counter>(5);

      ResourceManager target;
      target = std::move(source);

      CHECK_EQ(target.Get<Counter>().value, 5);
    }
  }

  TEST_CASE("ecs::ResourceManager::Clear") {
    SUBCASE("Clear on empty manager is a no-op") {
      ResourceManager manager;
      manager.Clear();
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Clear removes all resources") {
      ResourceManager manager;

      manager.Emplace<Counter>();
      manager.Emplace<Physics>();
      manager.Clear();

      CHECK_EQ(manager.Count(), 0);
      CHECK_FALSE(manager.Has<Counter>());
      CHECK_FALSE(manager.Has<Physics>());
    }

    SUBCASE("After Clear, same types can be re-inserted") {
      ResourceManager manager;

      manager.Emplace<Counter>(1);
      manager.Clear();
      manager.Emplace<Counter>(2);

      CHECK_EQ(manager.Get<Counter>().value, 2);
    }
  }

  TEST_CASE("ecs::ResourceManager::Insert") {
    SUBCASE("Insert stores the resource and makes it retrievable") {
      ResourceManager manager;
      manager.Insert(Counter{42});
      CHECK_EQ(manager.Get<Counter>().value, 42);
    }

    SUBCASE("Insert with lvalue copies the resource") {
      ResourceManager manager;

      Counter cnt{10};
      manager.Insert(cnt);

      CHECK_EQ(manager.Get<Counter>().value, 10);
    }

    SUBCASE("Insert replaces an existing resource of the same type") {
      ResourceManager manager;

      manager.Insert(Counter{1});
      manager.Insert(Counter{2});

      CHECK_EQ(manager.Get<Counter>().value, 2);
      CHECK_EQ(manager.Count(), 1);
    }

    SUBCASE("Inserting different types increases the count independently") {
      ResourceManager manager;

      manager.Insert(Counter{});
      manager.Insert(Physics{});

      CHECK_EQ(manager.Count(), 2);
    }

    SUBCASE("Inserting multiple resources") {
      ResourceManager manager;
      manager.Insert(Counter{1}, Physics{});
      CHECK_EQ(manager.Count(), 2);
    }
  }

  TEST_CASE("ecs::ResourceManager::TryInsert") {
    SUBCASE("TryInsert succeeds and returns true when resource is absent") {
      ResourceManager manager;

      const bool inserted = manager.TryInsert(Counter{5});

      CHECK(inserted);
      CHECK_EQ(manager.Get<Counter>().value, 5);
    }

    SUBCASE(
        "TryInsert returns false and does not overwrite existing resource") {
      ResourceManager manager;

      manager.Insert(Counter{1});
      const bool inserted = manager.TryInsert(Counter{99});

      CHECK_FALSE(inserted);
      CHECK_EQ(manager.Get<Counter>().value, 1);
    }

    SUBCASE("TryInsert for a different type succeeds independently") {
      ResourceManager manager;

      manager.Insert(Counter{});
      const bool inserted = manager.TryInsert(Physics{});

      CHECK(inserted);
      CHECK_EQ(manager.Count(), 2);
    }

    SUBCASE("TryInsert for multiple resources returns correct results") {
      ResourceManager manager;
      manager.Insert(Physics{});
      const auto results = manager.TryInsert(Counter{1}, Physics{});

      CHECK(results[0]);
      CHECK_FALSE(results[1]);
      CHECK_EQ(manager.Count(), 2);
    }
  }

  TEST_CASE("ecs::ResourceManager::Emplace") {
    SUBCASE("Emplace default-constructs a resource in-place") {
      ResourceManager manager;
      manager.Emplace<Counter>();
      CHECK_EQ(manager.Get<Counter>().value, 0);
    }

    SUBCASE("Emplace forwards constructor arguments") {
      ResourceManager manager;

      manager.Emplace<Vec3>(1.0F, 2.0F, 3.0F);

      const auto& vec = manager.Get<Vec3>();
      CHECK_EQ(vec.x, 1.0F);
      CHECK_EQ(vec.y, 2.0F);
      CHECK_EQ(vec.z, 3.0F);
    }

    SUBCASE("Emplace replaces an existing resource") {
      ResourceManager manager;

      manager.Emplace<Counter>(1);
      manager.Emplace<Counter>(2);

      CHECK_EQ(manager.Get<Counter>().value, 2);
      CHECK_EQ(manager.Count(), 1);
    }

    SUBCASE("Emplace with std::string resource stores the value correctly") {
      ResourceManager manager;

      manager.Emplace<Config>("engine", 3);

      CHECK_EQ(manager.Get<Config>().name, "engine");
      CHECK_EQ(manager.Get<Config>().version, 3);
    }
  }

  TEST_CASE("ecs::ResourceManager::TryEmplace") {
    SUBCASE("TryEmplace succeeds and returns true when resource is absent") {
      ResourceManager manager;

      const bool emplaced = manager.TryEmplace<Counter>(7);

      CHECK(emplaced);
      CHECK_EQ(manager.Get<Counter>().value, 7);
    }

    SUBCASE(
        "TryEmplace returns false and does not overwrite existing resource") {
      ResourceManager manager;

      manager.Emplace<Counter>(1);
      const bool emplaced = manager.TryEmplace<Counter>(99);

      CHECK_FALSE(emplaced);
      CHECK_EQ(manager.Get<Counter>().value, 1);
    }

    SUBCASE("TryEmplace with multiple args forwards them correctly") {
      ResourceManager manager;

      const bool emplaced = manager.TryEmplace<Vec3>(4.0F, 5.0F, 6.0F);

      CHECK(emplaced);
      CHECK_EQ(manager.Get<Vec3>().x, 4.0F);
    }
  }

  TEST_CASE("ecs::ResourceManager::Remove") {
    SUBCASE("Remove eliminates the resource") {
      ResourceManager manager;

      manager.Emplace<Counter>();
      manager.Remove<Counter>();

      CHECK_FALSE(manager.Has<Counter>());
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Remove only affects the targeted type") {
      ResourceManager manager;

      manager.Emplace<Counter>();
      manager.Emplace<Physics>();
      manager.Remove<Counter>();

      CHECK_FALSE(manager.Has<Counter>());
      CHECK(manager.Has<Physics>());
      CHECK_EQ(manager.Count(), 1);
    }

    SUBCASE("Removed type can be re-inserted afterwards") {
      ResourceManager manager;

      manager.Emplace<Counter>(1);
      manager.Remove<Counter>();
      manager.Emplace<Counter>(2);

      CHECK_EQ(manager.Get<Counter>().value, 2);
    }
  }

  TEST_CASE("ecs::ResourceManager::TryRemove") {
    SUBCASE("TryRemove returns true and removes existing resource") {
      ResourceManager manager;

      manager.Emplace<Counter>();
      const bool removed = manager.TryRemove<Counter>();

      CHECK(removed);
      CHECK_FALSE(manager.Has<Counter>());
    }

    SUBCASE("TryRemove returns false when resource does not exist") {
      ResourceManager manager;
      const bool removed = manager.TryRemove<Counter>();
      CHECK_FALSE(removed);
    }

    SUBCASE("TryRemove does not affect other resources") {
      ResourceManager manager;

      manager.Emplace<Counter>();
      manager.Emplace<Physics>();
      manager.TryRemove<Counter>();

      CHECK(manager.Has<Physics>());
    }
  }

  TEST_CASE("ecs::ResourceManager::Get (mutable)") {
    SUBCASE("Get returns mutable reference to stored resource") {
      ResourceManager manager;

      manager.Emplace<Counter>(5);
      auto& cnt = manager.Get<Counter>();

      CHECK_EQ(cnt.value, 5);
    }

    SUBCASE("Modification through Get is reflected in subsequent Get") {
      ResourceManager manager;

      manager.Emplace<Counter>(0);
      manager.Get<Counter>().value = 42;

      CHECK_EQ(manager.Get<Counter>().value, 42);
    }

    SUBCASE("Multiple distinct resources are independently addressable") {
      ResourceManager manager;

      manager.Emplace<Counter>(1);
      manager.Emplace<Physics>();

      manager.Get<Counter>().value = 100;
      manager.Get<Physics>().gravity = 1.0F;

      CHECK_EQ(manager.Get<Counter>().value, 100);
      CHECK_EQ(doctest::Approx(manager.Get<Physics>().gravity), 1.0F);
    }
  }

  TEST_CASE("ecs::ResourceManager::Get (const)") {
    SUBCASE("Const Get returns const reference") {
      ResourceManager manager;

      manager.Emplace<Counter>(3);
      const ResourceManager& const_manager = manager;
      const auto& cnt = const_manager.Get<Counter>();

      CHECK_EQ(cnt.value, 3);
    }

    SUBCASE("Const Get value matches what was inserted") {
      ResourceManager manager;

      manager.Insert(Config{"app", 2});
      const ResourceManager& const_manager = manager;

      CHECK_EQ(const_manager.Get<Config>().name, "app");
      CHECK_EQ(const_manager.Get<Config>().version, 2);
    }
  }

  TEST_CASE("ecs::ResourceManager::TryGet (mutable)") {
    SUBCASE("TryGet returns pointer to existing resource") {
      ResourceManager manager;

      manager.Emplace<Counter>(9);
      auto* ptr = manager.TryGet<Counter>();

      REQUIRE(ptr != nullptr);
      CHECK_EQ(ptr->value, 9);
    }

    SUBCASE("TryGet returns nullptr for missing resource") {
      ResourceManager manager;
      CHECK_EQ(manager.TryGet<Counter>(), nullptr);
    }

    SUBCASE("Modification via TryGet pointer persists") {
      ResourceManager manager;

      manager.Emplace<Counter>(0);
      manager.TryGet<Counter>()->value = 77;

      CHECK_EQ(manager.Get<Counter>().value, 77);
    }
  }

  TEST_CASE("ecs::ResourceManager::TryGet (const)") {
    SUBCASE("Const TryGet returns const pointer to existing resource") {
      ResourceManager manager;

      manager.Emplace<Counter>(4);
      const ResourceManager& const_manager = manager;
      const auto* ptr = const_manager.TryGet<Counter>();

      REQUIRE(ptr != nullptr);
      CHECK_EQ(ptr->value, 4);
    }

    SUBCASE("Const TryGet returns nullptr for missing resource") {
      const ResourceManager manager;
      CHECK_EQ(manager.TryGet<Counter>(), nullptr);
    }
  }

  TEST_CASE("ecs::ResourceManager::Has") {
    SUBCASE("Has returns false on empty manager") {
      const ResourceManager manager;
      CHECK_FALSE(manager.Has<Counter>());
    }

    SUBCASE("Has returns true after inserting resource") {
      ResourceManager manager;
      manager.Emplace<Counter>();
      CHECK(manager.Has<Counter>());
    }

    SUBCASE("Has returns false after removing resource") {
      ResourceManager manager;

      manager.Emplace<Counter>();
      manager.Remove<Counter>();

      CHECK_FALSE(manager.Has<Counter>());
    }

    SUBCASE("Has is independent per type") {
      ResourceManager manager;

      manager.Emplace<Counter>();

      CHECK(manager.Has<Counter>());
      CHECK_FALSE(manager.Has<Physics>());
    }
  }

  TEST_CASE("ecs::ResourceManager::Count") {
    SUBCASE("Count is zero for empty manager") {
      const ResourceManager manager;
      CHECK_EQ(manager.Count(), 0);
    }

    SUBCASE("Count increases with each distinct type inserted") {
      ResourceManager manager;
      manager.Emplace<Counter>();
      CHECK_EQ(manager.Count(), 1);

      manager.Emplace<Physics>();
      CHECK_EQ(manager.Count(), 2);

      manager.Emplace<Timer>();
      CHECK_EQ(manager.Count(), 3);
    }

    SUBCASE(
        "Count does not increase when same type is overwritten via Insert") {
      ResourceManager manager;

      manager.Emplace<Counter>(1);
      manager.Insert(Counter{});

      CHECK_EQ(manager.Count(), 1);
    }

    SUBCASE("Count decreases after Remove") {
      ResourceManager manager;

      manager.Emplace<Counter>();
      manager.Emplace<Physics>();
      manager.Remove<Counter>();

      CHECK_EQ(manager.Count(), 1);
    }

    SUBCASE("Count returns zero after Clear") {
      ResourceManager manager;

      manager.Emplace<Counter>();
      manager.Emplace<Physics>();
      manager.Clear();

      CHECK_EQ(manager.Count(), 0);
    }
  }
}
