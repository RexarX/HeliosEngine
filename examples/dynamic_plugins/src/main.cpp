#include "shared/plugin_types.hpp"

#include <helios/app/application.hpp>
#include <helios/app/dynamic_plugin.hpp>
#include <helios/app/schedules.hpp>
#include <helios/ecs/message/writer.hpp>
#include <helios/ecs/resource/param.hpp>
#include <helios/log/logger.hpp>

#include <filesystem>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

struct FrameCount {
  size_t count = 0;
};

struct BumpFrame {
  void operator()(hecs::Res<FrameCount> frames) const { ++frames->count; }
};

struct LogPluginResource {
  void operator()(hecs::Res<example_plugins::PluginTicks> ticks) const {
    ++ticks->count;
    hlog::Info("dynamic_plugins: plugin resource tick {}", ticks->count);
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 5) {
      exit_writer.Write({.code = happ::ExitCode::kSuccess});
    }
  }
};

}  // namespace

int main() {
#ifndef HELIOS_EXAMPLE_PLUGIN_PATH
  hlog::Error("dynamic_plugins: HELIOS_EXAMPLE_PLUGIN_PATH not defined");
  return 1;
#else
  const std::filesystem::path plugin_path{HELIOS_EXAMPLE_PLUGIN_PATH};
  hlog::Info("dynamic_plugins: loading '{}'", plugin_path.string());

  happ::DynamicPlugin dynamic_plugin;
  const auto load_result = dynamic_plugin.Load(plugin_path);
  if (!load_result) {
    hlog::Error("dynamic_plugins: load failed ({})",
                happ::DynamicPluginErrorToString(load_result.error()));
    return 1;
  }

  happ::App app;
  app.InsertResources(FrameCount{});
  app.AddDynamicPlugin(std::move(dynamic_plugin));
  app.AddSystem(happ::kFirst, BumpFrame{});
  app.AddSystem(happ::kUpdate, LogPluginResource{});
  app.AddSystem(happ::kLast, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
#endif
}
