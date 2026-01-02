#include "test_module.hpp"

#include <helios/core/app/app.hpp>
#include <helios/core/app/module.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/core.hpp>
#include <helios/core/logger.hpp>

namespace helios::test {

void TestSystem::Update(app::SystemContext& ctx) {
  auto& resource = ctx.WriteResource<TestResource>();
  ++resource.counter;
}

void TestModule::Build(app::App& app) {
  HELIOS_INFO("TestModule::Build() called");

  TestResource resource;
  resource.initialized = true;
  resource.counter = 42;

  app.InsertResource(std::move(resource));
  app.AddSystem<TestSystem>(app::kUpdate);
}

void TestModule::Destroy(app::App& /*app*/) {
  HELIOS_INFO("TestModule::Destroy() called");

  // Resource cleanup is handled automatically by the app
  // This is just to demonstrate the Destroy lifecycle
}

}  // namespace helios::test

// Export symbols for dynamic loading
extern "C" {

HELIOS_EXPORT helios::app::Module* helios_create_module() {
  return new helios::test::TestModule();
}

HELIOS_EXPORT helios::app::ModuleTypeId helios_module_id() noexcept {
  return helios::app::ModuleTypeIdOf<helios::test::TestModule>();
}

HELIOS_EXPORT const char* helios_module_name() noexcept {
  return "TestModule";
}

}  // extern "C"
