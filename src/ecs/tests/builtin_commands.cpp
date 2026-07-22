#include <doctest/doctest.h>

#include <helios/ecs/builtin_commands.hpp>
#include <helios/ecs/component/bundle.hpp>
#include <helios/ecs/world.hpp>

#include <memory>
#include <utility>
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

struct MoveOnly {
  std::unique_ptr<int> value;
};

struct Score {
  int value = 0;
};

using MotionBundle = ComponentBundle<Position, Velocity>;
using GameplayBundle = ComponentBundle<MotionBundle, Health>;

}  // namespace

TEST_SUITE("helios::ecs::FunctionCmd") {
  TEST_CASE("ecs::FunctionCmd::Execute") {
    SUBCASE("Executes lambda with world reference") {
      World world;
      bool called = false;

      FunctionCmd cmd([&called](World& /*world*/) { called = true; });
      cmd.Execute(world);

      CHECK(called);
    }

    SUBCASE("Receives correct world reference") {
      World world;
      World* received = nullptr;

      FunctionCmd cmd([&received](World& captured) { received = &captured; });
      cmd.Execute(world);

      CHECK_EQ(received, &world);
    }

    SUBCASE("Executes multiple times independently") {
      World world;
      int count = 0;

      FunctionCmd cmd([&count](World& /*world*/) { ++count; });
      cmd.Execute(world);
      cmd.Execute(world);

      CHECK_EQ(count, 2);
    }

    SUBCASE("Copy constructs correctly") {
      World world;
      int count = 0;

      FunctionCmd cmd1([&count](World& /*world*/) { ++count; });
      FunctionCmd cmd2 =
          cmd1;  // NOLINT(performance-unnecessary-copy-initialization)
      cmd1.Execute(world);
      cmd2.Execute(world);

      CHECK_EQ(count, 2);
    }

    SUBCASE("Move constructs correctly") {
      World world;
      bool called = false;

      FunctionCmd cmd1([&called](World& /*world*/) { called = true; });
      FunctionCmd cmd2 = std::move(cmd1);
      cmd2.Execute(world);

      CHECK(called);
    }
  }
}

