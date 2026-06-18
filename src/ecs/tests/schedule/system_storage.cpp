#include <doctest/doctest.h>

#include <helios/ecs/schedule/system_storage.hpp>
#include <helios/ecs/system/system.hpp>

#include <string>

using namespace helios::ecs;

namespace {

struct SimpleSystem {
  void operator()() {}
};

struct NamedSystem {
  static constexpr std::string_view kName = "NamedSystem";

  void operator()() {}
};

struct CounterSystem {
  void operator()() {}

  int count = 0;
};

}  // namespace

TEST_SUITE("helios::ecs::SystemStorage") {
  TEST_CASE("ecs::SystemStorage::ctor") {
    SUBCASE("Constructing from members creates valid storage") {
      auto storage = SystemStorage::From(
          "TestSystem",
          SystemCallable([](World& /*world*/, SystemLocalData& /*data*/) {}),
          AccessPolicy{});

      CHECK_EQ(storage.name, "TestSystem");
      CHECK_NE(storage.id, SystemId{});
    }
  }

  TEST_CASE("ecs::SystemStorage::From") {
    SUBCASE("From with explicit name and callable creates valid storage") {
      auto storage = SystemStorage::From(
          "TestSystem",
          SystemCallable([](World& /*world*/, SystemLocalData& /*data*/) {}),
          AccessPolicy{});

      CHECK_EQ(storage.name, "TestSystem");
      CHECK_NE(storage.id, SystemId{});
    }

    SUBCASE("From with empty name creates a hashed id") {
      auto storage = SystemStorage::From(
          "",
          SystemCallable([](World& /*world*/, SystemLocalData& /*data*/) {}),
          AccessPolicy{});

      CHECK_EQ(storage.name, "");
      CHECK_NE(storage.id, SystemId{});
    }

    SUBCASE("From with SystemTypeIndex creates valid storage") {
      constexpr auto index = SystemTypeIndex::From<SimpleSystem>();

      auto storage = SystemStorage::From(
          "SimpleSystem", index,
          SystemCallable([](World& /*world*/, SystemLocalData& /*data*/) {}),
          AccessPolicy{});

      CHECK_EQ(storage.name, "SimpleSystem");
      CHECK_EQ(storage.id, SystemId::From(index));
    }

    SUBCASE("From with SystemTypeId creates valid storage") {
      constexpr auto type_id = SystemTypeId::From<SimpleSystem>();

      auto storage = SystemStorage::From(
          "SimpleSystem", type_id,
          SystemCallable([](World& /*world*/, SystemLocalData& /*data*/) {}),
          AccessPolicy{});

      CHECK_EQ(storage.name, "SimpleSystem");
      CHECK_EQ(storage.id, SystemId::From(type_id));
    }
  }

  TEST_CASE("ecs::SystemStorage::FromParam") {
    SUBCASE("FromParam with a simple system deduces id and name") {
      auto storage = SystemStorage::FromParam(SimpleSystem{});

      CHECK_NE(storage.id, SystemId{});
      CHECK_FALSE(storage.name.empty());
    }

    SUBCASE("FromParam with a named system uses kName") {
      auto storage = SystemStorage::FromParam(NamedSystem{});

      CHECK_EQ(storage.name, "NamedSystem");
      CHECK_NE(storage.id, SystemId{});
    }

    SUBCASE("FromParam with counter system deduces correctly") {
      auto storage = SystemStorage::FromParam(CounterSystem{});

      CHECK_NE(storage.id, SystemId{});
      CHECK_FALSE(storage.name.empty());
    }
  }
}
