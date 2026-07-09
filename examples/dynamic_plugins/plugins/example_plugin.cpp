#include "shared/plugin_types.hpp"

#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>
#include <helios/platform/platform.hpp>

#include <string_view>

namespace happ = helios::app;
namespace hlog = helios::log;

namespace example_plugins {

struct ExampleDynamicPlugin final : public happ::Plugin {
  static constexpr std::string_view kName =
      "example_plugins::ExampleDynamicPlugin";

  void Build(happ::App& app) override {
    // The dynamically loaded plugin can use the same Plugin API as a static
    // plugin. Here it contributes one resource consumed by the host executable.
    hlog::Info("dynamic_plugins: ExampleDynamicPlugin::Build");
    app.InsertResources(PluginTicks{.count = 0});
  }

  void Finish(happ::App& /*app*/) override {
    hlog::Info("dynamic_plugins: ExampleDynamicPlugin::Finish");
  }

  void Destroy(happ::App& /*app*/) override {
    hlog::Info("dynamic_plugins: ExampleDynamicPlugin::Destroy");
  }
};

}  // namespace example_plugins

extern "C" {

// Dynamic plugins expose a small C ABI. The host looks up this symbol and takes
// ownership of the returned Plugin pointer.
HELIOS_EXPORT happ::Plugin* helios_create_plugin() {
  return new example_plugins::ExampleDynamicPlugin();
}

// The type export lets the host identify this plugin without relying on C++
// RTTI across the shared-library boundary.
HELIOS_EXPORT const happ::PluginTypeExport* helios_plugin_type_id() {
  static const auto kExport =
      happ::PluginTypeExport::From<example_plugins::ExampleDynamicPlugin>();
  return &kExport;
}

}  // extern "C"