TEST_SUITE("helios::ecs::DestroyEntityCmd") {
  TEST_CASE("ecs::DestroyEntityCmd::Execute") {
    SUBCASE("Destroys existing entity") {
      World world;
      const Entity entity = world.CreateEntity();

      DestroyEntityCmd cmd(entity);
      cmd.Execute(world);

      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("Copy constructs correctly") {
      World world;
      const Entity entity = world.CreateEntity();

      const DestroyEntityCmd cmd1(entity);
      DestroyEntityCmd cmd2 = cmd1;
      cmd2.Execute(world);

      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("Move constructs correctly") {
      World world;
      const Entity entity = world.CreateEntity();

      DestroyEntityCmd cmd1(entity);
      DestroyEntityCmd cmd2 = std::move(cmd1);
      cmd2.Execute(world);

      CHECK_FALSE(world.Exists(entity));
    }
  }
}

TEST_SUITE("helios::ecs::DestroyEntitiesCmd") {
  TEST_CASE("ecs::DestroyEntitiesCmd::Execute") {
    SUBCASE("Destroys multiple existing entities") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();
      const Entity e3 = world.CreateEntity();

      DestroyEntitiesCmd cmd(std::vector<Entity>{e1, e2, e3});
      cmd.Execute(world);

      CHECK_FALSE(world.Exists(e1));
      CHECK_FALSE(world.Exists(e2));
      CHECK_FALSE(world.Exists(e3));
    }

    SUBCASE("Empty vector is a no-op") {
      World world;
      [[maybe_unused]] const Entity entity = world.CreateEntity();

      DestroyEntitiesCmd cmd(std::vector<Entity>{});
      cmd.Execute(world);
    }
  }
}

TEST_SUITE("helios::ecs::TryDestroyEntityCmd") {
  TEST_CASE("ecs::TryDestroyEntityCmd::Execute") {
    SUBCASE("Destroys existing entity") {
      World world;
      const Entity entity = world.CreateEntity();

      TryDestroyEntityCmd cmd(entity);
      cmd.Execute(world);

      CHECK_FALSE(world.Exists(entity));
    }

    SUBCASE("Does not assert on non-existing entity") {
      World world;
      const Entity entity = world.ReserveEntity();

      // Entity was reserved but never committed to the world — should not
      // assert
      TryDestroyEntityCmd cmd(entity);
      cmd.Execute(world);
    }
  }
}

TEST_SUITE("helios::ecs::TryDestroyEntitiesCmd") {
  TEST_CASE("ecs::TryDestroyEntitiesCmd::Execute") {
    SUBCASE("Destroys only existing entities") {
      World world;
      const Entity e1 = world.CreateEntity();
      const Entity e2 = world.CreateEntity();

      TryDestroyEntitiesCmd cmd(std::vector<Entity>{e1, e2});
      cmd.Execute(world);

      CHECK_FALSE(world.Exists(e1));
      CHECK_FALSE(world.Exists(e2));
    }

    SUBCASE("Empty vector is a no-op") {
      World world;
      TryDestroyEntitiesCmd cmd(std::vector<Entity>{});
      cmd.Execute(world);
    }
  }
}

TEST_SUITE("helios::ecs::AddComponentsCmd") {
  TEST_CASE("ecs::AddComponentsCmd::Execute") {
    SUBCASE("Adds multiple components to entity") {
      World world;
      const Entity entity = world.CreateEntity();

      AddComponentsCmd cmd(entity, Position{1.0F, 2.0F}, Velocity{3.0F, 4.0F});
      cmd.Execute(world);

      CHECK(world.HasComponent<Position>(entity));
      CHECK(world.HasComponent<Velocity>(entity));
    }

    SUBCASE("Replaces existing components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{0.0F, 0.0F}, Velocity{0.0F, 0.0F});

      AddComponentsCmd cmd(entity, Position{9.0F, 9.0F}, Velocity{1.0F, 1.0F});
      cmd.Execute(world);

      CHECK_EQ(world.ReadComponent<Position>(entity).x, doctest::Approx(9.0F));
      CHECK_EQ(world.ReadComponent<Velocity>(entity).dx, doctest::Approx(1.0F));
    }
  }
}

TEST_SUITE("helios::ecs::AddBundleCmd") {
  TEST_CASE("ecs::AddBundleCmd::Execute") {
    SUBCASE("Adds a nested component bundle to entity") {
      World world;
      const Entity entity = world.CreateEntity();

      AddBundleCmd cmd(
          entity, GameplayBundle{
                      MotionBundle{Position{1.0F, 2.0F}, Velocity{3.0F, 4.0F}},
                      Health{75}});
      cmd.Execute(world);

      CHECK_EQ(world.ReadComponent<Position>(entity).x, doctest::Approx(1.0F));
      CHECK_EQ(world.ReadComponent<Velocity>(entity).dx, doctest::Approx(3.0F));
      CHECK_EQ(world.ReadComponent<Health>(entity).value, 75);
    }

    SUBCASE("Owns a move-only bundle until execution") {
      World world;
      const Entity entity = world.CreateEntity();

      AddBundleCmd cmd(entity, ComponentBundle<MoveOnly>{
                                   MoveOnly{std::make_unique<int>(42)}});
      cmd.Execute(world);

      const auto& component = world.ReadComponent<MoveOnly>(entity);
      CHECK_NE(component.value.get(), nullptr);
      CHECK_EQ(*component.value, 42);
    }
  }
}

TEST_SUITE("helios::ecs::TryAddComponentsCmd") {
  TEST_CASE("ecs::TryAddComponentsCmd::Execute") {
    SUBCASE("Adds only missing components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{7.0F, 7.0F});

      TryAddComponentsCmd cmd(entity, Position{0.0F, 0.0F},
                              Velocity{3.0F, 4.0F});
      cmd.Execute(world);

      // Position was already present — must not be overwritten
      CHECK_EQ(world.ReadComponent<Position>(entity).x, doctest::Approx(7.0F));
      CHECK(world.HasComponent<Velocity>(entity));
    }
  }
}

TEST_SUITE("helios::ecs::TryAddBundleCmd") {
  TEST_CASE("ecs::TryAddBundleCmd::Execute") {
    SUBCASE("Adds only missing components from bundle") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{7.0F, 7.0F});

      TryAddBundleCmd cmd(
          entity, MotionBundle{Position{0.0F, 0.0F}, Velocity{3.0F, 4.0F}});
      cmd.Execute(world);

      CHECK_EQ(world.ReadComponent<Position>(entity).x, doctest::Approx(7.0F));
      CHECK_EQ(world.ReadComponent<Velocity>(entity).dx, doctest::Approx(3.0F));
    }
  }
}

