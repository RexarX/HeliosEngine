#include <doctest/doctest.h>

#include <helios/core/app/module.hpp>

#include <string_view>

using namespace helios::app;

namespace {

struct BasicModule final : public Module {
  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}
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

struct NonDefaultConstructibleModule final : public Module {
  explicit NonDefaultConstructibleModule(int value) : value_(value) {}

  void Build(App& /*app*/) override {}
  void Destroy(App& /*app*/) override {}

  int value_ = 0;
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
  }

  TEST_CASE("ModuleTrait: Invalid Module Types") {
    CHECK_FALSE(ModuleTrait<NotAModule>);
    CHECK_FALSE(ModuleTrait<int>);
    CHECK_FALSE(ModuleTrait<NonDefaultConstructibleModule>);
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
    App* app_ptr = nullptr;

    module.Build(*app_ptr);
    module.Destroy(*app_ptr);
  }
}
