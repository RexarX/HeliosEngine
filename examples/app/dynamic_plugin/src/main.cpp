#include "helios/app/schedules.hpp"
#include "shared/plugin_types.hpp"

#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <filesystem>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct LogPluginResource {
  void operator()(hecs::Res<example_plugins::PluginTicks> ticks) const {
    // The resource was inserted by the shared-library plugin. After loading,
    // systems in the host app access it like any other world resource.
    ++ticks->count;
    hlog::Info("dynamic_plugins: plugin resource tick {}", ticks->count);
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 5) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
#ifndef HELIOS_EXAMPLE_PLUGIN_PATH
  hlog::Error("dynamic_plugins: HELIOS_EXAMPLE_PLUGIN_PATH not defined");
  return 1;
#else
  // CMake fills this definition with the built plugin DLL path, keeping the
  // example independent from the current working directory.
  const std::filesystem::path plugin_path{HELIOS_EXAMPLE_PLUGIN_PATH};
  hlog::Info("dynamic_plugins: loading '{}'", plugin_path.string());

  happ::DynamicPlugin dynamic_plugin;

  // Loading validates the exported symbols and constructs the plugin object.
  const auto load_result = dynamic_plugin.Load(plugin_path);
  if (!load_result) [[unlikely]] {
    hlog::Error("dynamic_plugins: load failed ({})",
                happ::DynamicPluginErrorToString(load_result.error()));
    return 1;
  }

  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});

  // AddDynamicPlugin gives the app ownership of the loaded plugin lifecycle.
  app.AddDynamicPlugin(std::move(dynamic_plugin));
  app.AddSystem(happ::kUpdate, LogPluginResource{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
#endif
}
