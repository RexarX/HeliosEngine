#include <helios/app/app.hpp>
#include <helios/core/core.hpp>
#include <helios/ecs/ecs.hpp>
#include <helios/log/log.hpp>
#include <helios/profile/backends/tracy.hpp>
#include <helios/profile/profile.hpp>

#include <cstddef>
#include <cstdint>
#include <new>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>
#include <thread>
#include <utility>

namespace happ = helios::app;
namespace hecs = helios::ecs;
namespace hlog = helios::log;
namespace hprofile = helios::profile;

namespace {

// This backend keeps profiling enabled without requiring an external viewer.
// It ignores zone storage and forwards profile messages to the normal logger.
class LoggingBackend final : public hprofile::Backend {
public:
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

  [[nodiscard]] size_t ZoneStorageSize() const noexcept override { return 0; }

  [[nodiscard]] std::string_view Name() const noexcept override {
    return "logging";
  }
};

struct PhysicsLabel {};

struct PhysicsCounter {
  size_t bodies = 0;
};

struct PreUpdateWork {
  // Profile scopes measure the lifetime of the current C++ scope.
  void operator()() const { HELIOS_PROFILE_SCOPE_N("PreUpdateWork"); }
};

struct UpdateWork {
  void operator()(hecs::Res<const happ::FrameCount> frames) const {
    HELIOS_PROFILE_SCOPE_N("UpdateWork");
    // Messages, values, and plots annotate the current backend stream.
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

  // Backends must be registered before Finalize. After that, profiling macros
  // can dispatch events without mutating backend configuration.
  profiler.AddBackend<LoggingBackend>();

  // Profile module provides a Tracy backend that can be used to visualize
  // profiling data in the Tracy profiler application. This backend is optional
  // and can be omitted if you don't want to use Tracy.
  profiler.AddBackend<hprofile::TracyBackend>();
  profiler.Finalize();

  HELIOS_PROFILE_SET_THREAD_NAME("profile_example_main");

  happ::App app;
  app.AddPlugins(happ::FrameCountPlugin{});

  // The app schedules profile-marked work across the normal frame stages.
  app.AddSystem(happ::kPreUpdate, PreUpdateWork{});
  app.AddSystem(happ::kUpdate, UpdateWork{});
  app.AddSystem(happ::kPostUpdate, PostUpdateWork{});
  app.AddSystem(happ::kPostUpdate, ExitAfterFrames{});

  // Sub-app work is profiled through the same global profiler instance.
  auto physics = happ::SubApp::From(PhysicsLabel{});
  physics.InsertResources(PhysicsCounter{});
  physics.AddSystem(happ::kUpdate, SimulatePhysics{});
  app.InsertSubApp(PhysicsLabel{}, std::move(physics));

  // Wait for tracy to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  const auto code = app.Run();
  return std::to_underlying(code);
}
