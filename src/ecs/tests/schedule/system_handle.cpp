#include <doctest/doctest.h>

#include <helios/ecs/schedule/schedule.hpp>
#include "schedule/details/schedule_fixtures.hpp"

#include <compare>
#include <string>
#include <type_traits>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::schedule_test;

TEST_SUITE("helios::ecs::SystemHandle") {
  TEST_CASE("ecs::SystemHandle::ctor") {
    SUBCASE("Constructing with id and schedule is valid") {
      Schedule schedule;
      const auto handle = schedule.Add(IncrementSystem{});

      CHECK_GE(handle.Id().slot, 0);
    }

    SUBCASE("Move-constructed handle preserves identity") {
      Schedule schedule;
      auto source = schedule.Add(IncrementSystem{});
      const ScheduleSystemId id = source.Id();

      const SystemHandle moved(std::move(source));

      CHECK_EQ(moved.Id(), id);
    }

    SUBCASE("Copy construction and assignment are deleted") {
      static_assert(!std::is_copy_constructible_v<SystemHandle>);
      static_assert(!std::is_copy_assignable_v<SystemHandle>);
      static_assert(std::is_move_constructible_v<SystemHandle>);
      static_assert(std::is_move_assignable_v<SystemHandle>);
    }
  }

  TEST_CASE("ecs::SystemHandle::operator=") {
    SUBCASE("Move assignment transfers identity") {
      Schedule schedule;
      auto source = schedule.Add(IncrementSystem{});
      auto target = schedule.Add(ReadOnlySystem{});
      const ScheduleSystemId id = source.Id();

      target = std::move(source);

      CHECK_EQ(target.Id(), id);
    }
  }

  TEST_CASE("ecs::SystemHandle::Before") {
    SUBCASE("Before with SystemId marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.Before(SystemId::From<SystemBeta>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with system type marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.Before<SystemBeta>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemHandle marks schedule dirty") {
      Schedule schedule;
      auto first = schedule.Add(IncrementSystem{});
      auto second = schedule.Add(ReadOnlySystem{});

      first.Before(second);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemSetHandle marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});
      auto set = schedule.Set(SetOne{});

      handle.Before(set);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemGroupHandle marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});
      const auto group = schedule.Add(OrderSystemA{}, OrderSystemB{});

      handle.Before(group);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemSetId marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.Before(SystemSetId::From<SetOne>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("BeforeSet marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.BeforeSet<SetOne>();

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemHandle::After") {
    SUBCASE("After with SystemId marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.After(SystemId::From<SystemBeta>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with system type marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.After<SystemBeta>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemHandle marks schedule dirty") {
      Schedule schedule;
      auto first = schedule.Add(IncrementSystem{});
      auto second = schedule.Add(ReadOnlySystem{});

      second.After(first);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemSetId marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.After(SystemSetId::From<SetOne>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("AfterSet marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.AfterSet<SetOne>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemSetHandle marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});
      auto set = schedule.Set(SystemSetId::From<SetOne>());

      handle.After(set);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemGroupHandle marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});
      const auto group = schedule.Add(OrderSystemA{}, OrderSystemB{});

      handle.After(group);

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemHandle::RunIf") {
    SUBCASE("RunIf with predicate marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.RunIf(
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
      auto handle = schedule.Add(IncrementSystem{});

      handle.RunIf(AlwaysTrue{});

      CHECK(schedule.IsDirty());
    }

    SUBCASE("RunIf with named lambda marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.RunIf(
          "LambdaCondition",
          [](Res<CounterResource> /*counter*/) -> bool { return true; });

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemHandle::InSet") {
    SUBCASE("InSet with SystemSetId assigns the system to a set") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.InSet(SystemSetId::From<SetOne>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("InSet with template set type assigns the system to a set") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.InSet<SetOne>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("InSet deduplicates repeated set assignment") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      handle.InSet<SetOne>();
      handle.InSet<SetOne>();

      CHECK(schedule.Build().has_value());
    }
  }

  TEST_CASE("ecs::SystemHandle::Done") {
    SUBCASE("Done returns reference to the parent schedule") {
      Schedule schedule;
      auto handle = schedule.Add(IncrementSystem{});

      Schedule& result = handle.Done();

      CHECK_EQ(&result, &schedule);
    }
  }

  TEST_CASE("ecs::SystemHandle::Id") {
    SUBCASE("Id returns the id provided at construction") {
      Schedule schedule;
      const auto handle = schedule.Add(IncrementSystem{});

      const ScheduleSystemId id = handle.Id();
      CHECK_GE(id.slot, 0);
    }
  }
}
