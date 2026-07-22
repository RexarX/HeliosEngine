#include <doctest/doctest.h>

#include <helios/ecs/component/bundle.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float x = 0.0F;
  float y = 0.0F;
};

struct Health {
  int value = 100;
};

struct MoveOnly {
  int value = 0;

  constexpr MoveOnly() = default;
  explicit constexpr MoveOnly(int initial_value) noexcept
      : value(initial_value) {}
  MoveOnly(const MoveOnly&) = delete;
  constexpr MoveOnly(MoveOnly&&) noexcept = default;

  MoveOnly& operator=(const MoveOnly&) = delete;
  constexpr MoveOnly& operator=(MoveOnly&&) noexcept = default;
};

class Polymorphic {
public:
  virtual ~Polymorphic() = default;
};

using MotionBundle = ComponentBundle<Position, Velocity>;
using NestedBundle = ComponentBundle<Health, MotionBundle>;

}  // namespace

TEST_SUITE("helios::ecs::ComponentBundleTrait") {
  TEST_CASE("ecs::ComponentBundleTrait::concept") {
    SUBCASE("Direct and nested bundles satisfy ComponentBundleTrait") {
      CHECK(ComponentBundleTrait<MotionBundle>);
      CHECK(ComponentBundleTrait<NestedBundle>);
      CHECK(ComponentBundleTrait<const NestedBundle&>);
    }

    SUBCASE("ComponentBundle is not a component") {
      CHECK_FALSE(ComponentTrait<MotionBundle>);
    }

    SUBCASE("Empty bundles do not satisfy ComponentBundleTrait") {
      CHECK_FALSE(ComponentBundleTrait<ComponentBundle<>>);
    }

    SUBCASE("Bundle elements must be unqualified owning values") {
      CHECK_FALSE(ComponentBundleTrait<ComponentBundle<const Position>>);
      CHECK_FALSE(ComponentBundleTrait<ComponentBundle<Position&>>);
      CHECK_FALSE(ComponentBundleTrait<ComponentBundle<Position[1]>>);
    }

    SUBCASE("Invalid component leaves do not satisfy ComponentBundleTrait") {
      CHECK_FALSE(ComponentBundleTrait<ComponentBundle<Polymorphic>>);
    }

    SUBCASE("Flattened component types must be unique") {
      CHECK_FALSE(ComponentBundleTrait<ComponentBundle<Position, Position>>);
      CHECK_FALSE(
          ComponentBundleTrait<
              ComponentBundle<Position, ComponentBundle<Velocity, Position>>>);
    }
  }
}

TEST_SUITE("helios::ecs::ComponentBundle") {
  TEST_CASE("ecs::ComponentBundle::ctor") {
    SUBCASE("Default constructor") {
      constexpr MotionBundle bundle;
      CHECK(ComponentBundleTrait<decltype(bundle)>);
    }

    SUBCASE("Value constructor") {
      constexpr MotionBundle bundle{Position{.x = 1.0F, .y = 2.0F},
                                    Velocity{.x = 3.0F, .y = 4.0F}};
      CHECK(ComponentBundleTrait<decltype(bundle)>);
    }

    SUBCASE("Nested bundle constructor") {
      constexpr NestedBundle bundle{
          Health{.value = 75},
          MotionBundle{Position{.x = 1.0F}, Velocity{.y = 2.0F}}};
      CHECK(ComponentBundleTrait<decltype(bundle)>);
    }

    SUBCASE("Class template argument deduction") {
      using Deduced = decltype(ComponentBundle{Position{}, Velocity{}});
      CHECK((std::same_as<Deduced, MotionBundle>));
    }

    SUBCASE("Copy constructor") {
      constexpr MotionBundle source{Position{}, Velocity{}};
      constexpr MotionBundle copy(source);
      CHECK(ComponentBundleTrait<decltype(copy)>);
    }

    SUBCASE("Move constructor supports move-only components") {
      ComponentBundle source{MoveOnly{42}};
      auto moved = std::move(source);
      CHECK(ComponentBundleTrait<decltype(moved)>);
    }

    SUBCASE("Value constructor is conditionally noexcept") {
      CHECK(noexcept(ComponentBundle{Position{}}));
    }
  }

  TEST_CASE("ecs::ComponentBundle::operator=") {
    SUBCASE("Copy assignment") {
      MotionBundle source{Position{.x = 1.0F}, Velocity{.y = 2.0F}};
      MotionBundle target;
      target = source;

      CHECK(ComponentBundleTrait<decltype(target)>);
    }

    SUBCASE("Move assignment") {
      ComponentBundle source{MoveOnly{42}};
      ComponentBundle<MoveOnly> target;
      target = std::move(source);

      CHECK(ComponentBundleTrait<decltype(target)>);
    }
  }
}
