#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>

#include <string_view>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;

namespace {

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

  void Build(happ::App& app) override {
    hlog::Info("plugins: DemoPlugin::Build");
    app.InsertResources(PluginState{.systems_registered = 1});
    app.AddSystem(happ::kUpdate, PluginTick{});
  }

  void Finish(happ::App& /*app*/) override {
    hlog::Info("plugins: DemoPlugin::Finish");
  }

  void Poll(happ::App& /*app*/) override {
    hlog::Info("plugins: DemoPlugin::Poll");
  }

  [[nodiscard]] bool IsReady(const happ::App& /*app*/) const noexcept override {
    return ready_;
  }

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
  plugin.ready_ = true;

  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.AddPlugins(std::move(plugin));
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  const auto code = app.Run();
  return std::to_underlying(code);
}
