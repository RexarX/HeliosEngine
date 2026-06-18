#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/plugin.hpp>

#include <string_view>

using namespace helios::app;

namespace {

struct NamedTestPlugin final : public Plugin {
  static constexpr std::string_view kName = "NamedTestPlugin";

  void Build(App& /*app*/) override { ++build_count_; }
  void Finish(App& /*app*/) override { ++finish_count_; }
  void Destroy(App& /*app*/) override { ++destroy_count_; }
  void Poll(App& /*app*/) override { ++poll_count_; }

  [[nodiscard]] bool IsReady(const App& /*app*/) const noexcept override {
    return ready_;
  }

  int build_count_ = 0;
  int finish_count_ = 0;
  int destroy_count_ = 0;
  int poll_count_ = 0;
  bool ready_ = true;
};

struct UnnamedTestPlugin final : public Plugin {
  void Build(App& /*app*/) override {}
};

}  // namespace

TEST_SUITE("helios::app::PluginNameOf") {
  TEST_CASE("app::PluginNameOf") {
    SUBCASE("Returns kName when PluginWithNameTrait is satisfied") {
      CHECK_EQ(PluginNameOf<NamedTestPlugin>(), "NamedTestPlugin");
      CHECK_EQ(PluginNameOf(NamedTestPlugin{}), "NamedTestPlugin");
    }

    SUBCASE("Falls back to type name for unnamed plugins") {
      const std::string_view name = PluginNameOf<UnnamedTestPlugin>();
      CHECK_FALSE(name.empty());
    }
  }
}

TEST_SUITE("helios::app::Plugin") {
  TEST_CASE("app::Plugin::Build") {
    NamedTestPlugin plugin;
    App app;
    plugin.Build(app);
    CHECK_EQ(plugin.build_count_, 1);
  }

  TEST_CASE("app::Plugin::Finish") {
    NamedTestPlugin plugin;
    App app;
    plugin.Finish(app);
    CHECK_EQ(plugin.finish_count_, 1);
  }

  TEST_CASE("app::Plugin::Destroy") {
    NamedTestPlugin plugin;
    App app;
    plugin.Destroy(app);
    CHECK_EQ(plugin.destroy_count_, 1);
  }

  TEST_CASE("app::Plugin::Poll") {
    NamedTestPlugin plugin;
    App app;
    plugin.Poll(app);
    CHECK_EQ(plugin.poll_count_, 1);
  }

  TEST_CASE("app::Plugin::IsReady") {
    SUBCASE("Default implementation returns true") {
      UnnamedTestPlugin plugin;
      App app;
      CHECK(plugin.IsReady(app));
    }

    SUBCASE("Override can return false") {
      NamedTestPlugin plugin;
      plugin.ready_ = false;
      App app;
      CHECK_FALSE(plugin.IsReady(app));
    }
  }
}
