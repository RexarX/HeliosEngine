#include <doctest/doctest.h>

#include <helios/ecs/schedule/system_set.hpp>
#include <helios/ecs/system/system.hpp>

using namespace helios::ecs;

namespace {

struct SetA {};
struct SetB {};

struct NamedSet {
  static constexpr std::string_view kName = "NamedSet";
};

struct SystemX {
  void operator()() {}
};

struct SystemY {
  void operator()() {}
};

struct SystemZ {
  void operator()() {}
};

}  // namespace

TEST_SUITE("helios::ecs::SystemSetId") {
  TEST_CASE("ecs::SystemSetId::From") {
    SUBCASE("Same set type produces same id") {
      constexpr auto id1 = SystemSetId::From<SetA>();
      constexpr auto id2 = SystemSetId::From<SetA>();
      CHECK_EQ(id1, id2);
    }

    SUBCASE("Different set types produce different ids") {
      constexpr auto id1 = SystemSetId::From<SetA>();
      constexpr auto id2 = SystemSetId::From<SetB>();
      CHECK_NE(id1, id2);
    }

    SUBCASE("From instance matches From type") {
      constexpr SetA instance{};

      constexpr auto id_from_type = SystemSetId::From<SetA>();
      constexpr auto id_from_instance = SystemSetId::From(instance);

      CHECK_EQ(id_from_type, id_from_instance);
    }

    SUBCASE("From range of system ids produces consistent id") {
      constexpr std::array<SystemId, 2> ids{SystemId::From<SystemX>(),
                                            SystemId::From<SystemY>()};
      constexpr auto id1 = SystemSetId::From(ids);
      constexpr auto id2 = SystemSetId::From(ids);
      CHECK_EQ(id1, id2);
    }
  }

  TEST_CASE("ecs::SystemSetId::operator<=>") {
    SUBCASE("Equal ids compare equal") {
      constexpr auto id1 = SystemSetId::From<SetA>();
      constexpr auto id2 = SystemSetId::From<SetA>();
      CHECK_EQ(id1, id2);
    }

    SUBCASE("Different ids compare unequal") {
      constexpr auto id1 = SystemSetId::From<SetA>();
      constexpr auto id2 = SystemSetId::From<SetB>();
      CHECK_NE(id1, id2);
    }
  }
}

