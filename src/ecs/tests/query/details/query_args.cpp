#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/query/details/query_args.hpp>
#include <helios/ecs/query/details/traits.hpp>

#include <array>
#include <concepts>
#include <tuple>

using namespace helios::ecs;
using namespace helios::ecs::details;

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

struct Enemy {};

struct Tag {};

}  // namespace

TEST_SUITE("helios::ecs::With") {
  TEST_CASE("With") {
    SUBCASE("Empty With<> satisfies QueryArg") {
      CHECK(QueryArg<With<>>);
    }

    SUBCASE("With<T> with a single component satisfies QueryArg") {
      CHECK(QueryArg<With<Position>>);
    }

    SUBCASE("With<T, U> with multiple components satisfies QueryArg") {
      CHECK((QueryArg<With<Position, Velocity>>));
    }

    SUBCASE("IsWithTag detects With<> correctly") {
      CHECK(IsWithTag<With<>>::value);
      CHECK(IsWithTag<With<Position>>::value);
      CHECK_FALSE(IsWithTag<Position>::value);
      CHECK_FALSE(IsWithTag<Without<Position>>::value);
    }

    SUBCASE("kIsQueryFilter is true for With types") {
      CHECK(kIsQueryFilter<With<>>);
      CHECK(kIsQueryFilter<With<Position>>);
      CHECK(kIsQueryFilter<With<Position, Velocity>>);
    }
  }
}

TEST_SUITE("helios::ecs::Without") {
  TEST_CASE("Without") {
    SUBCASE("Empty Without<> satisfies QueryArg") {
      CHECK(QueryArg<Without<>>);
    }

    SUBCASE("Without<T> with a single component satisfies QueryArg") {
      CHECK(QueryArg<Without<Position>>);
    }

    SUBCASE("Without<T, U> with multiple components satisfies QueryArg") {
      CHECK((QueryArg<Without<Position, Velocity>>));
    }

    SUBCASE("IsWithoutTag detects Without<> correctly") {
      CHECK(IsWithoutTag<Without<>>::value);
      CHECK(IsWithoutTag<Without<Position>>::value);
      CHECK_FALSE(IsWithoutTag<Position>::value);
      CHECK_FALSE(IsWithoutTag<With<Position>>::value);
    }

    SUBCASE("kIsQueryFilter is true for Without types") {
      CHECK(kIsQueryFilter<Without<>>);
      CHECK(kIsQueryFilter<Without<Position>>);
      CHECK(kIsQueryFilter<Without<Position, Velocity>>);
    }
  }
}

TEST_SUITE("helios::ecs::QueryArg") {
  TEST_CASE("QueryArg") {
    SUBCASE("Plain component type satisfies QueryArg") {
      CHECK(QueryArg<Position>);
    }

    SUBCASE("Const-ref component access satisfies QueryArg") {
      CHECK(QueryArg<const Position&>);
    }

    SUBCASE("Mutable-ref component access satisfies QueryArg") {
      CHECK(QueryArg<Position&>);
    }

    SUBCASE("Pointer component access satisfies QueryArg") {
      CHECK(QueryArg<Position*>);
    }

    SUBCASE("Const-pointer component access satisfies QueryArg") {
      CHECK(QueryArg<const Position*>);
    }

    SUBCASE("With filter tag satisfies QueryArg") {
      CHECK(QueryArg<With<Position>>);
    }

    SUBCASE("Without filter tag satisfies QueryArg") {
      CHECK(QueryArg<Without<Position>>);
    }
  }
}

TEST_SUITE("helios::ecs::QueryArgs") {
  TEST_CASE("QueryArgs") {
    SUBCASE("Single component access satisfies QueryArgs") {
      CHECK(QueryArgs<const Position&>);
    }

    SUBCASE("Mixed component accesses satisfy QueryArgs") {
      CHECK((QueryArgs<Position&, const Velocity&>));
    }

    SUBCASE("Components with With filter satisfy QueryArgs") {
      CHECK((QueryArgs<const Position&, With<Health>>));
    }

    SUBCASE("Components with Without filter satisfy QueryArgs") {
      CHECK((QueryArgs<const Position&, Without<Enemy>>));
    }

    SUBCASE("Components with both With and Without satisfy QueryArgs") {
      CHECK((QueryArgs<const Position&, With<Health>, Without<Enemy>>));
    }

    SUBCASE("Multiple filter tags alongside components satisfy QueryArgs") {
      CHECK((
          QueryArgs<Position&, const Velocity&, With<Health>, Without<Enemy>>));
    }
  }
}

