#include <doctest/doctest.h>

#include <helios/ecs/schedule/stage.hpp>

#include <string_view>

using namespace helios::ecs;

namespace {

struct EmptyStage {};

struct NamedStage {
  static constexpr std::string_view kName = "NamedStage";
};

struct NonEmptyStage {
  int data = 0;
};

class PolymorphicStage {
public:
  virtual ~PolymorphicStage() = default;
};

}  // namespace

TEST_SUITE("helios::ecs::StageTrait") {
  TEST_CASE("ecs::StageTrait::concept") {
    SUBCASE("Empty structs satisfy StageTrait") {
      CHECK_UNARY(StageTrait<EmptyStage>);
      CHECK_UNARY(StageTrait<NamedStage>);
    }

    SUBCASE("Non-empty structs do not satisfy StageTrait") {
      CHECK_FALSE(StageTrait<NonEmptyStage>);
    }

    SUBCASE("Polymorphic classes do not satisfy StageTrait") {
      CHECK_FALSE(StageTrait<PolymorphicStage>);
    }
  }
}

TEST_SUITE("helios::ecs::StageWithNameTrait") {
  TEST_CASE("ecs::StageWithNameTrait::concept") {
    SUBCASE("Empty struct with kName satisfies StageWithNameTrait") {
      CHECK_UNARY(StageWithNameTrait<NamedStage>);
    }

    SUBCASE("Empty struct without kName does not satisfy") {
      CHECK_FALSE(StageWithNameTrait<EmptyStage>);
    }
  }
}

TEST_SUITE("helios::ecs::StageNameOf") {
  TEST_CASE("ecs::StageNameOf::basic") {
    SUBCASE("Named stage returns its kName") {
      constexpr auto name = StageNameOf<NamedStage>();
      CHECK_EQ(name, "NamedStage");
    }

    SUBCASE("Unnamed stage returns its type name") {
      constexpr auto name = StageNameOf<EmptyStage>();
      CHECK_FALSE(name.empty());
    }

    SUBCASE("Instance overload matches type overload") {
      constexpr NamedStage stage{};

      constexpr auto name_from_type = StageNameOf<NamedStage>();
      constexpr auto name_from_instance = StageNameOf(stage);

      CHECK_EQ(name_from_type, name_from_instance);
    }
  }
}
