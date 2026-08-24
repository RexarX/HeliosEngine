#include <doctest/doctest.h>

#include <helios/ecs/entity/entity.hpp>
#include <helios/sdl3/window/details/window_map.hpp>

#include <SDL3/SDL_video.h>

using namespace helios;
using namespace helios::sdl3::window;

TEST_SUITE("helios::sdl3::window::WindowMap") {
  TEST_CASE("helios::sdl3::window::WindowMap::Insert") {
    SUBCASE("Inserts mappings in sorted entity order") {
      WindowMap map;
      constexpr ecs::Entity entity_a{1, 1};
      constexpr ecs::Entity entity_b{4, 1};
      auto* window_a = reinterpret_cast<SDL_Window*>(11);
      auto* window_b = reinterpret_cast<SDL_Window*>(22);

      map.Insert(entity_b, window_b, 20);
      map.Insert(entity_a, window_a, 10);

      CHECK_EQ(map.entries.size(), 2U);
      CHECK_EQ(map.entries[0].entity, entity_a);
      CHECK_EQ(map.entries[0].window_id, 10U);
      CHECK_EQ(map.entries[1].entity, entity_b);
      CHECK_EQ(map.entries[1].window, window_b);
    }
  }

  TEST_CASE("helios::sdl3::window::WindowMap::Erase") {
    SUBCASE("Removes an existing mapping") {
      WindowMap map;
      constexpr ecs::Entity entity_a{1, 1};
      constexpr ecs::Entity entity_b{2, 1};
      map.Insert(entity_a, reinterpret_cast<SDL_Window*>(1), 1);
      map.Insert(entity_b, reinterpret_cast<SDL_Window*>(2), 2);

      CHECK(map.Erase(entity_a));
      CHECK_FALSE(map.Contains(entity_a));
      CHECK(map.Contains(entity_b));
    }

    SUBCASE("Returns false when the entity is missing") {
      WindowMap map;
      map.Insert(ecs::Entity{1, 1}, reinterpret_cast<SDL_Window*>(1), 1);
      CHECK_FALSE(map.Erase(ecs::Entity{9, 1}));
    }
  }

  TEST_CASE("helios::sdl3::window::WindowMap::TryGet") {
    SUBCASE("Finds a mapping by entity") {
      WindowMap map;
      constexpr ecs::Entity entity{3, 1};
      auto* window = reinterpret_cast<SDL_Window*>(7);
      map.Insert(entity, window, 30);

      WindowMap::Entry* entry = map.TryGet(entity);
      REQUIRE_NE(entry, nullptr);
      CHECK_EQ(entry->window, window);

      const WindowMap& view = map;
      CHECK_NE(view.TryGet(entity), nullptr);
      CHECK_EQ(view.TryGet(ecs::Entity{1, 1}), nullptr);
    }
  }

  TEST_CASE("helios::sdl3::window::WindowMap::TryGetByWindowId") {
    SUBCASE("Finds a mapping by SDL window id") {
      WindowMap map;
      constexpr ecs::Entity entity{3, 1};
      map.Insert(entity, reinterpret_cast<SDL_Window*>(7), 42);

      WindowMap::Entry* entry = map.TryGetByWindowId(42);
      REQUIRE_NE(entry, nullptr);
      CHECK_EQ(entry->entity, entity);

      const WindowMap& view = map;
      CHECK_NE(view.TryGetByWindowId(42), nullptr);
      CHECK_EQ(view.TryGetByWindowId(7), nullptr);
    }
  }

  TEST_CASE("helios::sdl3::window::WindowMap::TryGetByWindow") {
    SUBCASE("Finds a mapping by native pointer") {
      WindowMap map;
      auto* window = reinterpret_cast<SDL_Window*>(9);
      map.Insert(ecs::Entity{1, 1}, window, 1);

      CHECK_EQ(map.TryGetByWindow(window)->window_id, 1U);
      CHECK_EQ(map.TryGetByWindow(nullptr), nullptr);
      CHECK_EQ(map.TryGetByWindow(reinterpret_cast<SDL_Window*>(2)), nullptr);

      const WindowMap& view = map;
      CHECK_NE(view.TryGetByWindow(window), nullptr);
    }
  }

  TEST_CASE("helios::sdl3::window::WindowMap::Contains") {
    SUBCASE("Reports presence after insert and erase") {
      WindowMap map;
      constexpr ecs::Entity entity{8, 1};
      CHECK_FALSE(map.Contains(entity));
      map.Insert(entity, reinterpret_cast<SDL_Window*>(1), 1);
      CHECK(map.Contains(entity));
      map.Erase(entity);
      CHECK_FALSE(map.Contains(entity));
    }
  }
}
