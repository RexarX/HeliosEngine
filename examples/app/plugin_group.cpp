#include <helios/app/app.hpp>
#include <helios/log/log.hpp>

namespace happ = helios::app;
namespace hlog = helios::log;

namespace {

struct First final : public happ::Plugin {
  void Build(happ::App& /*app*/) override {
    hlog::Info("plugin_groups: First::Build");
  }
};

struct Second final : public happ::Plugin {
  void Build(happ::App& /*app*/) override {
    hlog::Info("plugin_groups: Second::Build");
  }
};

struct MyGroup final : public happ::PluginGroup {
  MyGroup() : PluginGroup(First{}) {
    // A group can combine plugins passed to the base constructor with plugins
    // registered through Add in the constructor body.
    Add(Second{});
  }
};

}  // namespace

int main() {
  happ::App app;

  // AddPluginGroups adds every plugin registered by the group to the app.
  app.AddPluginGroups(MyGroup{});
  app.Initialize();
}
