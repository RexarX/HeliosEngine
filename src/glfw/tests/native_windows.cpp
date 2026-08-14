#include <doctest/doctest.h>

#include <helios/glfw/details/glfw_state.hpp>

using namespace helios::ecs;
using namespace helios::glfw;

TEST_SUITE("helios::glfw::NativeWindows") {
  TEST_CASE("helios::glfw::NativeWindows::Insert") {
    SUBCASE("Inserts entries in sorted entity order") {
      NativeWindows native;
      const Entity entity_a{1, 1};
      const Entity entity_b{3, 1};
      const Entity entity_c{5, 1};

      native.Insert(entity_b, {});
      native.Insert(entity_a, {});
      native.Insert(entity_c, {});

      CHECK_EQ(native.Size(), 3U);
      CHECK_EQ(native.entries[0].entity, entity_a);
      CHECK_EQ(native.entries[1].entity, entity_b);
      CHECK_EQ(native.entries[2].entity, entity_c);
    }
  }

  TEST_CASE("helios::glfw::NativeWindows::Erase") {
    SUBCASE("Removes an existing entry") {
      NativeWindows native;
      const Entity entity_a{1, 1};
      const Entity entity_b{3, 1};
      native.Insert(entity_a, {});
      native.Insert(entity_b, {});

      CHECK(native.Erase(entity_b));
      CHECK_FALSE(native.Contains(entity_b));
      CHECK(native.Contains(entity_a));
      CHECK_EQ(native.Size(), 1U);
    }

    SUBCASE("Returns false when the entity is missing") {
      NativeWindows native;
      native.Insert(Entity{1, 1}, {});
      CHECK_FALSE(native.Erase(Entity{2, 1}));
      CHECK_EQ(native.Size(), 1U);
    }
  }

  TEST_CASE("helios::glfw::NativeWindows::TryGet") {
    SUBCASE("Returns a mutable entry when present") {
      NativeWindows native;
      const Entity entity{4, 1};
      native.Insert(entity, NativeEntry{.last_cursor_x = 3.0});

      NativeWindows::Entry* entry = native.TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      CHECK_EQ(entry->native.last_cursor_x, 3.0);
      entry->native.last_cursor_y = 9.0;
      CHECK_EQ(native.TryGet(entity)->native.last_cursor_y, 9.0);
    }

    SUBCASE("Returns a const entry when present") {
      NativeWindows native;
      const Entity entity{4, 1};
      native.Insert(entity, {});
      const NativeWindows& view = native;

      CHECK_NE(view.TryGet(entity), nullptr);
      CHECK_EQ(view.TryGet(Entity{2, 1}), nullptr);
    }

    SUBCASE("Returns nullptr when missing") {
      NativeWindows native;
      native.Insert(Entity{1, 1}, {});
      CHECK_EQ(native.TryGet(Entity{2, 1}), nullptr);
    }
  }

  TEST_CASE("helios::glfw::NativeWindows::Contains") {
    SUBCASE("Reports presence after insert and erase") {
      NativeWindows native;
      const Entity entity{7, 1};
      CHECK_FALSE(native.Contains(entity));
      native.Insert(entity, {});
      CHECK(native.Contains(entity));
      native.Erase(entity);
      CHECK_FALSE(native.Contains(entity));
    }
  }

  TEST_CASE("helios::glfw::NativeWindows::Empty") {
    SUBCASE("Default-constructed table is empty") {
      NativeWindows native;
      CHECK(native.Empty());
    }

    SUBCASE("Empty after the last entry is erased") {
      NativeWindows native;
      native.Insert(Entity{1, 1}, {});
      CHECK_FALSE(native.Empty());
      native.Erase(Entity{1, 1});
      CHECK(native.Empty());
    }
  }

  TEST_CASE("helios::glfw::NativeWindows::Size") {
    SUBCASE("Tracks the number of entries") {
      NativeWindows native;
      CHECK_EQ(native.Size(), 0U);
      native.Insert(Entity{1, 1}, {});
      native.Insert(Entity{2, 1}, {});
      CHECK_EQ(native.Size(), 2U);
    }
  }
}
