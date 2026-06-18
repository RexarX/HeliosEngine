#include <helios/app/dynamic_plugin.hpp>
#include <helios/app/plugin.hpp>
#include <helios/platform/platform.hpp>

#include <string_view>

namespace helios::app::test_plugins {

struct TestDynamicPlugin final : public Plugin {
  static constexpr std::string_view kName = "TestDynamicPlugin";

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

}  // namespace helios::app::test_plugins

extern "C" {

HELIOS_EXPORT helios::app::Plugin* helios_create_plugin() {
  return new helios::app::test_plugins::TestDynamicPlugin();
}

HELIOS_EXPORT const helios::app::PluginTypeExport* helios_plugin_type_id() {
  static const auto kExport = helios::app::PluginTypeExport::From<
      helios::app::test_plugins::TestDynamicPlugin>();
  return &kExport;
}

}  // extern "C"