TEST_SUITE("helios::ecs::details::QueryArgSplit") {
  TEST_CASE("QueryArgSplit::Components") {
    SUBCASE("Only component args appear in Components tuple") {
      using Split = QueryArgSplit<const Position&, const Velocity&>;
      using Expected = std::tuple<const Position&, const Velocity&>;
      CHECK(std::same_as<Split::Components, Expected>);
    }

    SUBCASE("With filter is stripped from Components tuple") {
      using Split = QueryArgSplit<const Position&, With<Health>>;
      using Expected = std::tuple<const Position&>;
      CHECK(std::same_as<Split::Components, Expected>);
    }

    SUBCASE("Without filter is stripped from Components tuple") {
      using Split = QueryArgSplit<const Position&, Without<Enemy>>;
      using Expected = std::tuple<const Position&>;
      CHECK(std::same_as<Split::Components, Expected>);
    }

    SUBCASE("Both filters stripped; only components remain") {
      using Split =
          QueryArgSplit<const Position&, With<Health>, Without<Enemy>>;
      using Expected = std::tuple<const Position&>;
      CHECK(std::same_as<Split::Components, Expected>);
    }

    SUBCASE("No filters: all args become Components") {
      using Split = QueryArgSplit<Position&, const Velocity&, Health>;
      using Expected = std::tuple<Position&, const Velocity&, Health>;
      CHECK(std::same_as<Split::Components, Expected>);
    }
  }

  TEST_CASE("QueryArgSplit::kWithIndices") {
    SUBCASE("No With tag produces empty kWithIndices array") {
      using Split = QueryArgSplit<const Position&>;
      CHECK_EQ(Split::kWithIndices.size(), 0);
    }

    SUBCASE("With<T> produces one entry in kWithIndices") {
      using Split = QueryArgSplit<const Position&, With<Health>>;
      CHECK_EQ(Split::kWithIndices.size(), 1);
      CHECK_EQ(Split::kWithIndices[0], ComponentTypeIndex::From<Health>());
    }

    SUBCASE("With<T, U> produces two entries in kWithIndices") {
      using Split = QueryArgSplit<const Position&, With<Health, Velocity>>;
      CHECK_EQ(Split::kWithIndices.size(), 2);
    }

    SUBCASE("Multiple With<> tags are merged into kWithIndices") {
      using Split =
          QueryArgSplit<const Position&, With<Health>, With<Velocity>>;
      CHECK_EQ(Split::kWithIndices.size(), 2);
    }

    SUBCASE("Without tag does not affect kWithIndices") {
      using Split = QueryArgSplit<const Position&, Without<Enemy>>;
      CHECK_EQ(Split::kWithIndices.size(), 0);
    }

    SUBCASE("kWithIndices contains the correct ComponentTypeIndex") {
      using Split = QueryArgSplit<const Position&, With<Velocity>>;
      constexpr auto expected = ComponentTypeIndex::From<Velocity>();
      CHECK_EQ(Split::kWithIndices[0], expected);
    }
  }

  TEST_CASE("QueryArgSplit::kWithoutIndices") {
    SUBCASE("No Without tag produces empty kWithoutIndices array") {
      using Split = QueryArgSplit<const Position&>;
      CHECK_EQ(Split::kWithoutIndices.size(), 0);
    }

    SUBCASE("Without<T> produces one entry in kWithoutIndices") {
      using Split = QueryArgSplit<const Position&, Without<Enemy>>;
      CHECK_EQ(Split::kWithoutIndices.size(), 1);
      CHECK_EQ(Split::kWithoutIndices[0], ComponentTypeIndex::From<Enemy>());
    }

    SUBCASE("Without<T, U> produces two entries in kWithoutIndices") {
      using Split = QueryArgSplit<const Position&, Without<Enemy, Tag>>;
      CHECK_EQ(Split::kWithoutIndices.size(), 2);
    }

    SUBCASE("Multiple Without<> tags are merged into kWithoutIndices") {
      using Split =
          QueryArgSplit<const Position&, Without<Enemy>, Without<Tag>>;
      CHECK_EQ(Split::kWithoutIndices.size(), 2);
    }

    SUBCASE("With tag does not affect kWithoutIndices") {
      using Split = QueryArgSplit<const Position&, With<Health>>;
      CHECK_EQ(Split::kWithoutIndices.size(), 0);
    }

    SUBCASE("kWithoutIndices contains the correct ComponentTypeIndex") {
      using Split = QueryArgSplit<const Position&, Without<Enemy>>;
      constexpr auto expected = ComponentTypeIndex::From<Enemy>();
      CHECK_EQ(Split::kWithoutIndices[0], expected);
    }
  }

  TEST_CASE("QueryArgSplit - combined With and Without") {
    SUBCASE("With and Without are each placed in correct index arrays") {
      using Split =
          QueryArgSplit<const Position&, With<Health>, Without<Enemy>>;

      CHECK_EQ(Split::kWithIndices.size(), 1);
      CHECK_EQ(Split::kWithoutIndices.size(), 1);
      CHECK_EQ(Split::kWithIndices[0], ComponentTypeIndex::From<Health>());
      CHECK_EQ(Split::kWithoutIndices[0], ComponentTypeIndex::From<Enemy>());
    }

    SUBCASE("Components tuple is unaffected by filter tags") {
      using Split = QueryArgSplit<Position&, const Velocity&, With<Health>,
                                  Without<Enemy>>;
      using Expected = std::tuple<Position&, const Velocity&>;
      CHECK(std::same_as<Split::Components, Expected>);
      CHECK_EQ(Split::kWithIndices.size(), 1);
      CHECK_EQ(Split::kWithoutIndices.size(), 1);
    }

    SUBCASE("Arg order does not affect split correctness") {
      // Filters interleaved with components
      using Split = QueryArgSplit<With<Health>, const Position&, Without<Enemy>,
                                  const Velocity&>;
      using Expected = std::tuple<const Position&, const Velocity&>;
      CHECK(std::same_as<Split::Components, Expected>);
      CHECK_EQ(Split::kWithIndices.size(), 1);
      CHECK_EQ(Split::kWithoutIndices.size(), 1);
    }
  }
}

