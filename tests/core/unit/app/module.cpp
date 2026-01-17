#include <doctest/doctest.h>

#include <helios/core/app/app.hpp>
#include <helios/core/app/module.hpp>

#include <string_view>

using namespace helios::app;

namespace {

struct BasicModule final : public Module {
  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}
};

struct ConfigurableModule final : public Module {
  int config_value = 0;
  float multiplier = 1.0F;

  ConfigurableModule() = default;
  explicit ConfigurableModule(int value) : config_value(value) {}
  ConfigurableModule(int value, float mult) : config_value(value), multiplier(mult) {}

  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}
};

struct NonDefaultConstructibleModule final : public Module {
  int value_ = 0;

  explicit NonDefaultConstructibleModule(int value) : value_(value) {}

  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}
};

struct AnotherNonDefaultModule final : public Module {
  std::string_view name_;
  int priority_ = 0;

  AnotherNonDefaultModule(std::string_view name, int priority) : name_(name), priority_(priority) {}

  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}
};

struct ModuleWithReadyAndFinish final : public Module {
  mutable bool is_ready_called = false;
  bool finish_called = false;
  bool should_be_ready = true;

  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}

  [[nodiscard]] bool IsReady(const App& /*app*/) const noexcept override {
    is_ready_called = true;
    return should_be_ready;
  }

  void Finish(App& /*app*/) override { finish_called = true; }
};

struct AsyncReadyModule final : public Module {
  mutable int ready_check_count = 0;
  int ready_after_checks = 3;
  bool finish_called = false;

  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}

  [[nodiscard]] bool IsReady(const App& /*app*/) const noexcept override {
    ++ready_check_count;
    return ready_check_count >= ready_after_checks;
  }

  void Finish(App& /*app*/) override { finish_called = true; }
};

struct NamedModule final : public Module {
  static constexpr std::string_view GetName() noexcept { return "CustomModuleName"; }

  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}
};

struct AnotherModule final : public Module {
  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}
};

struct NotAModule {
  void Build(App& /*app*/) {}
  void Destroy(App& /*app*/) {}
};

}  // namespace

