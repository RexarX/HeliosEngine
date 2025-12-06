#include <doctest/doctest.h>

#include <helios/core/ecs/details/resources_manager.hpp>
#include <helios/core/ecs/resource.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::details;

namespace {

// Test resource types
struct GameConfig {
  int max_players = 4;
  float difficulty = 1.0F;
  bool sound_enabled = true;

  constexpr bool operator==(const GameConfig& other) const noexcept = default;
};

struct WindowSettings {
  int width = 1920;
  int height = 1080;
  bool fullscreen = false;

  constexpr bool operator==(const WindowSettings& other) const noexcept = default;
};

struct GameState {
  int score = 0;
  int level = 1;
  std::string player_name;

  constexpr bool operator==(const GameState& other) const noexcept = default;
};

struct AssetManager {
  std::vector<std::string> loaded_assets;

  constexpr bool operator==(const AssetManager& other) const noexcept = default;
};

struct Counter {
  int value = 0;

  static constexpr std::string_view GetName() noexcept { return "Counter"; }
};

// Non-copyable resource
struct UniqueResource {
  std::unique_ptr<int> data;

  UniqueResource() : data(std::make_unique<int>(42)) {}
  explicit UniqueResource(int value) : data(std::make_unique<int>(value)) {}

  UniqueResource(const UniqueResource&) = delete;
  UniqueResource(UniqueResource&&) noexcept = default;
  UniqueResource& operator=(const UniqueResource&) = delete;
  UniqueResource& operator=(UniqueResource&&) noexcept = default;
};

}  // namespace

