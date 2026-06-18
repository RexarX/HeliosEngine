#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/dynamic_plugin.hpp>
#include <helios/app/plugin.hpp>

#include <filesystem>

using namespace helios::app;

namespace helios::app::test_plugins {

struct TestDynamicPlugin;

}

TEST_SUITE("helios::app::PluginTypeExport") {
  TEST_CASE("app::PluginTypeExport::From") {
    SUBCASE("From PluginTypeId reproduces hash and name") {
      constexpr auto type_id = PluginTypeId::From<int>();
      constexpr auto export_info = PluginTypeExport::From(type_id);
      CHECK_EQ(export_info.hash, type_id.Index().Hash());
      CHECK_NE(export_info.qualified_name, nullptr);
    }

    SUBCASE("From template deduces plugin type") {
      struct SamplePlugin final : public Plugin {
        void Build(App& /*app*/) override {}
      };
      constexpr auto export_info = PluginTypeExport::From<SamplePlugin>();
      CHECK_EQ(export_info.hash,
               PluginTypeId::From<SamplePlugin>().Index().Hash());
    }
  }
}

TEST_SUITE("helios::app::DynamicPluginErrorToString") {
  TEST_CASE("app::DynamicPluginErrorToString") {
    CHECK_EQ(DynamicPluginErrorToString(DynamicPluginError::kLibraryLoadFailed),
             "Failed to load dynamic library");
    CHECK_EQ(
        DynamicPluginErrorToString(DynamicPluginError::kCreateSymbolNotFound),
        "Plugin creation function not found");
    CHECK_EQ(DynamicPluginErrorToString(DynamicPluginError::kIdSymbolNotFound),
             "Plugin ID function not found");
    CHECK_EQ(
        DynamicPluginErrorToString(DynamicPluginError::kNameSymbolNotFound),
        "Plugin name function not found");
    CHECK_EQ(DynamicPluginErrorToString(DynamicPluginError::kCreateFailed),
             "Plugin creation function returned nullptr");
    CHECK_EQ(DynamicPluginErrorToString(DynamicPluginError::kReloadFailed),
             "Failed to reload plugin");
  }
}

TEST_SUITE("helios::app::DynamicPlugin") {
  TEST_CASE("app::DynamicPlugin::ctor") {
    SUBCASE("Default construction is unloaded") {
      DynamicPlugin plugin;
      CHECK_FALSE(plugin.Loaded());
      CHECK_EQ(plugin.TryGetPlugin(), nullptr);
      CHECK(plugin.GetPluginTypeId().Empty());
      CHECK(plugin.GetPluginName().empty());
      CHECK(plugin.Path().empty());
      CHECK_EQ(plugin.Config().create_symbol, kDefaultCreateSymbol);
      CHECK_EQ(plugin.Config().plugin_type_id_symbol, kDefaultPluginIdSymbol);
    }
  }

  TEST_CASE("app::DynamicPlugin::Load") {
    SUBCASE("Invalid path returns kLibraryLoadFailed") {
      DynamicPlugin plugin;
      const auto result = plugin.Load(
          std::filesystem::path{"/nonexistent/helios_test_plugin.so"});
      CHECK_FALSE(result.has_value());
      CHECK_EQ(result.error(), DynamicPluginError::kLibraryLoadFailed);
      CHECK_FALSE(plugin.Loaded());
    }

#ifdef HELIOS_TEST_PLUGIN_PATH
    SUBCASE("Valid test plugin loads successfully") {
      DynamicPlugin plugin;
      const auto result =
          plugin.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH});
      REQUIRE(result.has_value());
      CHECK(plugin.Loaded());
      CHECK_NE(plugin.TryGetPlugin(), nullptr);
      CHECK_FALSE(plugin.GetPluginTypeId().Empty());
      CHECK_EQ(plugin.GetPluginName(), "TestDynamicPlugin");
      CHECK_EQ(
          plugin.Path().filename().string(),
          std::filesystem::path{HELIOS_TEST_PLUGIN_PATH}.filename().string());

      App app;
      plugin.GetPlugin().Build(app);
      CHECK_EQ(plugin.GetPluginTypeId(),
               PluginTypeId::From<test_plugins::TestDynamicPlugin>());
    }