TEST_SUITE("helios::ecs::SystemSet") {
  TEST_CASE("ecs::SystemSet::ctor") {
    SUBCASE("Constructs with the given id") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);

      CHECK_EQ(set.Id(), set_id);
    }

    SUBCASE("Move-constructed set transfers state") {
      SystemSet source(SystemSetId::From<SetA>());
      source.Before(SystemId::From<SystemX>());

      const SystemSet moved(std::move(source));

      CHECK_EQ(moved.BeforeTargets().size(), 1);
    }
  }

  TEST_CASE("ecs::SystemSet::operator=") {
    SUBCASE("Move assignment transfers state") {
      SystemSet source(SystemSetId::From<SetA>());
      source.Before(SystemId::From<SystemX>());

      SystemSet target(SystemSetId::From<SetB>());
      target = std::move(source);

      CHECK_EQ(target.BeforeTargets().size(), 1);
    }
  }

  TEST_CASE("ecs::SystemSet::Before") {
    SUBCASE("Before with SystemId adds a target") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto system_id = SystemId::From<SystemX>();

      set.Before(system_id);

      const auto& targets = set.BeforeTargets();
      CHECK_EQ(targets.size(), 1);
      CHECK_EQ(targets[0], system_id);
    }

    SUBCASE("Before with same SystemId adds it only once") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto system_id = SystemId::From<SystemX>();

      set.Before(system_id);
      set.Before(system_id);

      CHECK_EQ(set.BeforeTargets().size(), 1);
    }

    SUBCASE("Before with SystemSetId adds a set target") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto target_set_id = SystemSetId::From<SetB>();

      set.Before(target_set_id);

      const auto& targets = set.BeforeSetTargets();
      CHECK_EQ(targets.size(), 1);
      CHECK_EQ(targets[0], target_set_id);
    }
  }

  TEST_CASE("ecs::SystemSet::After") {
    SUBCASE("After with SystemId adds a target") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto system_id = SystemId::From<SystemX>();

      set.After(system_id);

      const auto& targets = set.AfterTargets();
      CHECK_EQ(targets.size(), 1);
      CHECK_EQ(targets[0], system_id);
    }

    SUBCASE("After with same SystemId adds it only once") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto system_id = SystemId::From<SystemX>();

      set.After(system_id);
      set.After(system_id);

      CHECK_EQ(set.AfterTargets().size(), 1);
    }

    SUBCASE("After with SystemSetId adds a set target") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto target_set_id = SystemSetId::From<SetB>();

      set.After(target_set_id);

      const auto& targets = set.AfterSetTargets();
      CHECK_EQ(targets.size(), 1);
      CHECK_EQ(targets[0], target_set_id);
    }
  }

  TEST_CASE("ecs::SystemSet::RunIf") {
    SUBCASE("RunIf with a simple predicate adds a condition") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);

      set.RunIf(RunConditionStorage::From(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          })));

      CHECK_EQ(set.Conditions().size(), 1);
    }

    SUBCASE("RunIf with a functor adds a condition") {
      struct AlwaysTrue {
        bool operator()() const { return true; }
      };

      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);

      set.RunIf(AlwaysTrue{});

      CHECK_EQ(set.Conditions().size(), 1);
    }

    SUBCASE("RunIf with named lambda adds a condition") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);

      set.RunIf("LambdaCondition", []() -> bool { return true; });

      CHECK_EQ(set.Conditions().size(), 1);
    }
  }

  TEST_CASE("ecs::SystemSet::Sequence") {
    SUBCASE("Sequence marks the set as a sequence") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);

      CHECK_FALSE(set.IsSequence());
      set.Sequence();
      CHECK(set.IsSequence());
    }
  }

  TEST_CASE("ecs::SystemSet::TakeConditions") {
    SUBCASE("TakeConditions moves out all conditions") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);

      set.RunIf(RunConditionStorage::From(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          })));

      CHECK_EQ(set.Conditions().size(), 1);

      auto taken = set.TakeConditions();
      CHECK_EQ(taken.size(), 1);
      CHECK(set.Conditions().empty());
    }
  }

  TEST_CASE("ecs::SystemSet::IsSequence") {
    SUBCASE("New set is not a sequence") {
      SystemSet set(SystemSetId::From<SetA>());
      CHECK_FALSE(set.IsSequence());
    }

    SUBCASE("Set becomes a sequence after calling Sequence") {
      SystemSet set(SystemSetId::From<SetA>());
      set.Sequence();
      CHECK(set.IsSequence());
    }
  }

  TEST_CASE("ecs::SystemSet::Id") {
    SUBCASE("Id returns the set id") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);

      CHECK_EQ(set.Id(), set_id);
    }
  }

  TEST_CASE("ecs::SystemSet::BeforeTargets") {
    SUBCASE("BeforeTargets returns correct references") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto system_x = SystemId::From<SystemX>();
      constexpr auto system_y = SystemId::From<SystemY>();

      set.Before(system_x);
      set.Before(system_y);

      const auto& targets = set.BeforeTargets();
      CHECK_EQ(targets.size(), 2);
      CHECK_EQ(targets[0], system_x);
      CHECK_EQ(targets[1], system_y);
    }
  }

  TEST_CASE("ecs::SystemSet::AfterTargets") {
    SUBCASE("AfterTargets returns correct references") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto system_x = SystemId::From<SystemX>();
      constexpr auto system_z = SystemId::From<SystemZ>();

      set.After(system_x);
      set.After(system_z);

      const auto& targets = set.AfterTargets();
      CHECK_EQ(targets.size(), 2);
      CHECK_EQ(targets[0], system_x);
      CHECK_EQ(targets[1], system_z);
    }
  }

  TEST_CASE("ecs::SystemSet::BeforeSetTargets") {
    SUBCASE("BeforeSetTargets returns correct references") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto target_set = SystemSetId::From<SetB>();

      set.Before(target_set);

      const auto& targets = set.BeforeSetTargets();
      CHECK_EQ(targets.size(), 1);
      CHECK_EQ(targets[0], target_set);
    }
  }

  TEST_CASE("ecs::SystemSet::AfterSetTargets") {
    SUBCASE("AfterSetTargets returns correct references") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);
      constexpr auto target_set = SystemSetId::From<SetB>();

      set.After(target_set);

      const auto& targets = set.AfterSetTargets();
      CHECK_EQ(targets.size(), 1);
      CHECK_EQ(targets[0], target_set);
    }
  }

  TEST_CASE("ecs::SystemSet::Conditions") {
    SUBCASE("Conditions returns correct references") {
      constexpr auto set_id = SystemSetId::From<SetA>();
      SystemSet set(set_id);

      set.RunIf(RunConditionStorage::From(
          RunCondition([](World& /*world*/, SystemLocalData& /*data*/) -> bool {
            return true;
          })));

      const auto& conditions = set.Conditions();
      CHECK_EQ(conditions.size(), 1);
    }
  }
}

TEST_SUITE("helios::ecs::SystemSetTrait") {
  TEST_CASE("ecs::SystemSetTrait::concept") {
    SUBCASE("Empty structs satisfy SystemSetTrait") {
      CHECK_UNARY(SystemSetTrait<SetA>);
      CHECK_UNARY(SystemSetTrait<SetB>);
    }

    SUBCASE("Non-empty structs do not satisfy SystemSetTrait") {
      struct NonEmpty {
        int data = 0;
      };
      CHECK_FALSE(SystemSetTrait<NonEmpty>);
    }
  }
}

TEST_SUITE("helios::ecs::SystemSetWithNameTrait") {
  TEST_CASE("ecs::SystemSetWithNameTrait::concept") {
    SUBCASE("Empty struct with kName satisfies") {
      CHECK_UNARY(SystemSetWithNameTrait<NamedSet>);
    }

    SUBCASE("Empty struct without kName does not satisfy") {
      CHECK_FALSE(SystemSetWithNameTrait<SetA>);
    }
  }
}

TEST_SUITE("helios::ecs::SystemSetNameOf") {
  TEST_CASE("ecs::SystemSetNameOf::basic") {
    SUBCASE("Named set returns its kName") {
      constexpr auto name = SystemSetNameOf<NamedSet>();
      CHECK_EQ(name, "NamedSet");
    }

    SUBCASE("Unnamed set returns its type name") {
      constexpr auto name = SystemSetNameOf<SetA>();
      CHECK_FALSE(name.empty());
    }

    SUBCASE("Instance overload matches type overload") {
      constexpr NamedSet instance{};

      constexpr auto name_from_type = SystemSetNameOf<NamedSet>();
      constexpr auto name_from_instance = SystemSetNameOf(instance);

      CHECK_EQ(name_from_type, name_from_instance);
    }
  }
}