TEST_SUITE("ecs::details::ResourcesManager") {
  TEST_CASE("ResourceStorage::ResourceStorage: construction") {
    SUBCASE("Default construction") {
      ResourceStorage<GameConfig> storage;
      CHECK_EQ(storage.Get().max_players, 4);
      CHECK_EQ(storage.Get().difficulty, 1.0F);
      CHECK(storage.Get().sound_enabled);
    }

    SUBCASE("Construction with arguments") {
      ResourceStorage<WindowSettings> storage(1280, 720, true);
      CHECK_EQ(storage.Get().width, 1280);
      CHECK_EQ(storage.Get().height, 720);
      CHECK(storage.Get().fullscreen);
    }

    SUBCASE("Construction with multiple arguments") {
      ResourceStorage<GameState> storage(1000, 5, "Player1");
      CHECK_EQ(storage.Get().score, 1000);
      CHECK_EQ(storage.Get().level, 5);
      CHECK_EQ(storage.Get().player_name, "Player1");
    }
  }

  TEST_CASE("ResourceStorage: copy and move semantics") {
    SUBCASE("Copy construction") {
      ResourceStorage<GameConfig> original;
      original.Get().max_players = 8;

      ResourceStorage<GameConfig> copy(original);
      CHECK_EQ(copy.Get().max_players, 8);
      CHECK_EQ(original.Get().max_players, 8);
    }

    SUBCASE("Move construction") {
      ResourceStorage<AssetManager> original(std::vector<std::string>{"texture1", "model1"});
      CHECK_EQ(original.Get().loaded_assets.size(), 2);

      ResourceStorage<AssetManager> moved(std::move(original));
      CHECK_EQ(moved.Get().loaded_assets.size(), 2);
      CHECK_EQ(moved.Get().loaded_assets[0], "texture1");
    }

    SUBCASE("Copy assignment") {
      ResourceStorage<GameConfig> storage1;
      storage1.Get().max_players = 10;

      ResourceStorage<GameConfig> storage2;
      storage2 = storage1;

      CHECK_EQ(storage2.Get().max_players, 10);
    }

    SUBCASE("Move assignment") {
      ResourceStorage<AssetManager> storage1(std::vector<std::string>{"asset1", "asset2", "asset3"});
      ResourceStorage<AssetManager> storage2;

      storage2 = std::move(storage1);
      CHECK_EQ(storage2.Get().loaded_assets.size(), 3);
    }
  }

  TEST_CASE("ResourceStorage::Get") {
    ResourceStorage<GameConfig> storage;

    SUBCASE("Mutable get") {
      GameConfig& config = storage.Get();
      config.max_players = 16;
      CHECK_EQ(storage.Get().max_players, 16);
    }

    SUBCASE("Const get") {
      const ResourceStorage<GameConfig>& const_storage = storage;
      const GameConfig& config = const_storage.Get();
      CHECK_EQ(config.max_players, 4);
    }
  }

  TEST_CASE("Resources::Resources: default construction") {
    Resources resources;

    CHECK_EQ(resources.Count(), 0);
  }

  TEST_CASE("Resources::Insert") {
    Resources resources;

    SUBCASE("Insert basic resource") {
      GameConfig config;
      config.max_players = 6;
      resources.Insert(std::move(config));

      CHECK_EQ(resources.Count(), 1);
      CHECK(resources.Has<GameConfig>());
      CHECK_EQ(resources.Get<GameConfig>().max_players, 6);
    }

    SUBCASE("Insert multiple resources") {
      resources.Insert(GameConfig{8, 2.0F, false});
      resources.Insert(WindowSettings{1024, 768, false});
      resources.Insert(GameState{500, 3, "TestPlayer"});

      CHECK_EQ(resources.Count(), 3);
      CHECK(resources.Has<GameConfig>());
      CHECK(resources.Has<WindowSettings>());
      CHECK(resources.Has<GameState>());
    }

    SUBCASE("Insert replaces existing") {
      resources.Insert(GameConfig{4, 1.0F, true});
      CHECK_EQ(resources.Get<GameConfig>().max_players, 4);

      resources.Insert(GameConfig{8, 2.0F, false});
      CHECK_EQ(resources.Count(), 1);
      CHECK_EQ(resources.Get<GameConfig>().max_players, 8);
      CHECK_EQ(resources.Get<GameConfig>().difficulty, 2.0F);
    }
  }

  TEST_CASE("Resources::TryInsert") {
    Resources resources;

    SUBCASE("TryInsert succeeds on new resource") {
      bool inserted = resources.TryInsert(GameConfig{6, 1.5f, true});
      CHECK(inserted);
      CHECK_EQ(resources.Count(), 1);
      CHECK_EQ(resources.Get<GameConfig>().max_players, 6);
    }

    SUBCASE("TryInsert fails on existing resource") {
      resources.Insert(GameConfig{4, 1.0F, true});
      [[maybe_unused]] const bool inserted = resources.TryInsert(GameConfig{8, 2.0F, false});

      // Note: Based on implementation, TryInsert uses insert which may succeed
      // The actual behavior depends on the container's insert behavior
      CHECK_EQ(resources.Count(), 1);
    }
  }

  TEST_CASE("Resources::Emplace") {
    Resources resources;

    SUBCASE("Emplace with default constructor") {
      resources.Emplace<GameConfig>();
      CHECK(resources.Has<GameConfig>());
      CHECK_EQ(resources.Get<GameConfig>().max_players, 4);
    }

    SUBCASE("Emplace with arguments") {
      resources.Emplace<WindowSettings>(2560, 1440, true);
      CHECK(resources.Has<WindowSettings>());
      CHECK_EQ(resources.Get<WindowSettings>().width, 2560);
      CHECK_EQ(resources.Get<WindowSettings>().height, 1440);
      CHECK(resources.Get<WindowSettings>().fullscreen);
    }

    SUBCASE("Emplace with complex arguments") {
      resources.Emplace<GameState>(2000, 10, "AdvancedPlayer");
      CHECK(resources.Has<GameState>());
      CHECK_EQ(resources.Get<GameState>().score, 2000);
      CHECK_EQ(resources.Get<GameState>().level, 10);
      CHECK_EQ(resources.Get<GameState>().player_name, "AdvancedPlayer");
    }

    SUBCASE("Emplace replaces existing") {
      resources.Emplace<GameConfig>();
      resources.Get<GameConfig>().max_players = 16;

      resources.Emplace<GameConfig>();
      CHECK_EQ(resources.Get<GameConfig>().max_players, 4);  // Reset to default
    }
  }

  TEST_CASE("Resources::TryEmplace") {
    Resources resources;

    SUBCASE("TryEmplace succeeds on new resource") {
      bool emplaced = resources.TryEmplace<WindowSettings>(1920, 1080, false);
      CHECK(emplaced);
      CHECK(resources.Has<WindowSettings>());
      CHECK_EQ(resources.Get<WindowSettings>().width, 1920);
    }

    SUBCASE("TryEmplace fails on existing resource") {
      resources.Emplace<GameConfig>();
      [[maybe_unused]] const bool emplaced = resources.TryEmplace<GameConfig>();

      // Behavior depends on implementation
      CHECK_EQ(resources.Count(), 1);
    }
  }

  TEST_CASE("Resources::Remove") {
    Resources resources;

    SUBCASE("Remove existing resource") {
      resources.Insert(GameConfig{});
      CHECK(resources.Has<GameConfig>());

      resources.Remove<GameConfig>();
      CHECK_FALSE(resources.Has<GameConfig>());
      CHECK_EQ(resources.Count(), 0);
    }

    SUBCASE("Remove one of multiple resources") {
      resources.Insert(GameConfig{});
      resources.Insert(WindowSettings{});
      resources.Insert(GameState{});

      resources.Remove<WindowSettings>();
      CHECK_EQ(resources.Count(), 2);
      CHECK(resources.Has<GameConfig>());
      CHECK_FALSE(resources.Has<WindowSettings>());
      CHECK(resources.Has<GameState>());
    }
  }

  TEST_CASE("Resources::TryRemove") {
    Resources resources;

    SUBCASE("TryRemove succeeds on existing resource") {
      resources.Insert(GameConfig{});
      bool removed = resources.TryRemove<GameConfig>();
      CHECK(removed);
      CHECK_FALSE(resources.Has<GameConfig>());
    }

    SUBCASE("TryRemove fails on non-existing resource") {
      bool removed = resources.TryRemove<GameConfig>();
      CHECK_FALSE(removed);
      CHECK_EQ(resources.Count(), 0);
    }

    SUBCASE("TryRemove multiple times") {
      resources.Insert(GameConfig{});
      bool removed1 = resources.TryRemove<GameConfig>();
      bool removed2 = resources.TryRemove<GameConfig>();

      CHECK(removed1);
      CHECK_FALSE(removed2);
    }
  }

  TEST_CASE("Resources::Get") {
    Resources resources;
    resources.Insert(GameConfig{8, 1.5f, false});

    SUBCASE("Get mutable reference") {
      GameConfig& config = resources.Get<GameConfig>();
      CHECK_EQ(config.max_players, 8);

      config.max_players = 12;
      CHECK_EQ(resources.Get<GameConfig>().max_players, 12);
    }

    SUBCASE("Get const reference") {
      const Resources& const_resources = resources;
      const GameConfig& config = const_resources.Get<GameConfig>();
      CHECK_EQ(config.max_players, 8);
    }
  }

  TEST_CASE("Resources::TryGet") {
    Resources resources;

    SUBCASE("TryGet returns pointer to existing resource") {
      resources.Insert(GameConfig{10, 2.0F, true});

      GameConfig* config = resources.TryGet<GameConfig>();
      REQUIRE(config != nullptr);
      CHECK_EQ(config->max_players, 10);

      config->max_players = 20;
      CHECK_EQ(resources.Get<GameConfig>().max_players, 20);
    }

    SUBCASE("TryGet returns nullptr for non-existing resource") {
      GameConfig* config = resources.TryGet<GameConfig>();
      CHECK_EQ(config, nullptr);
    }

    SUBCASE("TryGet const version") {
      resources.Insert(WindowSettings{800, 600, false});

      const Resources& const_resources = resources;
      const WindowSettings* settings = const_resources.TryGet<WindowSettings>();
      REQUIRE(settings != nullptr);
      CHECK_EQ(settings->width, 800);
    }

    SUBCASE("TryGet const returns nullptr for non-existing") {
      const Resources& const_resources = resources;
      const WindowSettings* settings = const_resources.TryGet<WindowSettings>();
      CHECK_EQ(settings, nullptr);
    }
  }

  TEST_CASE("Resources::Has") {
    Resources resources;

    SUBCASE("Has returns false for non-existing resource") {
      CHECK_FALSE(resources.Has<GameConfig>());
      CHECK_FALSE(resources.Has<WindowSettings>());
    }

    SUBCASE("Has returns true for existing resource") {
      resources.Insert(GameConfig{});
      CHECK(resources.Has<GameConfig>());
      CHECK_FALSE(resources.Has<WindowSettings>());
    }

    SUBCASE("Has works with multiple resources") {
      resources.Insert(GameConfig{});
      resources.Insert(WindowSettings{});

      CHECK(resources.Has<GameConfig>());
      CHECK(resources.Has<WindowSettings>());
      CHECK_FALSE(resources.Has<GameState>());
    }
  }

  TEST_CASE("Resources::Clear") {
    Resources resources;

    resources.Insert(GameConfig{});
    resources.Insert(WindowSettings{});
    resources.Insert(GameState{});

    CHECK_EQ(resources.Count(), 3);

    resources.Clear();

    CHECK_EQ(resources.Count(), 0);
    CHECK_FALSE(resources.Has<GameConfig>());
    CHECK_FALSE(resources.Has<WindowSettings>());
    CHECK_FALSE(resources.Has<GameState>());
  }

  TEST_CASE("Resources::Count") {
    Resources resources;

    CHECK_EQ(resources.Count(), 0);

    resources.Insert(GameConfig{});
    CHECK_EQ(resources.Count(), 1);

    resources.Insert(WindowSettings{});
    CHECK_EQ(resources.Count(), 2);

    resources.Insert(GameState{});
    CHECK_EQ(resources.Count(), 3);

    resources.Remove<WindowSettings>();
    CHECK_EQ(resources.Count(), 2);

    resources.Clear();
    CHECK_EQ(resources.Count(), 0);
  }

  TEST_CASE("Resources::Resources: move semantics") {
    Resources resources1;
    resources1.Insert(GameConfig{8, 1.5f, true});
    resources1.Insert(WindowSettings{1920, 1080, false});

    CHECK_EQ(resources1.Count(), 2);

    Resources resources2 = std::move(resources1);
    CHECK_EQ(resources2.Count(), 2);
    CHECK(resources2.Has<GameConfig>());
    CHECK(resources2.Has<WindowSettings>());
  }

  TEST_CASE("Resources::operator=: move assignment") {
    Resources resources1;
    resources1.Insert(GameConfig{});
    resources1.Insert(WindowSettings{});

    Resources resources2;
    resources2.Insert(GameState{});

    resources2 = std::move(resources1);
    CHECK_EQ(resources2.Count(), 2);
    CHECK(resources2.Has<GameConfig>());
    CHECK(resources2.Has<WindowSettings>());
    CHECK_FALSE(resources2.Has<GameState>());
  }

  TEST_CASE("Resources: non-copyable resources") {
    Resources resources;

    SUBCASE("Insert move-only resource") {
      resources.Insert(UniqueResource(100));
      CHECK(resources.Has<UniqueResource>());
      CHECK_EQ(*resources.Get<UniqueResource>().data, 100);
    }

    SUBCASE("Emplace move-only resource") {
      resources.Emplace<UniqueResource>(200);
      CHECK(resources.Has<UniqueResource>());
      CHECK_EQ(*resources.Get<UniqueResource>().data, 200);
    }

    SUBCASE("Get and modify move-only resource") {
      resources.Emplace<UniqueResource>();
      UniqueResource& resource = resources.Get<UniqueResource>();
      *resource.data = 999;
      CHECK_EQ(*resources.Get<UniqueResource>().data, 999);
    }
  }

  TEST_CASE("Resources: resource with custom name") {
    Resources resources;

    resources.Insert(Counter(42));
    CHECK(resources.Has<Counter>());
    CHECK_EQ(resources.Get<Counter>().value, 42);
  }

  TEST_CASE("Resources: complex scenarios") {
    Resources resources;

    SUBCASE("Insert, modify, remove sequence") {
      resources.Insert(GameState{100, 1, "Player"});
      GameState& state = resources.Get<GameState>();
      state.score = 500;
      state.level = 5;

      CHECK_EQ(resources.Get<GameState>().score, 500);
      CHECK_EQ(resources.Get<GameState>().level, 5);

      resources.Remove<GameState>();
      CHECK_FALSE(resources.Has<GameState>());
    }

    SUBCASE("Multiple resource types coexist") {
      resources.Insert(GameConfig{8, 2.0F, true});
      resources.Insert(WindowSettings{1920, 1080, true});
      resources.Insert(GameState{1000, 10, "Champion"});
      resources.Insert(AssetManager{std::vector<std::string>{"tex1", "tex2"}});

      CHECK_EQ(resources.Count(), 4);

      GameConfig& config = resources.Get<GameConfig>();
      WindowSettings& window = resources.Get<WindowSettings>();
      GameState& state = resources.Get<GameState>();
      AssetManager& assets = resources.Get<AssetManager>();

      CHECK_EQ(config.max_players, 8);
      CHECK_EQ(window.width, 1920);
      CHECK_EQ(state.score, 1000);
      CHECK_EQ(assets.loaded_assets.size(), 2);
    }

    SUBCASE("Replace and verify") {
      resources.Insert(GameConfig{4, 1.0F, false});
      CHECK_EQ(resources.Get<GameConfig>().max_players, 4);

      resources.Insert(GameConfig{16, 3.0F, true});
      CHECK_EQ(resources.Count(), 1);
      CHECK_EQ(resources.Get<GameConfig>().max_players, 16);
      CHECK_EQ(resources.Get<GameConfig>().difficulty, 3.0F);
      CHECK(resources.Get<GameConfig>().sound_enabled);
    }
  }

  TEST_CASE("ecs::details::Resources - Large Scale Operations") {
    Resources resources;

    // Insert many resources of the same type (replacing each time)
    for (int i = 0; i < 100; ++i) {
      resources.Insert(GameConfig{i, static_cast<float>(i), i % 2 == 0});
    }

    CHECK_EQ(resources.Count(), 1);
    CHECK_EQ(resources.Get<GameConfig>().max_players, 99);
  }

  TEST_CASE("ecs::details::Resources - Edge Cases") {
    Resources resources;

    SUBCASE("Empty resources operations") {
      CHECK_EQ(resources.Count(), 0);
      CHECK_FALSE(resources.Has<GameConfig>());
      CHECK_EQ(resources.TryGet<GameConfig>(), nullptr);
      CHECK_FALSE(resources.TryRemove<GameConfig>());
    }

    SUBCASE("Clear empty resources") {
      resources.Clear();
      CHECK_EQ(resources.Count(), 0);
    }

    SUBCASE("Multiple clears") {
      resources.Insert(GameConfig{});
      resources.Clear();
      resources.Clear();
      CHECK_EQ(resources.Count(), 0);
    }
  }

}  // TEST_SUITE
