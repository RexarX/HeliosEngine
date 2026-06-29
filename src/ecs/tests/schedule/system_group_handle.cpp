#include <doctest/doctest.h>

#include <helios/ecs/schedule/executor/main_thread.hpp>
#include <helios/ecs/schedule/schedule.hpp>
#include "schedule/details/schedule_fixtures.hpp"

#include <array>
#include <compare>
#include <string>
#include <type_traits>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::schedule_test;

struct MovementSet {};
struct DiagnosticsSet {};

TEST_SUITE("helios::ecs::SystemGroupHandle") {
  TEST_CASE("ecs::SystemGroupHandle::ctor") {
    SUBCASE("Adding multiple systems returns a group handle") {
      Schedule schedule;

      const auto group_handle =
          schedule.Add(IncrementSystem{}, SetFlagSystem{});

      CHECK(schedule.IsDirty());
      CHECK_NE(group_handle.Id().id, 0);
    }

    SUBCASE("Move-constructed handle preserves identity") {
      Schedule schedule;
      auto source = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      const SystemSetId id = source.Id();

      const SystemGroupHandle moved(std::move(source));

      CHECK_EQ(moved.Id(), id);
    }

    SUBCASE("Copy construction and assignment are deleted") {
      static_assert(!std::is_copy_constructible_v<SystemGroupHandle>);
      static_assert(!std::is_copy_assignable_v<SystemGroupHandle>);
      static_assert(std::is_move_constructible_v<SystemGroupHandle>);
      static_assert(std::is_move_assignable_v<SystemGroupHandle>);
    }
  }

  TEST_CASE("ecs::SystemGroupHandle::operator=") {
    SUBCASE("Move assignment transfers identity") {
      Schedule schedule;
      auto source = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      auto target = schedule.Add(OrderSystemA{}, OrderSystemB{});
      const SystemSetId id = source.Id();

      target = std::move(source);

      CHECK_EQ(target.Id(), id);
    }
  }

  TEST_CASE("ecs::SystemGroupHandle::Before") {
    SUBCASE("Before with SystemId marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.Before(SystemId::From<SystemAlpha>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with system type marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.Before<SystemAlpha>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemHandle marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      auto sys = schedule.Add(ReadOnlySystem{});

      group.Before(sys);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemSetId marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.Before(SystemSetId::From<MovementSet>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("BeforeSet marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.BeforeSet<MovementSet>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemSetHandle marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      auto set = schedule.Set(MovementSet{});

      group.Before(set);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemGroupHandle marks schedule dirty") {
      Schedule schedule;
      auto first = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      const auto second = schedule.Add(OrderSystemA{}, OrderSystemB{});

      first.Before(second);

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemGroupHandle::After") {
    SUBCASE("After with SystemId marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.After(SystemId::From<SystemAlpha>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with system type marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.After<SystemAlpha>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemHandle marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      auto sys = schedule.Add(ReadOnlySystem{});

      group.After(sys);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemSetId marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.After(SystemSetId::From<MovementSet>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("AfterSet marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.AfterSet<MovementSet>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemSetHandle marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      auto set = schedule.Set(MovementSet{});

      group.After(set);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemGroupHandle marks schedule dirty") {
      Schedule schedule;
      const auto first = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      auto second = schedule.Add(OrderSystemA{}, OrderSystemB{});

      second.After(first);

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemGroupHandle::RunIf") {
    SUBCASE("RunIf with predicate marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.RunIf(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          }));

      CHECK(schedule.IsDirty());
    }

    SUBCASE("RunIf with functor marks schedule dirty") {
      struct AlwaysTrue {
        bool operator()() const { return true; }
      };

      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.RunIf(AlwaysTrue{});

      CHECK(schedule.IsDirty());
    }

    SUBCASE("RunIf with named lambda marks schedule dirty") {
      Schedule schedule;
      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      group.RunIf("GroupLambdaCondition", []() -> bool { return true; });

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemGroupHandle::InSet") {
    SUBCASE(
        "InSet with SystemSetId assigns all group members to the target set") {
      Schedule schedule;

      schedule.Add(IncrementSystem{}, SetFlagSystem{})
          .InSet(SystemSetId::From<MovementSet>());

      CHECK(schedule.Build().has_value());
    }

    SUBCASE(
        "InSet with template set type assigns all group members to the target "
        "set") {
      Schedule schedule;

      schedule.Add(IncrementSystem{}, SetFlagSystem{}).InSet<MovementSet>();

      CHECK(schedule.Build().has_value());
    }

    SUBCASE("InSet deduplicates when called twice") {
      Schedule schedule;

      schedule.Add(IncrementSystem{}, SetFlagSystem{})
          .InSet<MovementSet>()
          .InSet<MovementSet>();

      CHECK(schedule.Build().has_value());
    }

    SUBCASE("Bulk-assigned systems inherit target set run conditions") {
      Schedule schedule;

      schedule.Set<DiagnosticsSet>().RunIf(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return false;
          }));

      schedule.Add(IncrementSystem{}, SetFlagSystem{}).InSet<DiagnosticsSet>();

      const auto build_result = schedule.Build();
      REQUIRE(build_result.has_value());

      World world;
      world.InsertResources(CounterResource{0}, FlagResource{});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(world.ReadResource<CounterResource>().value, 0);
      CHECK_FALSE(world.ReadResource<FlagResource>().triggered);
    }
  }

  TEST_CASE("ecs::SystemGroupHandle::Sequence") {
    SUBCASE("Sequence marks the group as sequential") {
      Schedule schedule;

      auto group = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      group.Sequence();

      CHECK(schedule.IsDirty());
      CHECK(schedule.Build().has_value());
    }

    SUBCASE("Sequence on one batch does not order members of another batch") {
      Schedule schedule;
      int index = 0;
      int first_a = -1;
      int first_b = -1;
      int second_a = -1;
      int second_b = -1;

      auto first = schedule.Add(OrderSystemA{&first_a, &index},
                                OrderSystemB{&first_b, &index});
      first.Sequence();

      [[maybe_unused]] const auto second = schedule.Add(
          OrderSystemA{&second_a, &index}, OrderSystemB{&second_b, &index});

      const auto build_result = schedule.Build();
      REQUIRE(build_result.has_value());

      World world;
      world.InsertResources(CounterResource{});
      MainThreadExecutor executor;
      schedule.RunAndWait(world, executor);

      CHECK_EQ(first_a, 0);
      CHECK_EQ(first_b, 1);
      CHECK_GE(second_a, 0);
      CHECK_GE(second_b, 0);
      CHECK_NE(second_a, second_b);
    }
  }

  TEST_CASE("ecs::SystemGroupHandle::Done") {
    SUBCASE("Done returns reference to the parent schedule") {
      Schedule schedule;
      auto group_handle = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      Schedule& result = group_handle.Done();

      CHECK_EQ(&result, &schedule);
    }
  }

  TEST_CASE("ecs::SystemGroupHandle::Id") {
    SUBCASE("Id returns the group id") {
      Schedule schedule;

      const auto group_handle =
          schedule.Add(IncrementSystem{}, SetFlagSystem{});

      CHECK_NE(group_handle.Id().id, 0);
    }

    SUBCASE("Each variadic Add creates a distinct group id") {
      Schedule schedule;

      const auto first = schedule.Add(IncrementSystem{}, SetFlagSystem{});
      const auto second = schedule.Add(IncrementSystem{}, SetFlagSystem{});

      CHECK_NE(first.Id(), second.Id());
    }
  }
}
