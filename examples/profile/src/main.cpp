#include <helios/app/app.hpp>
#include <helios/core/core.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>
#include <helios/profile/profile.hpp>

#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;
namespace hprofile = helios::profile;

namespace {

class LoggingBackend final : public hprofile::Backend {
public:
  [[nodiscard]] size_t ZoneStorageSize() const noexcept override { return 0; }

  void BeginZone(const hprofile::ZoneSpec& /*spec*/,
                 std::span<std::byte> /*storage*/) noexcept override {}

  void EndZone(std::span<std::byte> /*storage*/) noexcept override {}

  void ZoneText(std::span<std::byte> /*storage*/,
                std::string_view /*text*/) noexcept override {}

  void ZoneValue(std::span<std::byte> /*storage*/,
                 uint64_t /*value*/) noexcept override {}

  void ZoneName(std::span<std::byte> /*storage*/,
                std::string_view /*name*/) noexcept override {}

  void FrameMark() noexcept override {}

  void FrameMark(helios::CStringView /*name*/) noexcept override {}

  void FrameMarkStart(helios::CStringView /*name*/) noexcept override {}

  void FrameMarkEnd(helios::CStringView /*name*/) noexcept override {}

  void Message(std::string_view text, uint32_t /*color*/) noexcept override {
    hlog::Info("profile[logging]: {}", text);
  }

  void SetThreadName(helios::CStringView /*name*/) noexcept override {}

  void Plot(helios::CStringView /*name*/, double /*value*/) noexcept override {}

  void PlotConfig(helios::CStringView /*name*/, hprofile::PlotFormat /*type*/,
                  bool /*step*/, bool /*fill*/,
                  uint32_t /*color*/) noexcept override {}

  void Alloc(const void* /*ptr*/, size_t /*size*/,
             std::optional<helios::CStringView> /*text*/, int /*depth*/,
             std::source_location /*loc*/) noexcept override {}

  void Free(const void* /*ptr*/, std::optional<helios::CStringView> /*text*/,
            int /*depth*/, std::source_location /*loc*/) noexcept override {}

  void MemoryDiscard(helios::CStringView /*name*/) noexcept override {}

  void MemoryDiscard(helios::CStringView /*name*/,
                     int /*depth*/) noexcept override {}

  [[nodiscard]] std::string_view Name() const noexcept override {
    return "logging";
  }
};

struct PhysicsLabel {};

struct PhysicsCounter {
  size_t bodies = 0;
};

struct PreUpdateWork {
  void operator()() const { HELIOS_PROFILE_SCOPE_N("PreUpdateWork"); }
};

struct UpdateWork {
  void operator()(hecs::Res<const happ::FrameCount> frames) const {
    HELIOS_PROFILE_SCOPE_N("UpdateWork");
    HELIOS_PROFILE_MESSAGE("update tick");
    HELIOS_PROFILE_ZONE_VALUE(frames->count);
    HELIOS_PROFILE_PLOT("frame_count", static_cast<double>(frames->count));
  }
};

struct PostUpdateWork {
  void operator()() const { HELIOS_PROFILE_SCOPE_N("PostUpdateWork"); }
};

struct SimulatePhysics {
  void operator()(hecs::Res<PhysicsCounter> counter) const {
    HELIOS_PROFILE_SCOPE_N("SimulatePhysics");
    ++counter->bodies;
  }
};

struct ExitAfterFrames {
  void operator()(hecs::Res<const happ::FrameCount> frames,
                  hecs::MessageWriter<happ::AppExit> exit_writer) const {
    if (frames->count >= 100) {
      exit_writer.Write(happ::AppExit::Success());
    }
  }
};

}  // namespace

int main() {
  auto& profiler = hprofile::Profiler::Instance();
  profiler.AddBackend<LoggingBackend>();
  profiler.Finalize();

  HELIOS_PROFILE_SET_THREAD_NAME("profile_example_main");

  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});
  app.AddSystem(happ::kPreUpdate, PreUpdateWork{});
  app.AddSystem(happ::kUpdate, UpdateWork{});
  app.AddSystem(happ::kPostUpdate, PostUpdateWork{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  auto physics = happ::SubApp::From(PhysicsLabel{});
  physics.InsertResources(PhysicsCounter{});
  physics.AddSystem(happ::kUpdate, SimulatePhysics{});
  app.InsertSubApp(PhysicsLabel{}, std::move(physics));

  const auto code = app.Run();
  return std::to_underlying(code);
}