TEST_SUITE("helios::ecs::details::QueryTypeInfo") {
  TEST_CASE("QueryTypeInfo::kAllConst") {
    SUBCASE("All const accesses yields kAllConst == true") {
      using TI =
          QueryTypeInfo<World, std::tuple<const Position&, const Velocity&>>;
      CHECK(TI::kAllConst);
    }

    SUBCASE("Any mutable access yields kAllConst == false") {
      using TI = QueryTypeInfo<World, std::tuple<Position&, const Velocity&>>;
      CHECK_FALSE(TI::kAllConst);
    }
  }

  TEST_CASE("QueryTypeInfo::kIsConst") {
    SUBCASE("Non-const world with mutable access is not const") {
      using TI = QueryTypeInfo<World, std::tuple<Position&>>;
      CHECK_FALSE(TI::kIsConst);
    }

    SUBCASE("Const world forces kIsConst == true") {
      using TI = QueryTypeInfo<const World, std::tuple<Position&>>;
      CHECK(TI::kIsConst);
    }

    SUBCASE("Non-const world with all-const accesses yields kIsConst == true") {
      using TI = QueryTypeInfo<World, std::tuple<const Position&>>;
      CHECK(TI::kIsConst);
    }
  }

  TEST_CASE("QueryTypeInfo::ValueType") {
    SUBCASE("Single const-ref component maps to correct ValueType") {
      using TI = QueryTypeInfo<World, std::tuple<const Position&>>;
      using Expected = std::tuple<const Position&>;
      CHECK(std::same_as<TI::ValueType, Expected>);
    }

    SUBCASE("Multiple component accesses map to correct ValueType") {
      using TI = QueryTypeInfo<World, std::tuple<Position&, const Velocity&>>;
      using Expected = std::tuple<Position&, const Velocity&>;
      CHECK(std::same_as<TI::ValueType, Expected>);
    }

    SUBCASE("Value-copy access strips const in ValueType") {
      using TI = QueryTypeInfo<World, std::tuple<Position>>;
      using Expected = std::tuple<Position>;
      CHECK(std::same_as<TI::ValueType, Expected>);
    }

    SUBCASE("Pointer access is preserved in ValueType") {
      using TI = QueryTypeInfo<World, std::tuple<Position*>>;
      using Expected = std::tuple<Position*>;
      CHECK(std::same_as<TI::ValueType, Expected>);
    }

    SUBCASE("Const-pointer access is preserved in ValueType") {
      using TI = QueryTypeInfo<World, std::tuple<const Position*>>;
      using Expected = std::tuple<const Position*>;
      CHECK(std::same_as<TI::ValueType, Expected>);
    }
  }
}

TEST_SUITE("helios::ecs::details::UniqueComponentAccessFromTuple") {
  TEST_CASE("UniqueComponentAccessFromTuple") {
    SUBCASE("Single component is unique") {
      using T = UniqueComponentAccessFromTuple<std::tuple<const Position&>>;
      CHECK(T::kValue);
    }

    SUBCASE("Two different components are unique") {
      using T = UniqueComponentAccessFromTuple<
          std::tuple<const Position&, const Velocity&>>;
      CHECK(T::kValue);
    }

    SUBCASE("Same component accessed as const and mutable is not unique") {
      using T = UniqueComponentAccessFromTuple<
          std::tuple<const Position&, Position&>>;
      CHECK_FALSE(T::kValue);
    }
  }
}

TEST_SUITE("helios::ecs::details::ValidWorldComponentAccessFromTuple") {
  TEST_CASE("ValidWorldComponentAccessFromTuple") {
    SUBCASE("Non-const world with mutable access is valid") {
      using T =
          ValidWorldComponentAccessFromTuple<World, std::tuple<Position&>>;
      CHECK(T::kValue);
    }

    SUBCASE("Const world with only const accesses is valid") {
      using T = ValidWorldComponentAccessFromTuple<
          const World, std::tuple<const Position&, const Velocity&>>;
      CHECK(T::kValue);
    }

    SUBCASE("Const world with any mutable access is invalid") {
      using T = ValidWorldComponentAccessFromTuple<const World,
                                                   std::tuple<Position&>>;
      CHECK_FALSE(T::kValue);
    }
  }
}
