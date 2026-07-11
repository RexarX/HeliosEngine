#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/app/plugin_group.hpp>

#include <string_view>

using namespace helios::app;

namespace {

struct ValueResource {
  int value = 0;
};

struct ValuePlugin final : public Plugin {
  static constexpr std::string_view kName = "ValuePlugin";

  explicit constexpr ValuePlugin(int value = 1) noexcept : value_(value) {}

  void Build(App& app) override { app.InsertResources(ValueResource{value_}); }

  int value_ = 1;
};

struct SecondValuePlugin final : public Plugin {
  void Build(App& app) override { app.InsertResources(ValueResource{20}); }
};

struct ValuePluginGroup final : public PluginGroup {
  ValuePluginGroup() : PluginGroup(ValuePlugin{}) {}
};

struct SecondValuePluginGroup final : public PluginGroup {
  SecondValuePluginGroup() { Add(SecondValuePlugin{}); }
};

struct CombinedValuePluginGroup final : public PluginGroup {
  CombinedValuePluginGroup()
      : PluginGroup(ValuePlugin{}, SecondValuePlugin{}) {}
};

struct EmptyPluginGroup final : public PluginGroup {};

struct NonPluginGroup {};

}  // namespace

TEST_SUITE("helios::app::PluginGroup") {
  TEST_CASE("app::PluginGroup::Configure") {
    SUBCASE("Configured plugin is added instead of default plugin") {
      App app;
      app.AddPluginGroups(ValuePluginGroup{}.Configure(ValuePlugin{5}));

      CHECK(app.HasPlugins<ValuePlugin>());
      app.Initialize();
      CHECK_EQ(app.GetWorld().ReadResource<ValueResource>().value, 5);
    }

    SUBCASE("Repeated Configure keeps latest plugin instance") {
      App app;
      app.AddPluginGroups(ValuePluginGroup{}
                              .Configure(ValuePlugin{3})
                              .Configure(ValuePlugin{7}));

      app.Initialize();
      CHECK_EQ(app.GetWorld().ReadResource<ValueResource>().value, 7);
    }
  }

  TEST_CASE("app::PluginGroup::Disable") {
    SUBCASE("Disabled plugin is not added") {
      App app;
      app.AddPluginGroups(ValuePluginGroup{}.Disable<ValuePlugin>());

      CHECK_FALSE(app.HasPlugins<ValuePlugin>());
    }

    SUBCASE("Disable removes configured plugin") {
      App app;
      app.AddPluginGroups(
          ValuePluginGroup{}.Configure(ValuePlugin{9}).Disable<ValuePlugin>());

      CHECK_FALSE(app.HasPlugins<ValuePlugin>());
    }
  }

  TEST_CASE("app::PluginGroup::Build") {
    SUBCASE("Default plugin is added when no override exists") {
      App app;
      app.AddPluginGroups(ValuePluginGroup{});

      CHECK(app.HasPlugins<ValuePlugin>());
      app.Initialize();
      CHECK_EQ(app.GetWorld().ReadResource<ValueResource>().value, 1);
    }

    SUBCASE("Variadic constructor adds multiple plugins") {
      App app;
      app.AddPluginGroups(CombinedValuePluginGroup{});
      const auto has = app.HasPlugins<ValuePlugin, SecondValuePlugin>();

      CHECK(has[0]);
      CHECK(has[1]);
    }
  }
}

TEST_SUITE("helios::app::PluginGroupTrait") {
  TEST_CASE("app::PluginGroupTrait: concept validation") {
    SUBCASE("Concrete mutable derived types satisfy trait") {
      CHECK(PluginGroupTrait<ValuePluginGroup>);
      CHECK(PluginGroupTrait<ValuePluginGroup&>);
      CHECK(PluginGroupTrait<const ValuePluginGroup&>);
      CHECK(PluginGroupTrait<EmptyPluginGroup>);
    }

    SUBCASE("Base, and unrelated types do not satisfy trait") {
      CHECK_FALSE(PluginGroupTrait<PluginGroup>);
      CHECK_FALSE(PluginGroupTrait<NonPluginGroup>);
    }
  }
}
