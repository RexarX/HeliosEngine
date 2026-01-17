#include <doctest/doctest.h>

#include <helios/core/app/app.hpp>
#include <helios/core/app/runners.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/logger.hpp>

using namespace helios::app;
using namespace helios::ecs;

namespace {

// ============================================================================
// Test Resources
// ============================================================================

struct FrameCounter {
  int count = 0;
  static constexpr std::string_view GetName() noexcept { return "FrameCounter"; }
};

struct ShutdownTrigger {
  int trigger_frame = -1;
  static constexpr std::string_view GetName() noexcept { return "ShutdownTrigger"; }
};

struct TimingCounter {
  int frames = 0;
  float total_delta = 0.0F;
  static constexpr std::string_view GetName() noexcept { return "TimingCounter"; }
};

// ============================================================================
// Test Systems
// ============================================================================

struct IncrementFrameSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "IncrementFrameSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return AccessPolicy().WriteResources<FrameCounter>(); }

  void Update(SystemContext& ctx) override { ++ctx.WriteResource<FrameCounter>().count; }
};

struct ShutdownTriggerSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "ShutdownTriggerSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().ReadResources<ShutdownTrigger>().WriteResources<FrameCounter>();
  }

  void Update(SystemContext& ctx) override {
    const auto& trigger = ctx.ReadResource<ShutdownTrigger>();
    auto& counter = ctx.WriteResource<FrameCounter>();

    if (trigger.trigger_frame == counter.count) {
      ctx.EmitEvent(ShutdownEvent{.exit_code = ShutdownExitCode::kSuccess});
    }
  }
};

struct TimingSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "TimingSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return AccessPolicy().ReadResources<Time>().WriteResources<TimingCounter>();
  }

  void Update(SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<Time>();
    auto& counter = ctx.WriteResource<TimingCounter>();

    ++counter.frames;
    counter.total_delta += time.DeltaSeconds();
  }
};

}  // namespace

// ============================================================================
// Test Suite
// ============================================================================