TEST_SUITE("app::Module") {
  TEST_CASE("ModuleTrait: Valid Module") {
    CHECK(ModuleTrait<BasicModule>);
    CHECK(ModuleTrait<NamedModule>);
    CHECK(ModuleTrait<AnotherModule>);
    CHECK(ModuleTrait<ConfigurableModule>);
    CHECK(ModuleTrait<NonDefaultConstructibleModule>);
    CHECK(ModuleTrait<AnotherNonDefaultModule>);
  }

  TEST_CASE("ModuleTrait: Invalid Module Types") {
    CHECK_FALSE(ModuleTrait<NotAModule>);
    CHECK_FALSE(ModuleTrait<int>);
  }

  TEST_CASE("DefaultConstructibleModuleTrait: Valid Modules") {
    CHECK(DefaultConstructibleModuleTrait<BasicModule>);
    CHECK(DefaultConstructibleModuleTrait<NamedModule>);
    CHECK(DefaultConstructibleModuleTrait<AnotherModule>);
    CHECK(DefaultConstructibleModuleTrait<ConfigurableModule>);
  }

  TEST_CASE("DefaultConstructibleModuleTrait: Non-Default Constructible Modules") {
    CHECK_FALSE(DefaultConstructibleModuleTrait<NonDefaultConstructibleModule>);
    CHECK_FALSE(DefaultConstructibleModuleTrait<AnotherNonDefaultModule>);
  }

  TEST_CASE("ModuleWithNameTrait: Valid Named Module") {
    CHECK(ModuleWithNameTrait<NamedModule>);
  }

  TEST_CASE("ModuleWithNameTrait: Modules Without Name") {
    CHECK_FALSE(ModuleWithNameTrait<BasicModule>);
    CHECK_FALSE(ModuleWithNameTrait<AnotherModule>);
  }

  TEST_CASE("ModuleWithNameTrait: Invalid Types") {
    CHECK_FALSE(ModuleWithNameTrait<NotAModule>);
    CHECK_FALSE(ModuleWithNameTrait<int>);
  }

  TEST_CASE("ModuleTypeIdOf: Returns Unique IDs") {
    const ModuleTypeId id1 = ModuleTypeIdOf<BasicModule>();
    const ModuleTypeId id2 = ModuleTypeIdOf<NamedModule>();
    const ModuleTypeId id3 = ModuleTypeIdOf<AnotherModule>();

    CHECK_NE(id1, id2);
    CHECK_NE(id2, id3);
    CHECK_NE(id1, id3);
  }

  TEST_CASE("ModuleTypeIdOf: Returns Consistent IDs") {
    const ModuleTypeId id1 = ModuleTypeIdOf<BasicModule>();
    const ModuleTypeId id2 = ModuleTypeIdOf<BasicModule>();

    CHECK_EQ(id1, id2);
  }

  TEST_CASE("ModuleNameOf: Returns Custom Name For Named Module") {
    const std::string_view name = ModuleNameOf<NamedModule>();
    CHECK_EQ(name, "CustomModuleName");
  }

  TEST_CASE("ModuleNameOf: Returns Type Name For Unnamed Module") {
    const std::string_view name = ModuleNameOf<BasicModule>();
    CHECK_FALSE(name.empty());
  }

  TEST_CASE("ModuleNameOf: Different Modules Have Different Names") {
    const std::string_view name1 = ModuleNameOf<BasicModule>();
    const std::string_view name2 = ModuleNameOf<NamedModule>();
    const std::string_view name3 = ModuleNameOf<AnotherModule>();

    CHECK_NE(name1, name2);
    CHECK_NE(name2, name3);
    CHECK_NE(name1, name3);
  }

  TEST_CASE("Module:: Virtual Destructor") {
    Module* module = new BasicModule();
    delete module;
  }

  TEST_CASE("Module:: Build And Destroy Interface") {
    BasicModule module;
    App app;

    module.Build(app);
    module.Destroy(app);
  }

  TEST_CASE("Module:: Default IsReady Returns True") {
    BasicModule module;
    App app;

    CHECK(module.IsReady(app));
  }

  TEST_CASE("Module:: Default Finish Does Nothing") {
    BasicModule module;
    App app;

    // Should not throw or crash
    module.Finish(app);
  }

  TEST_CASE("Module:: Custom IsReady And Finish") {
    ModuleWithReadyAndFinish module;
    App app;

    CHECK_FALSE(module.is_ready_called);
    CHECK_FALSE(module.finish_called);

    // Call IsReady
    CHECK(module.IsReady(app));
    CHECK(module.is_ready_called);

    // Call Finish
    module.Finish(app);
    CHECK(module.finish_called);
  }

  TEST_CASE("Module:: IsReady Can Return False") {
    ModuleWithReadyAndFinish module;
    module.should_be_ready = false;
    App app;

    CHECK_FALSE(module.IsReady(app));
  }

  TEST_CASE("Module:: AsyncReady Pattern") {
    AsyncReadyModule module;
    App app;

    // Initially not ready
    CHECK_FALSE(module.IsReady(app));
    CHECK_EQ(module.ready_check_count, 1);

    CHECK_FALSE(module.IsReady(app));
    CHECK_EQ(module.ready_check_count, 2);

    // After enough checks, should be ready
    CHECK(module.IsReady(app));
    CHECK_EQ(module.ready_check_count, 3);

    // Still ready on subsequent checks
    CHECK(module.IsReady(app));
    CHECK_EQ(module.ready_check_count, 4);
  }

  TEST_CASE("App::AddModule: With Instance - Default Constructible") {
    App app;
    ConfigurableModule module(42, 2.5F);
    app.AddModule(std::move(module));

    CHECK_EQ(app.ModuleCount(), 1);
    CHECK(app.ContainsModule<ConfigurableModule>());
  }

  TEST_CASE("App::AddModule: With Instance - Non-Default Constructible") {
    App app;
    app.AddModule(NonDefaultConstructibleModule(100));

    CHECK_EQ(app.ModuleCount(), 1);
    CHECK(app.ContainsModule<NonDefaultConstructibleModule>());
  }

  TEST_CASE("App::AddModule: With Instance - Multiple Args Constructor") {
    App app;
    app.AddModule(AnotherNonDefaultModule("TestModule", 5));

    CHECK_EQ(app.ModuleCount(), 1);
    CHECK(app.ContainsModule<AnotherNonDefaultModule>());
  }

  TEST_CASE("App::AddModule: With Instance - Duplicate Detection") {
    App app;
    app.AddModule(NonDefaultConstructibleModule(1));
    app.AddModule(NonDefaultConstructibleModule(2));  // Should warn, not add

    CHECK_EQ(app.ModuleCount(), 1);
  }

  TEST_CASE("App::AddModules: With Instances - Multiple Non-Default Constructible") {
    App app;
    app.AddModules(NonDefaultConstructibleModule(42), AnotherNonDefaultModule("Test", 10));

    CHECK_EQ(app.ModuleCount(), 2);
    CHECK(app.ContainsModule<NonDefaultConstructibleModule>());
    CHECK(app.ContainsModule<AnotherNonDefaultModule>());
  }

  TEST_CASE("App::AddModules: With Instances - Mixed Constructibility") {
    App app;
    app.AddModules(ConfigurableModule(100, 3.0F), NonDefaultConstructibleModule(50));

    CHECK_EQ(app.ModuleCount(), 2);
    CHECK(app.ContainsModule<ConfigurableModule>());
    CHECK(app.ContainsModule<NonDefaultConstructibleModule>());
  }

  TEST_CASE("App::AddModule: Method Chaining With Instance") {
    App app;
    app.AddModule(NonDefaultConstructibleModule(1))
        .AddModule(AnotherNonDefaultModule("Chain", 2))
        .AddModule<BasicModule>();

    CHECK_EQ(app.ModuleCount(), 3);
    CHECK(app.ContainsModule<NonDefaultConstructibleModule>());
    CHECK(app.ContainsModule<AnotherNonDefaultModule>());
    CHECK(app.ContainsModule<BasicModule>());
  }

  TEST_CASE("App::AddModules: Method Chaining With Instances") {
    App app;
    app.AddModules(NonDefaultConstructibleModule(1), AnotherNonDefaultModule("Test", 2))
        .AddModules<BasicModule, NamedModule>();

    CHECK_EQ(app.ModuleCount(), 4);
    CHECK(app.ContainsModule<NonDefaultConstructibleModule>());
    CHECK(app.ContainsModule<AnotherNonDefaultModule>());
    CHECK(app.ContainsModule<BasicModule>());
    CHECK(app.ContainsModule<NamedModule>());
  }
}
