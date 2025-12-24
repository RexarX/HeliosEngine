#include <helios/example/example.hpp>

#include <helios/core/app/app.hpp>
#include <helios/core/app/schedules.hpp>

namespace helios::example {

void ExampleSystem::Update(app::SystemContext& ctx) {
  auto& resource = ctx.WriteResource<ExampleResource>();
  ++resource.counter;
}

void ExampleModule::Build(app::App& app) {
  app.InsertResource(ExampleResource{});
  app.AddSystem<ExampleSystem>(app::kUpdate);
}

void ExampleModule::Destroy(app::App& /*app*/) {
  /* No cleanup needed for this simple example */
}

}  // namespace helios::example