#endif
  }

  TEST_CASE("app::DynamicPlugin::Unload") {
#ifdef HELIOS_TEST_PLUGIN_PATH
    DynamicPlugin plugin;
    REQUIRE(plugin.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                .has_value());
    const auto result = plugin.Unload();
    CHECK(result.has_value());
    CHECK_FALSE(plugin.Loaded());
#endif
  }

  TEST_CASE("app::DynamicPlugin::GetPlugin") {
#ifdef HELIOS_TEST_PLUGIN_PATH
    DynamicPlugin plugin;
    REQUIRE(plugin.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                .has_value());
    CHECK_NE(&plugin.GetPlugin(), nullptr);
    CHECK_NE(plugin.TryGetPlugin(), nullptr);
#endif
  }

  TEST_CASE("app::DynamicPlugin::ReleasePlugin") {
#ifdef HELIOS_TEST_PLUGIN_PATH
    DynamicPlugin plugin;
    REQUIRE(plugin.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                .has_value());
    auto owned = plugin.ReleasePlugin();
    CHECK_NE(owned.get(), nullptr);
    CHECK_FALSE(plugin.Loaded());
#endif
  }

  TEST_CASE("app::DynamicPlugin::HasFileChanged") {
    SUBCASE("Returns false for unloaded plugin") {
      DynamicPlugin plugin;
      CHECK_FALSE(plugin.HasFileChanged());
    }

#ifdef HELIOS_TEST_PLUGIN_PATH
    SUBCASE("Returns false immediately after load without file changes") {
      DynamicPlugin plugin;
      REQUIRE(plugin.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                  .has_value());
      CHECK_FALSE(plugin.HasFileChanged());
    }
#endif
  }

  TEST_CASE("app::DynamicPlugin::UpdateFileTime") {
    SUBCASE("UpdateFileTime on unloaded plugin is a no-op") {
      DynamicPlugin plugin;
      plugin.UpdateFileTime();
      CHECK_FALSE(plugin.HasFileChanged());
    }

#ifdef HELIOS_TEST_PLUGIN_PATH
    SUBCASE("UpdateFileTime records baseline so HasFileChanged stays false") {
      DynamicPlugin plugin;
      REQUIRE(plugin.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                  .has_value());
      plugin.UpdateFileTime();
      CHECK_FALSE(plugin.HasFileChanged());
    }
#endif
  }

  TEST_CASE("app::DynamicPlugin::Library") {
    SUBCASE("Unloaded plugin has unloaded library handle") {
      DynamicPlugin plugin;
      CHECK_FALSE(plugin.Library().Loaded());
    }

#ifdef HELIOS_TEST_PLUGIN_PATH
    SUBCASE("Loaded plugin exposes loaded library handle") {
      DynamicPlugin plugin;
      REQUIRE(plugin.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                  .has_value());
      CHECK(plugin.Library().Loaded());
    }
#endif
  }

  TEST_CASE("app::DynamicPlugin::Config") {
    DynamicPluginConfig config{.create_symbol = "custom_create",
                               .plugin_type_id_symbol = "custom_id",
                               .auto_reload = true};
    DynamicPlugin plugin;
    CHECK_EQ(plugin.Config().create_symbol, kDefaultCreateSymbol);
    const auto result =
        plugin.Load(std::filesystem::path{"/nonexistent/plugin.so"}, config);
    CHECK_FALSE(result.has_value());
    CHECK_EQ(plugin.Config().create_symbol, "custom_create");
    CHECK_EQ(plugin.Config().plugin_type_id_symbol, "custom_id");
    CHECK(plugin.Config().auto_reload);
  }

  TEST_CASE("app::DynamicPlugin::Move construction") {
#ifdef HELIOS_TEST_PLUGIN_PATH
    DynamicPlugin source;
    REQUIRE(source.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                .has_value());
    DynamicPlugin moved{std::move(source)};
    CHECK(moved.Loaded());
    CHECK_FALSE(source.Loaded());
#endif
  }

  TEST_CASE("app::DynamicPlugin::Move assignment") {
#ifdef HELIOS_TEST_PLUGIN_PATH
    DynamicPlugin source;
    REQUIRE(source.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                .has_value());
    DynamicPlugin target;
    target = std::move(source);
    CHECK(target.Loaded());
    CHECK_FALSE(source.Loaded());
#endif
  }

  TEST_CASE("app::DynamicPlugin::ReloadIfChanged") {
#ifdef HELIOS_TEST_PLUGIN_PATH
    DynamicPlugin plugin;
    REQUIRE(plugin.Load(std::filesystem::path{HELIOS_TEST_PLUGIN_PATH})
                .has_value());
    App app;
    const auto result = plugin.ReloadIfChanged(app);
    CHECK(result.has_value());
#endif
  }
}
