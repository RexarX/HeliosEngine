#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <string_view>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

// Plugin-owned state is just a normal resource once inserted into the app.
struct PluginState {
  int systems_registered = 0;
};

struct PluginTick {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::Res<const PluginState> state) const {
    hlog::Info("plugins: PluginTick frame={} registered={}", frames->count,
               state->systems_registered);
  }
};

struct DemoPlugin final : public happ::Plugin {
  static constexpr std::string_view kName = "DemoPlugin";

  // Build is where a plugin extends the app: resources, systems, schedules,
  // and other plugins are registered here.
  void Build(happ::App& app) override {
    hlog::Info("plugins: DemoPlugin::Build");
    app.InsertResources(PluginState{.systems_registered = 1});
    app.AddSystem(happ::kUpdate, PluginTick{});
  }

  // Finish runs after the plugin is ready, giving it one last setup hook
  // before normal app execution starts.
  void Finish(happ::App& /*app*/) override {
    hlog::Info("plugins: DemoPlugin::Finish");
  }

  // Poll is called while IsReady is false. Async plugins can use this to
  // finish background setup before exposing their systems.
  void Poll(happ::App& /*app*/) override {
    hlog::Info("plugins: DemoPlugin::Poll");
  }

  // Returning false delays Finish and app startup.
  [[nodiscard]] bool IsReady(const happ::App& /*app*/) const noexcept override {
    return ready_;
  }

  // Destroy is the plugin shutdown hook. It runs after app systems have
  // stopped, so plugin-owned external resources can be released here.
  void Destroy(happ::App& /*app*/) override {
    hlog::Info("plugins: DemoPlugin::Destroy");
  }

  bool ready_ = false;
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
  DemoPlugin plugin;

  // The demo marks the plugin ready immediately so startup can complete in one
  // pass. A real plugin might flip this after loading files or devices.
  plugin.ready_ = true;

  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});

  // Moving the plugin transfers ownership of its lifecycle to the app.
  app.AddPlugins(std::move(plugin));
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
