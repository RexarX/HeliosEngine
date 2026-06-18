#include <doctest/doctest.h>

#include <helios/ecs/component/archetype.hpp>
#include <helios/ecs/component/component.hpp>

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

constexpr auto pos_idx = ComponentTypeIndex::From<Position>();
constexpr auto vel_idx = ComponentTypeIndex::From<Velocity>();

}  // namespace

TEST_SUITE("helios::ecs::Archetype") {
  TEST_CASE("ecs::Archetype::ctor") {
    SUBCASE("Ctor with empty archetype id") {
      ArchetypeId id;
      Archetype archetype(id);

      CHECK(archetype.Empty());
      CHECK_EQ(archetype.ColumnCount(), 0);
      CHECK_EQ(archetype.EntityCount(), 0);
    }

    SUBCASE("Ctor with single component") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));

      CHECK(archetype.Empty());
      CHECK_EQ(archetype.ColumnCount(), 1);
      CHECK_EQ(archetype.EntityCount(), 0);  // No entities yet
    }

    SUBCASE("Ctor with multiple components") {
      ArchetypeId id{pos_idx, vel_idx};
      Archetype archetype(std::move(id));
      CHECK(archetype.Empty());
    }

    SUBCASE("Move ctor") {
      ArchetypeId id{pos_idx};

      Archetype archetype1(std::move(id));
      archetype1.AllocateRow(Entity{1, 0});

      Archetype archetype2(std::move(archetype1));

      CHECK_EQ(archetype2.ColumnCount(), 1);
      CHECK_EQ(archetype2.EntityCount(), 1);  // No entities yet
    }
  }

  TEST_CASE("ecs::Archetype::operator=") {
    SUBCASE("Move assignment") {
      ArchetypeId id{pos_idx};

      Archetype archetype1(id);
      archetype1.AllocateRow(Entity{1, 0});

      Archetype archetype2(std::move(id));
      archetype2 = std::move(archetype1);

      CHECK_EQ(archetype2.ColumnCount(), 1);
      CHECK_EQ(archetype2.EntityCount(), 1);
    }
  }

  TEST_CASE("ecs::Archetype::Clear") {
    ArchetypeId id{pos_idx};
    Archetype archetype(std::move(id));

    archetype.AllocateRow(Entity{1, 0});
    archetype.AllocateRow(Entity{2, 0});
    archetype.AllocateRow(Entity{3, 0});

    SUBCASE("Clear removes all entities") {
      archetype.Clear();
      CHECK(archetype.Empty());
    }

    SUBCASE("Can allocate after clear") {
      archetype.Clear();
      const auto row = archetype.AllocateRow(Entity{100, 0});

      CHECK_NE(row, Archetype::kInvalidRow);
      CHECK_EQ(archetype.EntityCount(), 1);
    }
  }

  TEST_CASE("ecs::Archetype::AllocateRow") {
    SUBCASE("Allocate row for entity") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));
      const Entity entity{1, 0};

      const auto row = archetype.AllocateRow(entity);

      CHECK_NE(row, Archetype::kInvalidRow);
      CHECK_EQ(archetype.EntityCount(), 1);
    }

    SUBCASE("Allocate multiple rows") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));

      const auto row1 = archetype.AllocateRow(Entity{1, 0});
      const auto row2 = archetype.AllocateRow(Entity{2, 0});

      CHECK_NE(row1, row2);
      CHECK_EQ(archetype.EntityCount(), 2);
    }

    SUBCASE("Entity is retrievable after allocation") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));
      const Entity entity{5, 3};

      archetype.AllocateRow(entity);

      const auto entities = archetype.Entities();
      CHECK_NE(std::ranges::find(entities, entity), entities.end());
    }
  }

  TEST_CASE("ecs::Archetype::Add") {
    const Position position{.x = 10.0F, .y = 20.0F};
    const Velocity velocity{.x = 1.0F, .y = 2.0F};
    const Entity entity{1, 0};

    SUBCASE("Add entity with single component") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));

      const auto row = archetype.Add(entity, position);
      CHECK_NE(row, Archetype::kInvalidRow);

      const auto entities = archetype.Entities();
      CHECK_NE(std::ranges::find(entities, entity), entities.end());

      const auto* pos = archetype.TryGet<Position>(entity);
      CHECK_NE(pos, nullptr);
      CHECK_EQ(*pos, position);

      CHECK_EQ(archetype.EntityCount(), 1);
      CHECK_EQ(archetype.ColumnCount(), 1);
    }

    SUBCASE("Add entity with multiple components") {
      ArchetypeId id{pos_idx, vel_idx};
      Archetype archetype(std::move(id));

      const auto row = archetype.Add(entity, position, velocity);
      CHECK_NE(row, Archetype::kInvalidRow);

      const auto entities = archetype.Entities();
      CHECK_NE(std::ranges::find(entities, entity), entities.end());

      const auto* pos = archetype.TryGet<Position>(entity);
      const auto* vel = archetype.TryGet<Velocity>(entity);
      CHECK_NE(pos, nullptr);
      CHECK_NE(vel, nullptr);
      CHECK_EQ(*pos, position);
      CHECK_EQ(*vel, velocity);

      CHECK_EQ(archetype.EntityCount(), 1);
      CHECK_EQ(archetype.ColumnCount(), 2);
    }

    SUBCASE("Add multiple entities") {
      ArchetypeId id{pos_idx, vel_idx};
      Archetype archetype(std::move(id));
      const Entity entity1{1, 0};
      const Entity entity2{2, 0};

      const auto row1 = archetype.Add(entity1, position);
      const auto row2 = archetype.Add(entity2, velocity);

      CHECK_NE(row1, Archetype::kInvalidRow);
      CHECK_NE(row2, Archetype::kInvalidRow);
      CHECK_NE(row1, row2);

      const auto entities = archetype.Entities();
      CHECK_NE(std::ranges::find(entities, entity1), entities.end());
      CHECK_NE(std::ranges::find(entities, entity2), entities.end());

      const auto* pos = archetype.TryGet<Position>(entity);
      const auto* vel = archetype.TryGet<Velocity>(entity);
      CHECK_NE(pos, nullptr);
      CHECK_NE(vel, nullptr);
      CHECK_EQ(*pos, position);
      CHECK_EQ(*vel, velocity);

      CHECK_EQ(archetype.EntityCount(), 2);
      CHECK_EQ(archetype.ColumnCount(), 2);
    }
  }

  TEST_CASE("ecs::Archetype::Remove") {
    ArchetypeId id;
    Archetype archetype(std::move(id));
    const Entity entity{1, 0};

    archetype.AllocateRow(entity);
    archetype.Remove(entity);

    const auto entities = archetype.Entities();
    CHECK_EQ(std::ranges::find(entities, entity), entities.end());

    CHECK_EQ(archetype.EntityCount(), 0);
    CHECK(archetype.Empty());
  }

  TEST_CASE("ecs::Archetype::Set") {
    ArchetypeId id{pos_idx};
    Archetype archetype(std::move(id));
    const Entity entity{1, 0};
    const Position new_position{.x = 30.0F, .y = 40.0F};

    archetype.Add(entity, Position{.x = 10.0F, .y = 20.0F});
    archetype.Set(entity, new_position);

    const auto* pos = archetype.TryGet<Position>(entity);
    CHECK_NE(pos, nullptr);
    CHECK_EQ(*pos, new_position);
  }

  TEST_CASE("ecs::Archetype::Emplace") {
    ArchetypeId id{pos_idx};
    Archetype archetype(std::move(id));
    const Entity entity{1, 0};

    archetype.AllocateRow(entity);
    archetype.Emplace<Position>(entity, 10.0F, 20.0F);

    const auto* pos = archetype.TryGet<Position>(entity);
    CHECK_NE(pos, nullptr);
    CHECK_EQ(*pos, Position{.x = 10.0F, .y = 20.0F});
  }

  TEST_CASE("ecs::Archetype::Get") {
    ArchetypeId id{pos_idx};
    Archetype archetype(std::move(id));
    const Entity entity{1, 0};
    const Position position{.x = 10.0F, .y = 20.0F};

    archetype.Add(entity, position);

    const auto& pos = archetype.Get<Position>(entity);
    CHECK_EQ(pos, position);
  }

  TEST_CASE("ecs::Archetype::TryGet") {
    ArchetypeId id{pos_idx};
    Archetype archetype(std::move(id));
    const Entity entity{1, 0};
    const Position position{.x = 10.0F, .y = 20.0F};

    archetype.Add(entity, position);

    SUBCASE("Component exists") {
      const auto* pos = archetype.TryGet<Position>(entity);
      CHECK_NE(pos, nullptr);
      CHECK_EQ(*pos, position);
    }

    SUBCASE("Component doesn't exist") {
      const auto* vel = archetype.TryGet<Velocity>(entity);
      CHECK_EQ(vel, nullptr);
    }
  }

  TEST_CASE("ecs::Archetype::ComponentColumn") {
    ArchetypeId id{pos_idx};
    Archetype archetype(std::move(id));
    const Position pos1{.x = 10.0F, .y = 20.0F};
    const Position pos2{.x = 30.0F, .y = 40.0F};

    archetype.Add(Entity{1, 0}, pos1);
    archetype.Add(Entity{2, 0}, pos2);

    SUBCASE("Column exists") {
      const auto pos_column = archetype.ComponentColumn<Position>();

      CHECK_EQ(pos_column.size(), 2);
      CHECK_EQ(pos_column[0], pos1);
      CHECK_EQ(pos_column[1], pos2);
    }

    SUBCASE("Column doesn't exist") {
      const auto vel_column = archetype.ComponentColumn<Velocity>();
      CHECK(vel_column.empty());
    }
  }

  TEST_CASE("ecs::Archetype::Column") {
    ArchetypeId id{pos_idx};
    Archetype archetype(std::move(id));
    const Position pos1{.x = 10.0F, .y = 20.0F};
    const Position pos2{.x = 30.0F, .y = 40.0F};

    archetype.Add(Entity{1, 0}, pos1);
    archetype.Add(Entity{2, 0}, pos2);

    const auto& pos_column = archetype.Column(pos_idx);
    CHECK_EQ(pos_column.Size(), 2);
    CHECK_EQ(pos_column.At<Position>(0), pos1);
    CHECK_EQ(pos_column.At<Position>(1), pos2);
  }

  TEST_CASE("ecs::Archetype::TryColumn") {
    ArchetypeId id{pos_idx};
    Archetype archetype(std::move(id));
    const Position pos1{.x = 10.0F, .y = 20.0F};
    const Position pos2{.x = 30.0F, .y = 40.0F};

    archetype.Add(Entity{1, 0}, pos1);
    archetype.Add(Entity{2, 0}, pos2);

    SUBCASE("Column exists") {
      const auto* pos_column = archetype.TryColumn(pos_idx);

      CHECK_NE(pos_column, nullptr);
      CHECK_EQ(pos_column->Size(), 2);
      CHECK_EQ(pos_column->At<Position>(0), pos1);
      CHECK_EQ(pos_column->At<Position>(1), pos2);
    }

    SUBCASE("Column doesn't exist") {
      const auto* vel_column = archetype.TryColumn(vel_idx);
      CHECK_EQ(vel_column, nullptr);
    }
  }

  TEST_CASE("ecs::Archetype::Row") {
    ArchetypeId id;
    Archetype archetype(std::move(id));
    const Entity entity{1, 0};

    const auto row = archetype.AllocateRow(entity);
    const auto row_to_check = archetype.Row(entity);

    CHECK_NE(row_to_check, Archetype::kInvalidRow);
    CHECK_EQ(row, row_to_check);
  }

  TEST_CASE("ecs::Archetype::EntityAt") {
    ArchetypeId id;
    Archetype archetype(std::move(id));
    const Entity entity{1, 0};

    const auto row = archetype.AllocateRow(entity);
    const auto entity_at_row = archetype.EntityAt(row);

    CHECK_EQ(entity_at_row, entity);
  }

  TEST_CASE("ecs::Archetype::Empty") {
    ArchetypeId id{pos_idx};
    Archetype archetype(std::move(id));
    const Entity entity{1, 0};

    SUBCASE("Archetype is empty initially") {
      CHECK(archetype.Empty());
    }

    SUBCASE("Archetype not empty after AllocateRow") {
      archetype.AllocateRow(entity);
      CHECK_FALSE(archetype.Empty());
    }

    SUBCASE("Archetype not empty after Add") {
      archetype.Add(entity, Position{});
      CHECK_FALSE(archetype.Empty());
    }

    SUBCASE("Archetype empty after clear") {
      archetype.AllocateRow(entity);
      archetype.Clear();

      CHECK(archetype.Empty());
    }
  }

  TEST_CASE("ecs::Archetype::Contains") {
    Entity entity{1, 0};

    SUBCASE("Archetype contains entity after AllocateRow") {
      ArchetypeId id;
      Archetype archetype(std::move(id));

      archetype.AllocateRow(entity);

      CHECK(archetype.Contains(entity));
    }

    SUBCASE("Archetype contains entity after Add") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));

      archetype.Add(entity, Position{});

      CHECK(archetype.Contains(entity));
    }

    SUBCASE("Archetype doesn't contain entity after Clear") {
      ArchetypeId id;
      Archetype archetype(std::move(id));

      archetype.AllocateRow(entity);
      archetype.Clear();

      CHECK_FALSE(archetype.Contains(entity));
    }
  }

  TEST_CASE("ecs::Archetype::HasColumn") {
    SUBCASE("Archetype has column") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));
      CHECK(archetype.HasColumn(pos_idx));
    }

    SUBCASE("Archetype doesn't have column") {
      ArchetypeId id;
      Archetype archetype(std::move(id));
      CHECK_FALSE(archetype.HasColumn(pos_idx));
    }

    SUBCASE("Archetype have column after Clear") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));

      archetype.Clear();

      CHECK(archetype.HasColumn(pos_idx));
    }
  }

  TEST_CASE("ecs::Archetype::EntityCount") {
    const Entity entity{1, 0};

    SUBCASE("Archetype entity count equals zero initially") {
      ArchetypeId id;
      Archetype archetype(std::move(id));
      CHECK_EQ(archetype.EntityCount(), 0);
    }

    SUBCASE("Archetype entity count increases with AllocateRow") {
      ArchetypeId id;
      Archetype archetype(std::move(id));

      archetype.AllocateRow(entity);

      CHECK_EQ(archetype.EntityCount(), 1);
    }

    SUBCASE("Archetype entity count increases with Add") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));

      archetype.Add(entity, Position{});

      CHECK_EQ(archetype.EntityCount(), 1);
    }

    SUBCASE("Archetype entity count decreases after Remove") {
      ArchetypeId id;
      Archetype archetype(std::move(id));

      archetype.AllocateRow(entity);
      archetype.Remove(entity);

      CHECK_EQ(archetype.EntityCount(), 0);
    }

    SUBCASE("Archetype entity count equals 0 after Clear") {
      ArchetypeId id;
      Archetype archetype(std::move(id));

      archetype.AllocateRow(entity);
      archetype.Clear();

      CHECK_EQ(archetype.EntityCount(), 0);
    }
  }

  TEST_CASE("ecs::Archetype::ColumnCount") {
    ArchetypeId id{pos_idx};
    const auto expected_cols = id.Size();
    Archetype archetype(std::move(id));
    CHECK_EQ(archetype.ColumnCount(), expected_cols);
  }

  TEST_CASE("ecs::Archetype::Entities") {
    SUBCASE("Entities span is empty initially") {
      ArchetypeId id;
      Archetype archetype(std::move(id));

      const auto entities = archetype.Entities();

      CHECK(entities.empty());
    }

    SUBCASE("Entities span contains entities after AllocateRow and Add") {
      ArchetypeId id{pos_idx};
      Archetype archetype(std::move(id));
      const Entity entity1{1, 0};
      const Entity entity2{2, 0};

      archetype.AllocateRow(entity1);
      archetype.Add(entity2, Position{});

      const auto entities = archetype.Entities();
      CHECK_EQ(entities.size(), 2);
      CHECK_NE(std::ranges::find(entities, entity1), entities.end());
      CHECK_NE(std::ranges::find(entities, entity2), entities.end());
    }
  }

  TEST_CASE("ecs::Archetype::Id") {
    SUBCASE("Archetype ID is correct") {
      ArchetypeId id{pos_idx, vel_idx};
      const auto expected_id = id;
      Archetype archetype(std::move(id));
      CHECK_EQ(archetype.Id(), expected_id);
    }
  }
}
