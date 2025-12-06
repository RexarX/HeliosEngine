#include <doctest/doctest.h>

#include <helios/core/app/system_set.hpp>

using namespace helios::app;

namespace {

// System sets for testing traits and utilities
struct PhysicsSet {};
struct RenderSet {};
struct GameplaySet {};

struct NamedSet {
  static constexpr std::string_view GetName() noexcept { return "NamedSet"; }
};

}  // namespace

TEST_SUITE("app::SystemSet") {
  TEST_CASE("SystemSetTrait::SystemSetTrait: empty structs satisfy trait") {
    CHECK(SystemSetTrait<PhysicsSet>);
    CHECK(SystemSetTrait<RenderSet>);
    CHECK(SystemSetTrait<GameplaySet>);
  }

  TEST_CASE("SystemSetTrait::SystemSetTrait: non empty types do not satisfy trait") {
    struct NotEmpty {
      int value = 0;
    };

    CHECK_FALSE(SystemSetTrait<NotEmpty>);
  }

  TEST_CASE("SystemSetWithNameTrait::SystemSetWithNameTrait: types with GetName satisfy trait") {
    CHECK(SystemSetWithNameTrait<NamedSet>);
    CHECK(SystemSetTrait<NamedSet>);
  }

  TEST_CASE("SystemSetIdOf::SystemSetIdOf: different types have different ids") {
    const auto id_physics = SystemSetIdOf<PhysicsSet>();
    const auto id_render = SystemSetIdOf<RenderSet>();
    const auto id_gameplay = SystemSetIdOf<GameplaySet>();

    CHECK_NE(id_physics, id_render);
    CHECK_NE(id_physics, id_gameplay);
    CHECK_NE(id_render, id_gameplay);
  }

  TEST_CASE("SystemSetNameOf::SystemSetNameOf: unnamed sets use type name") {
    const auto name_physics = SystemSetNameOf<PhysicsSet>();
    const auto name_render = SystemSetNameOf<RenderSet>();

    CHECK_FALSE(name_physics.empty());
    CHECK_FALSE(name_render.empty());
    CHECK_NE(name_physics, name_render);
  }

  TEST_CASE("SystemSetNameOf::SystemSetNameOf: named set uses custom name") {
    const auto name_named = SystemSetNameOf<NamedSet>();
    CHECK_EQ(name_named, "NamedSet");
  }
}
