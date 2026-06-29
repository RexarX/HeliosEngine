#include <doctest/doctest.h>

#include <helios/ecs/schedule/schedule.hpp>
#include "schedule/details/schedule_fixtures.hpp"

#include <compare>
#include <string>
#include <type_traits>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::schedule_test;

TEST_SUITE("helios::ecs::SystemSetHandle") {
  TEST_CASE("ecs::SystemSetHandle::ctor") {
    SUBCASE("Constructing with id and schedule is valid") {
      Schedule schedule;
      const auto handle = schedule.Set(SetOne{});

      CHECK_EQ(handle.Id(), SystemSetId::From(SetOne{}));
    }

    SUBCASE("Move-constructed handle preserves identity") {
      Schedule schedule;
      auto source = schedule.Set(SetOne{});
      const SystemSetId id = source.Id();

      const SystemSetHandle moved(std::move(source));

      CHECK_EQ(moved.Id(), id);
    }

    SUBCASE("Copy construction and assignment are deleted") {
      static_assert(!std::is_copy_constructible_v<SystemSetHandle>);
      static_assert(!std::is_copy_assignable_v<SystemSetHandle>);
      static_assert(std::is_move_constructible_v<SystemSetHandle>);
      static_assert(std::is_move_assignable_v<SystemSetHandle>);
    }
  }

  TEST_CASE("ecs::SystemSetHandle::operator=") {
    SUBCASE("Move assignment transfers identity") {
      Schedule schedule;
      auto source = schedule.Set(SetOne{});
      auto target = schedule.Set(SetTwo{});
      const SystemSetId id = source.Id();

      target = std::move(source);

      CHECK_EQ(target.Id(), id);
    }
  }

  TEST_CASE("ecs::SystemSetHandle::Before") {
    SUBCASE("Before with SystemId marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.Before(SystemId::From<SystemAlpha>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with system type marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.Before<SystemAlpha>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemHandle marks schedule dirty") {
      Schedule schedule;
      auto sys = schedule.Add(IncrementSystem{});
      auto set = schedule.Set(SetOne{});

      set.Before(sys);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemSetId marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.Before(SystemSetId::From<SetTwo>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("BeforeSet marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.BeforeSet<SetTwo>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemSetHandle marks schedule dirty") {
      Schedule schedule;
      auto first = schedule.Set(SetOne{});
      auto second = schedule.Set(SetTwo{});

      first.Before(second);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("Before with SystemGroupHandle marks schedule dirty") {
      Schedule schedule;
      auto set = schedule.Set(SetOne{});
      const auto group = schedule.Add(OrderSystemA{}, OrderSystemB{});

      set.Before(group);

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemSetHandle::After") {
    SUBCASE("After with SystemId marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.After(SystemId::From<SystemAlpha>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with system type marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.After<SystemAlpha>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemHandle marks schedule dirty") {
      Schedule schedule;
      auto sys = schedule.Add(IncrementSystem{});
      auto set = schedule.Set(SetOne{});

      set.After(sys);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemSetId marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.After(SystemSetId::From<SetTwo>());

      CHECK(schedule.IsDirty());
    }

    SUBCASE("AfterSet marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.AfterSet<SetTwo>();

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemSetHandle marks schedule dirty") {
      Schedule schedule;
      auto first = schedule.Set(SetOne{});
      auto second = schedule.Set(SetTwo{});

      second.After(first);

      CHECK(schedule.IsDirty());
    }

    SUBCASE("After with SystemGroupHandle marks schedule dirty") {
      Schedule schedule;
      auto set = schedule.Set(SetOne{});
      const auto group = schedule.Add(OrderSystemA{}, OrderSystemB{});

      set.After(group);

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemSetHandle::RunIf") {
    SUBCASE("RunIf with predicate marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

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
      auto handle = schedule.Set(SetOne{});

      handle.RunIf(AlwaysTrue{});

      CHECK(schedule.IsDirty());
    }

    SUBCASE("RunIf with named lambda marks schedule dirty") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.RunIf("SetLambdaCondition", []() -> bool { return true; });

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemSetHandle::Sequence") {
    SUBCASE("Sequence marks the set as sequential") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      handle.Sequence();

      CHECK(schedule.IsDirty());
    }
  }

  TEST_CASE("ecs::SystemSetHandle::Done") {
    SUBCASE("Done returns reference to the parent schedule") {
      Schedule schedule;
      auto handle = schedule.Set(SetOne{});

      Schedule& result = handle.Done();

      CHECK_EQ(&result, &schedule);
    }
  }

  TEST_CASE("ecs::SystemSetHandle::Id") {
    SUBCASE("Id returns the set id") {
      Schedule schedule;
      constexpr auto set_id = SystemSetId::From<SetOne>();

      const auto handle = schedule.Set(set_id);

      CHECK_EQ(handle.Id(), set_id);
    }
  }
}