TEST_SUITE("app::Runners") {
  TEST_CASE("DefaultRunner: Basic execution") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 5});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      AppExitCode result = DefaultRunner(running_app);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    AppExitCode result = app.Run();

    CHECK_EQ(result, AppExitCode::kSuccess);
    CHECK_EQ(captured_frames, 5);
  }

  TEST_CASE("DefaultRunner: Updates time resource") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(TimingCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 3});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);
    app.AddSystem<TimingSystem>(kUpdate);

    float captured_delta = 0.0F;
    int captured_frames = 0;

    app.SetRunner([&captured_delta, &captured_frames](App& running_app) {
      DefaultRunnerConfig config{.update_time_resource = true};
      AppExitCode result = DefaultRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_delta = world.ReadResource<TimingCounter>().total_delta;
      captured_frames = world.ReadResource<TimingCounter>().frames;
      return result;
    });

    app.Run();

    CHECK_GT(captured_delta, 0.0F);
    CHECK_EQ(captured_frames, 3);
  }

  TEST_CASE("DefaultRunner: Graceful shutdown") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 3});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      AppExitCode result = DefaultRunner(running_app);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    AppExitCode result = app.Run();

    // Should exit gracefully when shutdown event is emitted
    CHECK_EQ(result, AppExitCode::kSuccess);
    CHECK_EQ(captured_frames, 3);
  }

  TEST_CASE("DefaultRunner: Disables time update") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(TimingCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 2});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);
    app.AddSystem<TimingSystem>(kUpdate);

    float captured_delta = 0.0F;

    app.SetRunner([&captured_delta](App& running_app) {
      DefaultRunnerConfig config{.update_time_resource = false};
      AppExitCode result = DefaultRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_delta = world.ReadResource<TimingCounter>().total_delta;
      return result;
    });

    app.Run();

    CHECK_EQ(captured_delta, 0.0F);
  }

  TEST_CASE("FrameLimitedRunner: Runs exact number of frames") {
    App app;
    app.InsertResource(FrameCounter{});
    app.AddSystem<IncrementFrameSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      FrameLimitedRunnerConfig config{.max_frames = 10};
      AppExitCode result = FrameLimitedRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    AppExitCode result = app.Run();

    CHECK_EQ(result, AppExitCode::kSuccess);
    CHECK_EQ(captured_frames, 10);
  }

  TEST_CASE("FrameLimitedRunner: Respects shutdown event") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 5});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      FrameLimitedRunnerConfig config{.max_frames = 100};
      AppExitCode result = FrameLimitedRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    app.Run();

    CHECK_EQ(captured_frames, 5);
  }

  TEST_CASE("FrameLimitedRunner: Early shutdown") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 5});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      FrameLimitedRunnerConfig config{.max_frames = 100};
      AppExitCode result = FrameLimitedRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    AppExitCode result = app.Run();

    // Should exit early due to shutdown event, not reaching max_frames
    CHECK_EQ(result, AppExitCode::kSuccess);
    CHECK_EQ(captured_frames, 5);
  }

  TEST_CASE("TimedRunner: Runs for specified duration") {
    App app;
    app.InsertResource(FrameCounter{});
    app.AddSystem<IncrementFrameSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      TimedRunnerConfig config{.duration_seconds = 0.1F};
      AppExitCode result = TimedRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    app.Run();

    CHECK_GT(captured_frames, 0);
  }

  TEST_CASE("TimedRunner: Respects shutdown event") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 3});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      TimedRunnerConfig config{.duration_seconds = 10.0F};
      AppExitCode result = TimedRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    app.Run();

    CHECK_EQ(captured_frames, 3);
  }

  TEST_CASE("TimedRunner: Early exit via event") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 2});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      TimedRunnerConfig config{.duration_seconds = 10.0F};
      AppExitCode result = TimedRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    AppExitCode result = app.Run();

    // Should exit early due to shutdown event before time limit
    CHECK_EQ(result, AppExitCode::kSuccess);
    CHECK_EQ(captured_frames, 2);
  }

  TEST_CASE("FixedTimestepRunner: Uses fixed timestep") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 5});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      FixedTimestepRunnerConfig config{.fixed_delta_seconds = 1.0F / 60.0F};
      AppExitCode result = FixedTimestepRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    app.Run();

    CHECK_EQ(captured_frames, 5);
  }

  TEST_CASE("FixedTimestepRunner: Respects shutdown event") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 4});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      FixedTimestepRunnerConfig config{.fixed_delta_seconds = 1.0F / 60.0F};
      AppExitCode result = FixedTimestepRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    app.Run();

    CHECK_EQ(captured_frames, 4);
  }

  TEST_CASE("FixedTimestepRunner: Limits substeps") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 2});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      FixedTimestepRunnerConfig config{.fixed_delta_seconds = 1.0F / 60.0F, .max_substeps = 5};
      AppExitCode result = FixedTimestepRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    app.Run();

    CHECK_EQ(captured_frames, 2);
  }

  TEST_CASE("FixedTimestepRunner: Early exit via event") {
    App app;
    app.InsertResource(FrameCounter{});
    app.InsertResource(ShutdownTrigger{.trigger_frame = 3});
    app.AddSystem<IncrementFrameSystem>(kUpdate);
    app.AddSystem<ShutdownTriggerSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      FixedTimestepRunnerConfig config{.fixed_delta_seconds = 1.0F / 60.0F};
      AppExitCode result = FixedTimestepRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    AppExitCode result = app.Run();

    // Should exit early due to shutdown event
    CHECK_EQ(result, AppExitCode::kSuccess);
    CHECK_EQ(captured_frames, 3);
  }

  TEST_CASE("OnceRunner: Executes exactly one frame") {
    App app;
    app.InsertResource(FrameCounter{});
    app.AddSystem<IncrementFrameSystem>(kUpdate);

    int captured_frames = 0;

    app.SetRunner([&captured_frames](App& running_app) {
      AppExitCode result = OnceRunner(running_app);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_frames = world.ReadResource<FrameCounter>().count;
      return result;
    });

    AppExitCode result = app.Run();

    CHECK_EQ(result, AppExitCode::kSuccess);
    CHECK_EQ(captured_frames, 1);
  }

  TEST_CASE("OnceRunner: Updates time resource") {
    App app;
    app.InsertResource(TimingCounter{});
    app.AddSystem<TimingSystem>(kUpdate);

    float captured_delta = 0.0F;

    app.SetRunner([&captured_delta](App& running_app) {
      OnceRunnerConfig config{.update_time_resource = true};
      AppExitCode result = OnceRunner(running_app, config);
      const auto& world = std::as_const(running_app).GetMainWorld();
      captured_delta = world.ReadResource<TimingCounter>().total_delta;
      return result;
    });

    app.Run();

    CHECK_GT(captured_delta, 0.0F);
  }

  TEST_CASE("CheckShutdownEvent: Returns false when no event") {
    App app;

    const auto [should_shutdown, exit_code] = CheckShutdownEvent(app);

    CHECK_FALSE(should_shutdown);
    CHECK_EQ(exit_code, ShutdownExitCode::kSuccess);
  }

  TEST_CASE("ToAppExitCode: Converts success") {
    AppExitCode result = ToAppExitCode(ShutdownExitCode::kSuccess);
    CHECK_EQ(result, AppExitCode::kSuccess);
  }

  TEST_CASE("ToAppExitCode: Converts failure") {
    AppExitCode result = ToAppExitCode(ShutdownExitCode::kFailure);
    CHECK_EQ(result, AppExitCode::kFailure);
  }

  TEST_CASE("DefaultRunnerConfig: Default values") {
    DefaultRunnerConfig config{};
    CHECK(config.update_time_resource);
  }

  TEST_CASE("FrameLimitedRunnerConfig: Default values") {
    FrameLimitedRunnerConfig config{};
    CHECK_EQ(config.max_frames, 1);
    CHECK(config.update_time_resource);
  }

  TEST_CASE("TimedRunnerConfig: Default values") {
    TimedRunnerConfig config{};
    CHECK_EQ(config.duration_seconds, 1.0F);
    CHECK(config.update_time_resource);
  }

  TEST_CASE("FixedTimestepRunnerConfig: Default values") {
    FixedTimestepRunnerConfig config{};
    CHECK_EQ(config.fixed_delta_seconds, 1.0F / 60.0F);
    CHECK_EQ(config.max_substeps, 10);
    CHECK(config.update_time_resource);
  }

  TEST_CASE("OnceRunnerConfig: Default values") {
    OnceRunnerConfig config{};
    CHECK(config.update_time_resource);
  }
}
