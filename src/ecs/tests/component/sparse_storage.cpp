#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/component/sparse_storage.hpp>
#include <helios/ecs/entity/entity.hpp>

using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const Position& other) const noexcept = default;
};

struct Velocity {
  float x = 0.0F;
  float y = 0.0F;

  constexpr bool operator==(const Velocity& other) const noexcept = default;
};

}  // namespace

TEST_SUITE("helios::ecs::SparseComponentStorage") {
  TEST_CASE("ecs::SparseComponentStorage::Clear") {
    const Entity e1{1, 0};
    const Position pos1{.x = 1.0F, .y = 2.0F};
    const Position pos2{.x = 3.0F, .y = 4.0F};
    const Position pos3{.x = 5.0F, .y = 6.0F};

    SUBCASE("Clear on empty storage is a no-op") {
      SparseComponentStorage<Position> storage;
      storage.Clear();
      CHECK_EQ(storage.Size(), 0);
    }

    SUBCASE("Clear removes all components") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, pos1);
      storage.Set(Entity{2, 0}, pos2);
      storage.Set(Entity{3, 0}, pos3);
      storage.Clear();

      CHECK_EQ(storage.Size(), 0);
    }

    SUBCASE("Contains returns false for all entities after Clear") {
      SparseComponentStorage<Position> storage;
      storage.Set(e1, Position{});
      storage.Clear();

      CHECK_FALSE(storage.Contains(e1));
    }

    SUBCASE("Storage is usable after Clear") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, pos1);
      storage.Clear();
      storage.Set(e1, pos2);

      CHECK_EQ(storage.Size(), 1);
      CHECK_EQ(storage.Get(e1), pos2);
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::Remove") {
    const Entity e1{1, 0};

    SUBCASE("Remove decrements size") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, Position{});
      storage.Remove(e1);

      CHECK_EQ(storage.Size(), 0);
    }

    SUBCASE("Contains returns false after Remove") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, Position{});
      storage.Remove(e1);

      CHECK_FALSE(storage.Contains(e1));
    }

    SUBCASE("Removing one entity does not affect others") {
      SparseComponentStorage<Position> storage;
      const Entity e2{2, 0};
      const Position pos{.x = 2.0F, .y = 0.0F};

      storage.Set(e1, Position{.x = 1.0F, .y = 0.0F});
      storage.Set(e2, pos);
      storage.Remove(e1);

      CHECK_FALSE(storage.Contains(e1));
      CHECK(storage.Contains(e2));
      CHECK_EQ(storage.Get(e2), pos);
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::TryRemove") {
    const Entity entity{1, 0};

    SUBCASE("Returns true and removes existing component") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, Position{});
      const bool removed = storage.TryRemove(entity);

      CHECK(removed);
      CHECK_FALSE(storage.Contains(entity));
    }

    SUBCASE("Returns false when entity does not have the component") {
      SparseComponentStorage<Position> storage;
      const bool removed = storage.TryRemove(entity);
      CHECK_FALSE(removed);
    }

    SUBCASE("Size decrements only when TryRemove succeeds") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, Position{});
      storage.TryRemove(entity);

      CHECK_EQ(storage.Size(), 0);
      storage.TryRemove(entity);  // second call — should be a no-op
      CHECK_EQ(storage.Size(), 0);
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::Set") {
    const Entity e1{1, 0};
    const Position pos1{.x = 1.0F, .y = 2.0F};
    const Position pos2{.x = 3.0F, .y = 4.0F};

    SUBCASE("Entity has component after Set") {
      SparseComponentStorage<Position> storage;
      storage.Set(e1, pos1);
      CHECK(storage.Contains(e1));
    }

    SUBCASE("Value is stored correctly") {
      SparseComponentStorage<Position> storage;
      storage.Set(e1, pos1);
      CHECK_EQ(storage.Get(e1), pos1);
    }

    SUBCASE("Size increments on first Set") {
      SparseComponentStorage<Position> storage;
      storage.Set(e1, pos1);
      CHECK_EQ(storage.Size(), 1);
    }

    SUBCASE("Repeated Set replaces the existing component") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, pos1);
      storage.Set(e1, pos2);

      CHECK_EQ(storage.Get(e1), pos2);
    }

    SUBCASE("Repeated Set does not change Size") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, pos1);
      storage.Set(e1, pos2);

      CHECK_EQ(storage.Size(), 1);
    }

    SUBCASE("Multiple entities are stored independently") {
      SparseComponentStorage<Position> storage;
      const Entity e2{2, 0};

      storage.Set(e1, pos1);
      storage.Set(e2, pos2);

      CHECK_EQ(storage.Get(e1), pos1);
      CHECK_EQ(storage.Get(e2), pos2);
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::TrySet") {
    const Entity entity{1, 0};
    const Position pos1{.x = 1.0F, .y = 2.0F};
    const Position pos2{.x = 3.0F, .y = 4.0F};

    SUBCASE(
        "Returns true and inserts when entity does not have the component") {
      SparseComponentStorage<Position> storage;

      const bool inserted = storage.TrySet(entity, pos1);
      CHECK(inserted);
      CHECK(storage.Contains(entity));
      CHECK_EQ(storage.Get(entity), pos1);
    }

    SUBCASE("Returns false when entity already has the component") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, pos1);

      const bool inserted = storage.TrySet(entity, pos2);
      CHECK_FALSE(inserted);
    }

    SUBCASE("Existing value is unchanged when TrySet returns false") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, pos1);
      storage.TrySet(entity, pos2);

      CHECK_EQ(storage.Get(entity), pos1);
    }

    SUBCASE("Size does not change when TrySet returns false") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, pos1);
      storage.TrySet(entity, pos1);

      CHECK_EQ(storage.Size(), 1);
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::Emplace") {
    const Entity entity{1, 0};

    SUBCASE("Entity has component after Emplace") {
      SparseComponentStorage<Position> storage;
      storage.Emplace(entity, 4.0F, 8.0F);
      CHECK(storage.Contains(entity));
    }

    SUBCASE("Value is correct after Emplace") {
      SparseComponentStorage<Position> storage;
      storage.Emplace(entity, 4.0F, 8.0F);
      CHECK_EQ(storage.Get(entity), Position{.x = 4.0F, .y = 8.0F});
    }

    SUBCASE("Emplace replaces existing component") {
      SparseComponentStorage<Position> storage;

      storage.Emplace(entity, 1.0F, 2.0F);
      storage.Emplace(entity, 9.0F, 9.0F);

      CHECK_EQ(storage.Get(entity), Position{.x = 9.0F, .y = 9.0F});
    }

    SUBCASE("Repeated Emplace does not change Size") {
      SparseComponentStorage<Position> storage;

      storage.Emplace(entity, 1.0F, 2.0F);
      storage.Emplace(entity, 3.0F, 4.0F);

      CHECK_EQ(storage.Size(), 1);
    }

    SUBCASE("Default construction via Emplace with no args") {
      SparseComponentStorage<Position> storage;
      storage.Emplace(entity);
      CHECK_EQ(storage.Get(entity), Position{});
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::TryEmplace") {
    const Entity entity{1, 0};
    const Position pos{.x = 1.0F, .y = 2.0F};

    SUBCASE(
        "Returns true and emplaces when entity does not have the component") {
      SparseComponentStorage<Position> storage;

      const bool emplaced = storage.TryEmplace(entity, 1.0F, 2.0F);
      CHECK(emplaced);
      CHECK_EQ(storage.Get(entity), pos);
    }

    SUBCASE("Returns false when entity already has the component") {
      SparseComponentStorage<Position> storage;

      storage.Emplace(entity, 1.0F, 2.0F);

      const bool emplaced = storage.TryEmplace(entity, 9.0F, 9.0F);
      CHECK_FALSE(emplaced);
    }

    SUBCASE("Existing value is unchanged when TryEmplace returns false") {
      SparseComponentStorage<Position> storage;

      storage.Emplace(entity, 1.0F, 2.0F);
      storage.TryEmplace(entity, 9.0F, 9.0F);

      CHECK_EQ(storage.Get(entity), pos);
    }

    SUBCASE("Size does not change when TryEmplace returns false") {
      SparseComponentStorage<Position> storage;

      storage.Emplace(entity, 1.0F, 2.0F);
      storage.TryEmplace(entity, 3.0F, 4.0F);

      CHECK_EQ(storage.Size(), 1);
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::Get") {
    const Entity entity{1, 0};
    const Position pos{.x = 1.0F, .y = 2.0F};

    SUBCASE("Returns correct value") {
      SparseComponentStorage<Position> storage;
      storage.Set(entity, pos);
      CHECK_EQ(storage.Get(entity), pos);
    }

    SUBCASE("Returns mutable reference — mutation is reflected") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, pos);
      storage.Get(entity).x = 99.0F;

      CHECK_EQ(storage.Get(entity).x, 99.0F);
    }

    SUBCASE("Const Get returns correct value") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, pos);

      const auto& const_storage = storage;
      CHECK_EQ(const_storage.Get(entity), pos);
    }

    SUBCASE("Independent entities return independent values") {
      SparseComponentStorage<Position> storage;
      const Entity e2{2, 0};

      storage.Set(entity, pos);
      storage.Set(e2, Position{.x = 2.0F, .y = 0.0F});

      CHECK_EQ(storage.Get(entity).x, 1.0F);
      CHECK_EQ(storage.Get(e2).x, 2.0F);
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::TryGet") {
    const Entity entity{1, 0};
    const Position pos{.x = 1.0F, .y = 2.0F};

    SUBCASE("Returns non-null pointer for existing component") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, pos);
      const auto* ptr = storage.TryGet(entity);

      CHECK_NE(ptr, nullptr);
      CHECK_EQ(*ptr, pos);
    }

    SUBCASE("Returns nullptr when entity does not have the component") {
      SparseComponentStorage<Position> storage;
      const auto* ptr = storage.TryGet(entity);
      CHECK_EQ(ptr, nullptr);
    }

    SUBCASE("Mutable TryGet allows in-place mutation") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, pos);
      auto* ptr = storage.TryGet(entity);

      REQUIRE_NE(ptr, nullptr);
      ptr->x = 99.0F;
      CHECK_EQ(storage.Get(entity).x, 99.0F);
    }

    SUBCASE("Const TryGet returns non-null for existing component") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, pos);
      const auto& const_storage = storage;
      const auto* ptr = const_storage.TryGet(entity);

      CHECK_NE(ptr, nullptr);
      CHECK_EQ(*ptr, pos);
    }

    SUBCASE("Const TryGet returns nullptr when component is absent") {
      SparseComponentStorage<Position> storage;

      const auto& const_storage = storage;
      const auto* ptr = const_storage.TryGet(entity);

      CHECK_EQ(ptr, nullptr);
    }

    SUBCASE("Returns nullptr after component is removed") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, Position{});
      storage.Remove(entity);

      CHECK_EQ(storage.TryGet(entity), nullptr);
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::Contains") {
    const Entity entity{1, 0};

    SUBCASE("Returns false for empty storage") {
      SparseComponentStorage<Position> storage;
      CHECK_FALSE(storage.Contains(entity));
    }

    SUBCASE("Returns true after Set") {
      SparseComponentStorage<Position> storage;
      storage.Set(entity, Position{});
      CHECK(storage.Contains(entity));
    }

    SUBCASE("Returns true after Emplace") {
      SparseComponentStorage<Position> storage;
      storage.Emplace(entity, 1.0F, 2.0F);
      CHECK(storage.Contains(entity));
    }

    SUBCASE("Returns false after Remove") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, Position{});
      storage.Remove(entity);

      CHECK_FALSE(storage.Contains(entity));
    }

    SUBCASE("Returns false after Clear") {
      SparseComponentStorage<Position> storage;

      storage.Set(entity, Position{});
      storage.Clear();

      CHECK_FALSE(storage.Contains(entity));
    }

    SUBCASE("Returns true only for entities that have the component") {
      SparseComponentStorage<Position> storage;
      const Entity e2{2, 0};

      storage.Set(entity, Position{});

      CHECK(storage.Contains(entity));
      CHECK_FALSE(storage.Contains(e2));
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::Size") {
    const Entity e1{1, 0};
    const Entity e2{2, 0};

    SUBCASE("Size is zero for empty storage") {
      SparseComponentStorage<Position> storage;
      CHECK_EQ(storage.Size(), 0);
    }

    SUBCASE("Size increments with each new Set") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, Position{});
      CHECK_EQ(storage.Size(), 1);
      storage.Set(e2, Position{});
      CHECK_EQ(storage.Size(), 2);
      storage.Set(Entity{3, 0}, Position{});
      CHECK_EQ(storage.Size(), 3);
    }

    SUBCASE("Size decrements after Remove") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, Position{});
      storage.Remove(e1);

      CHECK_EQ(storage.Size(), 0);
    }

    SUBCASE("Size is zero after Clear") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, Position{});
      storage.Set(e2, Position{});
      storage.Clear();

      CHECK_EQ(storage.Size(), 0);
    }
  }

  TEST_CASE("ecs::SparseComponentStorage::Data") {
    const Entity e1{1, 0};
    const Entity e2{3, 0};
    const Position pos1{.x = 1.0F, .y = 2.0F};
    const Position pos2{.x = 3.0F, .y = 4.0F};

    SUBCASE("Data span is empty for empty storage") {
      SparseComponentStorage<Position> storage;
      CHECK(storage.Data().empty());
    }

    SUBCASE("Data span size matches component count") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, pos1);
      storage.Set(e2, pos2);

      CHECK_EQ(storage.Data().size(), 2);
    }

    SUBCASE("Data span contains all stored values") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, pos1);
      storage.Set(e2, pos2);
      const auto data = storage.Data();

      const bool has_pos1 = std::ranges::find(data, pos1) != data.end();
      const bool has_pos2 = std::ranges::find(data, pos2) != data.end();
      CHECK(has_pos1);
      CHECK(has_pos2);
    }

    SUBCASE("Mutation through mutable Data span is reflected in Get") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, pos1);
      storage.Data()[0].x = 99.0F;

      CHECK_EQ(storage.Get(e1).x, 99.0F);
    }

    SUBCASE("Const Data span is accessible") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, pos1);
      const auto& const_storage = storage;
      const auto data = const_storage.Data();

      CHECK_EQ(data.size(), 1);
    }

    SUBCASE("Data span is empty after Clear") {
      SparseComponentStorage<Position> storage;

      storage.Set(e1, pos1);
      storage.Clear();

      CHECK(storage.Data().empty());
    }
  }
}