TEST_SUITE("helios::ecs::RemoveComponentsCmd") {
  TEST_CASE("ecs::RemoveComponentsCmd::Execute") {
    SUBCASE("Removes multiple existing components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{});

      RemoveComponentsCmd<Position, Velocity> cmd(entity);
      cmd.Execute(world);

      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
    }
  }
}

TEST_SUITE("helios::ecs::RemoveBundleCmd") {
  TEST_CASE("ecs::RemoveBundleCmd::Execute") {
    SUBCASE("Removes every component represented by a nested bundle") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{}, Health{});

      RemoveBundleCmd<GameplayBundle> cmd(entity);
      cmd.Execute(world);

      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
      CHECK_FALSE(world.HasComponent<Health>(entity));
    }
  }
}

TEST_SUITE("helios::ecs::TryRemoveComponentsCmd") {
  TEST_CASE("ecs::TryRemoveComponentsCmd::Execute") {
    SUBCASE("Removes only present components") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});

      TryRemoveComponentsCmd<Position, Velocity> cmd(entity);
      cmd.Execute(world);

      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
    }
  }
}

TEST_SUITE("helios::ecs::TryRemoveBundleCmd") {
  TEST_CASE("ecs::TryRemoveBundleCmd::Execute") {
    SUBCASE("Removes only present components represented by bundle") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{});

      TryRemoveBundleCmd<MotionBundle> cmd(entity);
      cmd.Execute(world);

      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
    }
  }
}

TEST_SUITE("helios::ecs::ClearComponentsCmd") {
  TEST_CASE("ecs::ClearComponentsCmd::Execute") {
    SUBCASE("Removes all components from entity") {
      World world;
      const Entity entity = world.CreateEntity();
      world.AddComponents(entity, Position{}, Velocity{}, Health{});

      ClearComponentsCmd cmd(entity);
      cmd.Execute(world);

      CHECK_FALSE(world.HasComponent<Position>(entity));
      CHECK_FALSE(world.HasComponent<Velocity>(entity));
      CHECK_FALSE(world.HasComponent<Health>(entity));
    }

    SUBCASE("No-op when entity has no components") {
      World world;
      const Entity entity = world.CreateEntity();

      ClearComponentsCmd cmd{entity};
      cmd.Execute(world);

      CHECK_FALSE(world.HasComponent<Position>(entity));
    }
  }
}

TEST_SUITE("helios::ecs::InsertResourceCmd") {
  TEST_CASE("ecs::InsertResourceCmd::Execute") {
    SUBCASE("Inserts resource into world") {
      World world;
      Score score{42};

      InsertResourceCmd cmd(std::move(score));
      cmd.Execute(world);

      CHECK_EQ(world.ReadResource<Score>().value, 42);
    }

    SUBCASE("Replaces existing resource") {
      World world;
      world.InsertResources(Score{1});

      InsertResourceCmd cmd(Score{99});
      cmd.Execute(world);

      CHECK_EQ(world.ReadResource<Score>().value, 99);
    }
  }
}

TEST_SUITE("helios::ecs::TryInsertResourceCmd") {
  TEST_CASE("ecs::TryInsertResourceCmd::Execute") {
    SUBCASE("Inserts resource when absent") {
      World world;

      TryInsertResourceCmd cmd(Score{10});
      cmd.Execute(world);

      CHECK_EQ(world.ReadResource<Score>().value, 10);
    }

    SUBCASE("Does not overwrite existing resource") {
      World world;
      world.InsertResources(Score{5});

      TryInsertResourceCmd cmd(Score{99});
      cmd.Execute(world);

      CHECK_EQ(world.ReadResource<Score>().value, 5);
    }
  }
}

TEST_SUITE("helios::ecs::RemoveResourceCmd") {
  TEST_CASE("ecs::RemoveResourceCmd::Execute") {
    SUBCASE("Removes existing resource") {
      World world;
      world.InsertResources(Score{1});

      RemoveResourceCmd<Score>::Execute(world);

      CHECK_FALSE(world.HasResource<Score>());
    }
  }
}

TEST_SUITE("helios::ecs::TryRemoveResourceCmd") {
  TEST_CASE("ecs::TryRemoveResourceCmd::Execute") {
    SUBCASE("Removes resource when present") {
      World world;
      world.InsertResources(Score{1});

      TryRemoveResourceCmd<Score>::Execute(world);

      CHECK_FALSE(world.HasResource<Score>());
    }

    SUBCASE("Does not assert when resource is absent") {
      World world;
      TryRemoveResourceCmd<Score>::Execute(world);
      CHECK_FALSE(world.HasResource<Score>());
    }
  }
}
