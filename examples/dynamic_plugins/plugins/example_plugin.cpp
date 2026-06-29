#include "shared/plugin_types.hpp"

#include <helios/app/application.hpp>
#include <helios/app/dynamic_plugin.hpp>
#include <helios/app/plugin.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/log/logger.hpp>
#include <helios/platform/platform.hpp>

#include <string_view>

namespace happ = helios::app;
namespace hlog = helios::log;

namespace example_plugins {

struct ExampleDynamicPlugin final : public happ::Plugin {
  static constexpr std::string_view kName =
      "example_plugins::ExampleDynamicPlugin";

  void Build(happ::App& app) override {
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

HELIOS_EXPORT happ::Plugin* helios_create_plugin() {
  return new example_plugins::ExampleDynamicPlugin();
}

HELIOS_EXPORT const happ::PluginTypeExport* helios_plugin_type_id() {
  static const auto kExport =
      happ::PluginTypeExport::From<example_plugins::ExampleDynamicPlugin>();
  return &kExport;
}

}  // extern "C"
