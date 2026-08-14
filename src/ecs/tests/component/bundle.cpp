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

struct Tag {};

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

using FlatMotionBundle = ComponentBundleTypes<Position, Velocity>;

struct MotionBundle {
  using ComponentTypes = ComponentBundleTypes<Position, Velocity>;

  Position position;
  Velocity velocity;

  [[nodiscard]] constexpr ComponentTypes Build() {
    return {std::move(position), std::move(velocity)};
  }
};

struct NestedBundle {
  using ComponentTypes = ComponentBundleTypes<Health, MotionBundle>;

  Health health;
  MotionBundle motion;

  [[nodiscard]] constexpr ComponentTypes Build() {
    return {std::move(health), std::move(motion)};
  }
};

struct MoveOnlyBundle {
  using ComponentTypes = ComponentBundleTypes<MoveOnly>;

  MoveOnly value;

  [[nodiscard]] constexpr ComponentTypes Build() { return {std::move(value)}; }
};

struct PolymorphicLeafBundle {
  using ComponentTypes = ComponentBundleTypes<Polymorphic>;

  [[nodiscard]] constexpr ComponentTypes Build() { return {}; }
};

}  // namespace

TEST_SUITE("helios::ecs::ComponentBundleTrait") {
  TEST_CASE("helios::ecs::ComponentBundleTrait::concept") {
    SUBCASE("ComponentBundleTypes satisfies ComponentBundleTrait") {
      CHECK(ComponentBundleTrait<FlatMotionBundle>);
      CHECK(ComponentBundleTrait<const FlatMotionBundle&>);
    }

    SUBCASE("Struct bundles with Build satisfy ComponentBundleTrait") {
      CHECK(ComponentBundleTrait<MotionBundle>);
      CHECK(ComponentBundleTrait<NestedBundle>);
      CHECK(ComponentBundleTrait<const NestedBundle&>);
    }

    SUBCASE("Bundles are not components") {
      CHECK_FALSE(ComponentTrait<FlatMotionBundle>);
      CHECK_FALSE(ComponentTrait<MotionBundle>);
    }

    SUBCASE("Empty ComponentBundleTypes do not satisfy ComponentBundleTrait") {
      CHECK_FALSE(ComponentBundleTrait<ComponentBundleTypes<>>);
    }

    SUBCASE("Bundle elements must be unqualified owning values") {
      CHECK_FALSE(ComponentBundleTrait<ComponentBundleTypes<const Position>>);
    }

    SUBCASE("Invalid component leaves do not satisfy ComponentBundleTrait") {
      CHECK_FALSE(ComponentBundleTrait<PolymorphicLeafBundle>);
    }

    SUBCASE("Flattened component types must be unique") {
      CHECK_FALSE(
          ComponentBundleTrait<ComponentBundleTypes<Position, Position>>);
      CHECK_FALSE(
          ComponentBundleTrait<ComponentBundleTypes<Position, MotionBundle>>);
    }

    SUBCASE("ComponentBundleTypes flattens nested bundle types") {
      using Leaves = details::BundleLeafTypes<NestedBundle>;
      CHECK((std::same_as<Leaves,
                          ComponentBundleTypes<Health, Position, Velocity>>));
      CHECK_EQ(details::kComponentBundleSize<NestedBundle>, 3);
    }
  }
}

TEST_SUITE("helios::ecs::ComponentBundleTypes") {
  TEST_CASE("helios::ecs::ComponentBundleTypes::ctor") {
    SUBCASE("Default constructor") {
      constexpr FlatMotionBundle bundle{};
      CHECK(ComponentBundleTrait<decltype(bundle)>);
    }

    SUBCASE("Value constructor") {
      constexpr FlatMotionBundle bundle{Position{.x = 1.0F, .y = 2.0F},
                                        Velocity{.x = 3.0F, .y = 4.0F}};
      CHECK(ComponentBundleTrait<decltype(bundle)>);
    }

    SUBCASE("Copy constructor") {
      constexpr FlatMotionBundle source{Position{}, Velocity{}};
      constexpr FlatMotionBundle copy(source);
      CHECK(ComponentBundleTrait<decltype(copy)>);
    }

    SUBCASE("Move constructor supports move-only components") {
      ComponentBundleTypes<MoveOnly> source{MoveOnly{42}};
      auto moved = std::move(source);
      CHECK(ComponentBundleTrait<decltype(moved)>);
    }
  }

  TEST_CASE("helios::ecs::ComponentBundleTypes::operator=") {
    SUBCASE("Copy assignment") {
      FlatMotionBundle source{Position{.x = 1.0F}, Velocity{.y = 2.0F}};
      FlatMotionBundle target;
      target = source;

      CHECK(ComponentBundleTrait<decltype(target)>);
    }

    SUBCASE("Move assignment") {
      ComponentBundleTypes<MoveOnly> source{MoveOnly{42}};
      ComponentBundleTypes<MoveOnly> target;
      target = std::move(source);

      CHECK(ComponentBundleTrait<decltype(target)>);
    }
  }

  TEST_CASE("helios::ecs::ComponentBundleTypes::Build") {
    SUBCASE("ApplyComponentBundle extracts flattened values from Build") {
      NestedBundle bundle{.health = {.value = 10},
                          .motion = {.position = {.x = 1.0F, .y = 2.0F},
                                     .velocity = {.x = 3.0F, .y = 4.0F}}};

      details::ApplyComponentBundle(
          std::move(bundle),
          [](Health health, Position position, Velocity velocity) {
            CHECK_EQ(health.value, 10);
            CHECK_EQ(position.x, 1.0F);
            CHECK_EQ(position.y, 2.0F);
            CHECK_EQ(velocity.x, 3.0F);
            CHECK_EQ(velocity.y, 4.0F);
          });
    }

    SUBCASE("ApplyComponentBundle extracts values from ComponentBundleTypes") {
      FlatMotionBundle bundle{Position{.x = 1.0F, .y = 2.0F},
                              Velocity{.x = 3.0F, .y = 4.0F}};

      details::ApplyComponentBundle(std::move(bundle),
                                    [](Position position, Velocity velocity) {
                                      CHECK_EQ(position.x, 1.0F);
                                      CHECK_EQ(position.y, 2.0F);
                                      CHECK_EQ(velocity.x, 3.0F);
                                      CHECK_EQ(velocity.y, 4.0F);
                                    });
    }

    SUBCASE("Build can synthesize extras not stored as fields") {
      struct TaggedMotionBundle final {
        using ComponentTypes = ComponentBundleTypes<Position, Velocity, Tag>;

        Position position;
        Velocity velocity;

        [[nodiscard]] ComponentTypes Build() && {
          return {std::move(position), std::move(velocity), Tag{}};
        }
      };

      TaggedMotionBundle bundle{.position = {.x = 1.0F},
                                .velocity = {.y = 2.0F}};
      details::ApplyComponentBundle(
          std::move(bundle), [](Position, Velocity, Tag) { CHECK(true); });
    }
  }
}
